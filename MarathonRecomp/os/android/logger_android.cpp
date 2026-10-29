#include <os/logger.h>
#include <os/android/logger_android.h>
#include <os/android/storage_android.h>

#include <android/log.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <sys/utsname.h>
#include <ucontext.h>

// Ported from UnleashedRecomp-Android (os/android/logger_android.cpp); the frame
// watchdog and test-location markers are not ported yet.

#define ANDROID_LOG_TAG "MarathonRecomp"

// ---------------------------------------------------------------------------
// Persistent on-device log file
// ---------------------------------------------------------------------------
// logcat is often drowned out by driver spam and testers may have no adb, so every
// log line, plus captured stderr (where plume prints Vulkan errors), is mirrored into
// a plain text file on external app storage that can be copied off over MTP:
//   Android/data/<pkg>/files/log.txt

static std::mutex s_logMutex;
static FILE* s_logFile = nullptr;
static bool s_logFileOpenAttempted = false;

// Raw descriptor of log.txt for the crash handler, which cannot take s_logMutex or use stdio.
static std::atomic<int> s_logRawFd{ -1 };

static int GetTid()
{
    return static_cast<int>(syscall(SYS_gettid));
}

static double MonotonicSeconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return double(ts.tv_sec) + double(ts.tv_nsec) / 1e9;
}

static const double s_startSeconds = MonotonicSeconds();

// Caller must hold s_logMutex. Opens the file lazily because external storage can
// only be resolved once SDL/JNI is up, which is later than the first log lines.
static FILE* GetLogFileLocked()
{
    if (s_logFile != nullptr || s_logFileOpenAttempted)
        return s_logFile;

    const std::filesystem::path& dir = os::android::GetExternalFilesDir();
    if (dir.empty())
        return nullptr; // not ready yet - try again on the next log line

    s_logFileOpenAttempted = true;

    std::error_code ec;
    std::filesystem::path path = dir / "log.txt";
    if (std::filesystem::exists(path, ec))
        std::filesystem::rename(path, dir / "log_prev.txt", ec);

    s_logFile = fopen(path.c_str(), "wb");
    if (s_logFile != nullptr)
    {
        setvbuf(s_logFile, nullptr, _IONBF, 0); // unbuffered: a hang/kill keeps the tail
        s_logRawFd.store(fileno(s_logFile), std::memory_order_release);
    }

    return s_logFile;
}

// Writes one "[+seconds][tTID]<tag>[func] message" line to the log file. The line is assembled
// first and written with a single call: the file is unbuffered, so every fwrite/fputc would be
// a separate system call.
static void WriteLogRecord(const char* tag, const char* func, const char* msg, size_t msgLen)
{
    char prefix[80];
    int plen = snprintf(prefix, sizeof(prefix), "[%9.3f][t%d]%s",
        MonotonicSeconds() - s_startSeconds, GetTid(), tag != nullptr ? tag : "");
    if (plen < 0)
        plen = 0;
    else if (plen >= (int)sizeof(prefix))
        plen = (int)sizeof(prefix) - 1;

    thread_local std::string line;
    line.assign(prefix, size_t(plen));
    if (func != nullptr)
    {
        line += '[';
        line += func;
        line += ']';
    }
    line += ' ';
    if (msg != nullptr && msgLen > 0)
        line.append(msg, msgLen);
    line += '\n';

    std::lock_guard<std::mutex> lock(s_logMutex);
    FILE* file = GetLogFileLocked();
    if (file == nullptr)
        return;

    fwrite(line.data(), 1, line.size(), file);
}

// ---------------------------------------------------------------------------
// stderr -> logcat + log file
// ---------------------------------------------------------------------------
// plume and other thirdparty code report errors via fprintf(stderr, ...), which is
// otherwise discarded on Android. Redirect stderr through a pipe so those messages
// reach both logcat and log.txt.
static int s_stderrPipeReadFd = -1;

static void* StderrToLogcatThread(void*)
{
    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = read(s_stderrPipeReadFd, buffer, sizeof(buffer) - 1)) > 0)
    {
        if (buffer[bytesRead - 1] == '\n')
            --bytesRead;

        buffer[bytesRead] = '\0';
        __android_log_write(ANDROID_LOG_ERROR, "stderr", buffer);
        WriteLogRecord("[stderr]", nullptr, buffer, size_t(bytesRead));
    }

    return nullptr;
}

// ---------------------------------------------------------------------------
// Crash reporter
// ---------------------------------------------------------------------------
// Tombstones land in /data/tombstones, which testers cannot read without root. Catch
// fatal signals and append the signal, fault address and PC/LR (resolved to
// module+offset via dladdr) to log.txt with only async-signal-safe raw write()s, then
// re-raise into the previous handler so the system tombstone/debuggerd flow still runs.

static struct sigaction s_previousCrashActions[NSIG];

// Non-blocking pipe created with the handler. Writing from an address into it copies the memory
// in the kernel, which fails with EFAULT instead of faulting, so the handler can walk frame
// records of a corrupted or foreign stack safely.
static int s_crashProbePipe[2] = { -1, -1 };

// Per-thread descriptions set with os::android::SetCrashContext. Fixed slots so the signal
// handler can read them without locks or allocation; a torn text is acceptable for diagnostics.
struct CrashContextSlot
{
    std::atomic<int> tid{ 0 };
    char text[512]{};
};

static CrashContextSlot s_crashContexts[16];

static void CrashWriteRaw(int fd, const char* str)
{
    size_t len = strlen(str);
    while (len > 0)
    {
        const ssize_t written = write(fd, str, len);
        if (written <= 0)
            return;

        str += written;
        len -= size_t(written);
    }
}

static void CrashWriteHex(int fd, uint64_t value)
{
    char buffer[19];
    char* p = buffer + sizeof(buffer);
    *--p = '\0';
    do
    {
        *--p = "0123456789abcdef"[value & 0xF];
        value >>= 4;
    } while (value != 0);
    *--p = 'x';
    *--p = '0';
    CrashWriteRaw(fd, p);
}

static void CrashWriteDec(int fd, uint64_t value)
{
    char buffer[21];
    char* p = buffer + sizeof(buffer);
    *--p = '\0';
    do
    {
        *--p = char('0' + value % 10);
        value /= 10;
    } while (value != 0);
    CrashWriteRaw(fd, p);
}

// dladdr is not formally async-signal-safe, but bionic's implementation only walks
// already-loaded soinfo structures without allocating; debuggerd relies on the same.
static void CrashWriteAddress(int fd, const char* label, uint64_t address)
{
    CrashWriteRaw(fd, label);
    CrashWriteHex(fd, address);

    Dl_info info{};
    if (dladdr(reinterpret_cast<void*>(address), &info) != 0 && info.dli_fname != nullptr)
    {
        const char* baseName = strrchr(info.dli_fname, '/');
        CrashWriteRaw(fd, " (");
        CrashWriteRaw(fd, baseName != nullptr ? baseName + 1 : info.dli_fname);
        CrashWriteRaw(fd, "+");
        CrashWriteHex(fd, address - reinterpret_cast<uint64_t>(info.dli_fbase));
        CrashWriteRaw(fd, ")");
    }
}

static bool CrashReadPair(uint64_t address, uint64_t out[2])
{
    if (s_crashProbePipe[1] < 0 || address == 0 || (address & 0x7) != 0)
        return false;

    const ssize_t written = write(s_crashProbePipe[1], reinterpret_cast<const void*>(address), 2 * sizeof(uint64_t));
    if (written <= 0)
        return false;

    const ssize_t readBack = read(s_crashProbePipe[0], out, size_t(written));
    return written == ssize_t(2 * sizeof(uint64_t)) && readBack == written;
}

static void CrashWriteThreadName(int fd)
{
    char path[64] = "/proc/self/task/";
    char tidText[21];
    char* p = tidText + sizeof(tidText);
    *--p = '\0';
    uint64_t tid = uint64_t(GetTid());
    do
    {
        *--p = char('0' + tid % 10);
        tid /= 10;
    } while (tid != 0);
    strlcat(path, p, sizeof(path));
    strlcat(path, "/comm", sizeof(path));

    const int commFd = open(path, O_RDONLY | O_CLOEXEC);
    if (commFd < 0)
        return;

    char name[32]{};
    const ssize_t length = read(commFd, name, sizeof(name) - 1);
    close(commFd);
    if (length <= 0)
        return;

    for (ssize_t i = 0; i < length; i++)
    {
        if (name[i] == '\n')
            name[i] = '\0';
    }

    CrashWriteRaw(fd, " thread=\"");
    CrashWriteRaw(fd, name);
    CrashWriteRaw(fd, "\"");
}

#if defined(__aarch64__)
// Return addresses may carry pointer-authentication or tag bits above the 48-bit address space.
static constexpr uint64_t CRASH_ADDRESS_MASK = 0x0000FFFFFFFFFFFFull;

static void CrashWriteFrame(int fd, uint32_t index, uint64_t address)
{
    CrashWriteRaw(fd, "[crash] #");
    if (index < 10)
        CrashWriteRaw(fd, "0");
    CrashWriteDec(fd, index);
    CrashWriteAddress(fd, " ", address & CRASH_ADDRESS_MASK);
    CrashWriteRaw(fd, "\n");
}

// Walks the AArch64 frame records (x29 -> {previous x29, return address}). Android code keeps
// frame pointers on arm64; the walk stops at the first record it cannot read or that does not
// move up the stack.
static void CrashWriteBacktrace(int fd, const ucontext_t* context)
{
    CrashWriteRaw(fd, "[crash] backtrace (frame pointers, #00 = pc, #01 = lr):\n");
    CrashWriteFrame(fd, 0, context->uc_mcontext.pc);
    CrashWriteFrame(fd, 1, context->uc_mcontext.regs[30]);

    uint64_t framePointer = context->uc_mcontext.regs[29];
    for (uint32_t index = 2; index < 48; index++)
    {
        uint64_t record[2];
        if (!CrashReadPair(framePointer, record))
            break;

        const uint64_t returnAddress = record[1] & CRASH_ADDRESS_MASK;
        if (returnAddress == 0)
            break;

        CrashWriteFrame(fd, index, returnAddress);

        if (record[0] <= framePointer)
            break;

        framePointer = record[0];
    }
}
#endif

static void CrashWriteContexts(int fd)
{
    const int tid = GetTid();
    for (auto& slot : s_crashContexts)
    {
        const int slotTid = slot.tid.load(std::memory_order_acquire);
        if (slotTid == 0 || slot.text[0] == '\0')
            continue;

        CrashWriteRaw(fd, slotTid == tid ? "[crash] context (this thread): " : "[crash] context tid=");
        if (slotTid != tid)
        {
            CrashWriteDec(fd, uint64_t(slotTid));
            CrashWriteRaw(fd, ": ");
        }
        CrashWriteRaw(fd, slot.text);
        CrashWriteRaw(fd, "\n");
    }
}

static void CrashSignalHandler(int signal, siginfo_t* info, void* contextPtr)
{
    const int fd = s_logRawFd.load(std::memory_order_acquire);
    if (fd >= 0)
    {
        CrashWriteRaw(fd, "[crash] runtime_ms=");
        CrashWriteDec(fd, uint64_t((MonotonicSeconds() - s_startSeconds) * 1000.0));
        CrashWriteRaw(fd, " FATAL SIGNAL ");
        CrashWriteDec(fd, uint64_t(signal));
        switch (signal)
        {
            case SIGSEGV: CrashWriteRaw(fd, " (SIGSEGV)"); break;
            case SIGABRT: CrashWriteRaw(fd, " (SIGABRT)"); break;
            case SIGBUS:  CrashWriteRaw(fd, " (SIGBUS)"); break;
            case SIGILL:  CrashWriteRaw(fd, " (SIGILL)"); break;
            case SIGFPE:  CrashWriteRaw(fd, " (SIGFPE)"); break;
            case SIGTRAP: CrashWriteRaw(fd, " (SIGTRAP)"); break;
        }

        CrashWriteRaw(fd, " code=");
        CrashWriteDec(fd, uint64_t(info != nullptr ? info->si_code : 0));
        CrashWriteRaw(fd, " tid=");
        CrashWriteDec(fd, uint64_t(GetTid()));
        CrashWriteThreadName(fd);
        if (info != nullptr && (signal == SIGSEGV || signal == SIGBUS))
        {
            CrashWriteRaw(fd, " fault_addr=");
            CrashWriteHex(fd, reinterpret_cast<uint64_t>(info->si_addr));
        }
        CrashWriteRaw(fd, "\n");

#if defined(__aarch64__)
        const ucontext_t* context = static_cast<const ucontext_t*>(contextPtr);
        if (context != nullptr)
        {
            CrashWriteRaw(fd, "[crash]");
            CrashWriteAddress(fd, " pc=", context->uc_mcontext.pc);
            CrashWriteAddress(fd, " lr=", context->uc_mcontext.regs[30]);
            CrashWriteRaw(fd, " sp=");
            CrashWriteHex(fd, context->uc_mcontext.sp);
            CrashWriteRaw(fd, "\n");
            CrashWriteBacktrace(fd, context);
        }
#endif

        CrashWriteContexts(fd);
        CrashWriteRaw(fd, "[crash] end of report; the system tombstone (if any) has the full backtrace.\n");
    }

    // Restore and re-raise so debuggerd still produces the real tombstone.
    sigaction(signal, &s_previousCrashActions[signal], nullptr);
    raise(signal);
}

// One alternate signal stack per thread, released when the thread exits.
struct CrashAltStack
{
    static constexpr size_t Size = 64 * 1024;
    void* memory = nullptr;

    void Install()
    {
        if (memory != nullptr)
            return;

        void* mapped = mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapped == MAP_FAILED)
            return;

        stack_t stack{};
        stack.ss_sp = mapped;
        stack.ss_size = Size;
        if (sigaltstack(&stack, nullptr) != 0)
        {
            munmap(mapped, Size);
            return;
        }

        memory = mapped;
    }

    ~CrashAltStack()
    {
        if (memory == nullptr)
            return;

        stack_t disable{};
        disable.ss_flags = SS_DISABLE;
        sigaltstack(&disable, nullptr);
        munmap(memory, Size);
    }
};

static thread_local CrashAltStack s_crashAltStack;

void os::android::SetCrashContext(const char* text)
{
    const int tid = GetTid();
    CrashContextSlot* own = nullptr;
    for (auto& slot : s_crashContexts)
    {
        if (slot.tid.load(std::memory_order_relaxed) == tid)
        {
            own = &slot;
            break;
        }
    }

    if (text == nullptr)
    {
        if (own != nullptr)
        {
            own->text[0] = '\0';
            own->tid.store(0, std::memory_order_release);
        }
        return;
    }

    if (own == nullptr)
    {
        for (auto& slot : s_crashContexts)
        {
            int expected = 0;
            if (slot.tid.compare_exchange_strong(expected, tid, std::memory_order_acq_rel))
            {
                own = &slot;
                break;
            }
        }
    }

    if (own != nullptr)
        strlcpy(own->text, text, sizeof(own->text));
}

void os::android::InstallCrashAltStackForCurrentThread()
{
    s_crashAltStack.Install();
}

static void InstallCrashHandler()
{
    os::android::InstallCrashAltStackForCurrentThread();

    if (pipe2(s_crashProbePipe, O_CLOEXEC | O_NONBLOCK) != 0)
    {
        s_crashProbePipe[0] = -1;
        s_crashProbePipe[1] = -1;
    }

    struct sigaction action{};
    action.sa_sigaction = CrashSignalHandler;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);

    for (const int signal : { SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE, SIGTRAP })
        sigaction(signal, &action, &s_previousCrashActions[signal]);
}

// ---------------------------------------------------------------------------
// Device info header
// ---------------------------------------------------------------------------
// Put the model, SoC, Android version, GPU HAL, page size and RAM at the top of every
// log so a bare log.txt is enough to tell which hardware a report belongs to.

static void LogDeviceInfo()
{
    const char* properties[][2] =
    {
        { "device", "ro.product.model" },
        { "brand", "ro.product.manufacturer" },
        { "soc", "ro.soc.model" },
        { "android", "ro.build.version.release" },
        { "sdk", "ro.build.version.sdk" },
        { "vulkan_hal", "ro.hardware.vulkan" },
        { "abi", "ro.product.cpu.abi" },
    };

    char line[512];
    int offset = 0;
    for (const auto& [label, property] : properties)
    {
        char value[PROP_VALUE_MAX]{};
        __system_property_get(property, value);
        offset += snprintf(line + offset, sizeof(line) - size_t(offset), "%s%s=%s",
            offset > 0 ? " " : "", label, value[0] != '\0' ? value : "?");
        if (offset >= int(sizeof(line)))
            break;
    }

    if (offset < int(sizeof(line)))
    {
        const long pages = sysconf(_SC_PHYS_PAGES);
        const long pageSize = sysconf(_SC_PAGE_SIZE);
        snprintf(line + offset, sizeof(line) - size_t(offset), " page_size=%ld ram_mb=%lld",
            pageSize, (long long)(pages) * pageSize / (1024 * 1024));
    }

    WriteLogRecord("[device]", nullptr, line, strlen(line));

    struct utsname kernel{};
    if (uname(&kernel) == 0)
    {
        snprintf(line, sizeof(line), "release=%s version=%s machine=%s",
            kernel.release, kernel.version, kernel.machine);
        WriteLogRecord("[kernel]", nullptr, line, strlen(line));
    }
}

// The launcher's game switches are marker files in the game root (*.enabled / *.disabled); with
// no marker every switch is at its recommended setting. Listing the markers present shows a
// changed switch in a tester's log without comparing numbers first.
void os::android::LogLauncherSwitches()
{
    const auto isMarker = [](const std::string& name)
    {
        for (const std::string_view suffix : { std::string_view(".enabled"), std::string_view(".disabled") })
        {
            if (name.size() > suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
                return true;
        }

        return false;
    };

    std::vector<std::string> markers;
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator(os::android::GetDataRoot(), error);
        !error && it != std::filesystem::directory_iterator(); it.increment(error))
    {
        const std::string name = it->path().filename().string();
        if (isMarker(name) && it->is_regular_file(error))
            markers.push_back(name);
    }

    std::sort(markers.begin(), markers.end());
    std::string line = markers.empty() ? "all at the recommended settings (no marker files)" : "differ from the recommended settings:";
    for (const std::string& marker : markers)
        line += " " + marker;

    WriteLogRecord("[switches]", nullptr, line.c_str(), line.size());
}

// ---------------------------------------------------------------------------
// os::logger interface
// ---------------------------------------------------------------------------

void os::logger::Init()
{
    int pipeFds[2];
    if (pipe(pipeFds) == 0)
    {
        setvbuf(stderr, nullptr, _IONBF, 0);
        dup2(pipeFds[1], STDERR_FILENO);
        close(pipeFds[1]);

        s_stderrPipeReadFd = pipeFds[0];

        pthread_t thread;
        pthread_create(&thread, nullptr, StderrToLogcatThread, nullptr);
        pthread_detach(thread);
    }

    // Create log.txt promptly (and roll the previous one) so a tester always finds a
    // fresh file, even if this run happens to log nothing else before a freeze.
    static constexpr char Started[] = "Marathon Recompiled Android log started";
    WriteLogRecord("[logger]", nullptr, Started, sizeof(Started) - 1);
    LogDeviceInfo();
    InstallCrashHandler();
}

// Utility lines dominate the log: a "Loading file" line per file request (logged twice, by the
// game's file hook and by FileSystem::ResolvePath), plus every stub call. Each costs a logcat
// write and a log.txt write on the loading thread, thousands per scene transition. Off by
// default; create <game root>/log_utility.enabled before launching to get them back.
bool os::logger::IsUtilityEnabled()
{
    static const bool enabled = []
    {
        std::error_code error;
        const bool requested = std::filesystem::exists(os::android::GetDataRoot() / "log_utility.enabled", error);
        if (requested)
        {
            static constexpr char Enabled[] = "Utility logging enabled by log_utility.enabled";
            WriteLogRecord("[logger]", nullptr, Enabled, sizeof(Enabled) - 1);
        }

        return requested;
    }();

    return enabled;
}

void os::logger::Log(const std::string_view str, ELogType type, const char* func)
{
    if (type == ELogType::Utility && !IsUtilityEnabled())
        return;

    android_LogPriority priority = ANDROID_LOG_INFO;
    const char* fileTag = "";
    switch (type)
    {
    case ELogType::Warning:
        priority = ANDROID_LOG_WARN;
        fileTag = "[warn]";
        break;
    case ELogType::Error:
        priority = ANDROID_LOG_ERROR;
        fileTag = "[error]";
        break;
    default:
        break;
    }

    if (func)
    {
        __android_log_print(priority, ANDROID_LOG_TAG, "[%s] %.*s", func, (int)(str.size()), str.data());
    }
    else
    {
        __android_log_print(priority, ANDROID_LOG_TAG, "%.*s", (int)(str.size()), str.data());
    }

    WriteLogRecord(fileTag, func, str.data(), str.size());
}
