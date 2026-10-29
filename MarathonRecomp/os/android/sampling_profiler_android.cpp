#include "sampling_profiler_android.h"
#include <os/android/storage_android.h>
#include <os/logger.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <link.h>
#include <mutex>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#ifndef DT_GNU_HASH
#define DT_GNU_HASH 0x6ffffef5
#endif

// The tester devices cannot be profiled with simpleperf, so the guest thread samples itself:
// a timer on the thread's CPU-time clock sends SIGPROF every 1 ms of CPU time, the handler
// stores the interrupted PC and the return address of the current frame record in a ring, and
// a report thread aggregates the ring and writes the summary.
namespace
{
    constexpr uint32_t RingSize = 1u << 15;
    constexpr uint32_t MaxThreads = 4;
    constexpr auto ReportInterval = std::chrono::seconds(20);
    constexpr uintptr_t PointerMask = 0x0000FFFFFFFFFFFFull;

    struct Sample
    {
        uintptr_t pc;
        uintptr_t caller;
        // 1 + timer overruns: CPU-time timers are checked on the scheduler tick (4 ms at
        // HZ=250), so one signal can stand for several expired milliseconds.
        uint32_t weight;
    };

    struct SampledThread
    {
        char label[16]{};
        pid_t tid = 0;
        uintptr_t stackLow = 0, stackHigh = 0;
        std::atomic<uint32_t> head{ 0 }, tail{ 0 }, dropped{ 0 };
        Sample ring[RingSize];
    };

    SampledThread* s_threads[MaxThreads]{};
    std::atomic<uint32_t> s_threadCount{ 0 };
    std::mutex s_startMutex;

    void OnSample(int, siginfo_t* info, void* context)
    {
        const int savedErrno = errno;
        const pid_t tid = gettid();
        const uint32_t count = std::min(s_threadCount.load(std::memory_order_acquire), MaxThreads);
        for (uint32_t i = 0; i < count; i++)
        {
            SampledThread* thread = s_threads[i];
            if (thread == nullptr || thread->tid != tid)
                continue;

            const auto* uc = static_cast<const ucontext_t*>(context);
            const uintptr_t pc = uintptr_t(uc->uc_mcontext.pc);
            const uintptr_t fp = uintptr_t(uc->uc_mcontext.regs[29]);
            uintptr_t caller = 0;
            if ((fp & 7) == 0 && fp >= thread->stackLow && fp + 16 <= thread->stackHigh)
                caller = reinterpret_cast<const uintptr_t*>(fp)[1] & PointerMask;

            const uint32_t head = thread->head.load(std::memory_order_relaxed);
            if (head - thread->tail.load(std::memory_order_acquire) >= RingSize)
            {
                thread->dropped.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                thread->ring[head % RingSize] = { pc, caller, 1u + uint32_t(std::max(0, info->si_overrun)) };
                thread->head.store(head + 1, std::memory_order_release);
            }
            break;
        }
        errno = savedErrno;
    }

    // Exported functions of libmain.so from its dynamic symbol table, sorted by address.
    struct FunctionTable
    {
        struct Entry
        {
            uintptr_t start, size;
            const char* name;
        };
        std::vector<Entry> entries;
        uintptr_t base = 0, end = 0;

        static int Collect(dl_phdr_info* info, size_t, void* data)
        {
            auto* table = static_cast<FunctionTable*>(data);
            const char* name = info->dlpi_name;
            const size_t length = name ? strlen(name) : 0;
            if (length < 10 || strcmp(name + length - 10, "libmain.so") != 0)
                return 0;

            const uintptr_t base = info->dlpi_addr;
            const ElfW(Dyn)* dynamic = nullptr;
            for (int i = 0; i < info->dlpi_phnum; i++)
            {
                const auto& phdr = info->dlpi_phdr[i];
                if (phdr.p_type == PT_DYNAMIC)
                    dynamic = reinterpret_cast<const ElfW(Dyn)*>(base + phdr.p_vaddr);
                if (phdr.p_type == PT_LOAD)
                    table->end = std::max<uintptr_t>(table->end, base + phdr.p_vaddr + phdr.p_memsz);
            }
            if (dynamic == nullptr)
                return 1;

            // Bionic keeps the unrelocated addresses in .dynamic.
            auto resolve = [base](uintptr_t value) { return value < base ? value + base : value; };
            const ElfW(Sym)* symbols = nullptr;
            const char* strings = nullptr;
            const uint32_t* gnuHash = nullptr;
            for (const ElfW(Dyn)* entry = dynamic; entry->d_tag != DT_NULL; entry++)
            {
                if (entry->d_tag == DT_SYMTAB) symbols = reinterpret_cast<const ElfW(Sym)*>(resolve(entry->d_un.d_ptr));
                else if (entry->d_tag == DT_STRTAB) strings = reinterpret_cast<const char*>(resolve(entry->d_un.d_ptr));
                else if (entry->d_tag == DT_GNU_HASH) gnuHash = reinterpret_cast<const uint32_t*>(resolve(entry->d_un.d_ptr));
            }
            if (symbols == nullptr || strings == nullptr || gnuHash == nullptr)
                return 1;

            // Symbol count from the GNU hash table: the end of the chain of the last bucket.
            const uint32_t bucketCount = gnuHash[0], symbolOffset = gnuHash[1], bloomSize = gnuHash[2];
            const uint32_t* buckets = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint64_t*>(gnuHash + 4) + bloomSize);
            const uint32_t* chains = buckets + bucketCount;
            uint32_t last = 0;
            for (uint32_t i = 0; i < bucketCount; i++)
                last = std::max(last, buckets[i]);
            uint32_t symbolCount = symbolOffset;
            if (last >= symbolOffset)
            {
                while ((chains[last - symbolOffset] & 1) == 0)
                    last++;
                symbolCount = last + 1;
            }

            table->base = base;
            for (uint32_t i = 0; i < symbolCount; i++)
            {
                const auto& symbol = symbols[i];
                if (ELF64_ST_TYPE(symbol.st_info) == STT_FUNC && symbol.st_shndx != SHN_UNDEF && symbol.st_value != 0)
                    table->entries.push_back({ base + symbol.st_value, symbol.st_size, strings + symbol.st_name });
            }
            std::sort(table->entries.begin(), table->entries.end(), [](const Entry& a, const Entry& b) { return a.start < b.start; });
            return 1;
        }

        void Load()
        {
            dl_iterate_phdr(Collect, this);
        }

        const Entry* Find(uintptr_t pc) const
        {
            auto it = std::upper_bound(entries.begin(), entries.end(), pc, [](uintptr_t value, const Entry& entry) { return value < entry.start; });
            if (it == entries.begin())
                return nullptr;
            --it;
            return pc < it->start + std::max<uintptr_t>(it->size, 4) ? &*it : nullptr;
        }
    };

    // "_Z12sub_82160000R10PPCContextPh" -> "sub_82160000"; other names are kept as they are.
    std::string ShortName(const char* name)
    {
        std::string result = name ? name : "?";
        if (result.size() > 4 && result.compare(0, 2, "_Z") == 0)
        {
            size_t i = 2, length = 0;
            while (i < result.size() && result[i] >= '0' && result[i] <= '9')
                length = length * 10 + size_t(result[i++] - '0');
            if (length != 0 && i + length <= result.size() && result.compare(i, 4, "sub_") == 0)
                return result.substr(i, length);
        }
        if (result.size() > 72)
            result.resize(72);
        return result;
    }

    struct Symbolizer
    {
        FunctionTable libmain;
        std::unordered_map<uintptr_t, std::pair<std::string, std::string>> cache; // pc -> (function, library)

        // Groups a PC into a function (libmain export, other library symbol) or, when no symbol
        // covers it, the 256-byte block as "<library>+0x<offset>" for offline symbolisation.
        const std::pair<std::string, std::string>& Resolve(uintptr_t pc)
        {
            auto it = cache.find(pc);
            if (it != cache.end())
                return it->second;

            std::pair<std::string, std::string> result;
            if (pc >= libmain.base && pc < libmain.end && libmain.base != 0)
            {
                result.second = "libmain.so";
                if (const auto* entry = libmain.Find(pc))
                    result.first = ShortName(entry->name);
                else
                    result.first = fmt::format("libmain.so+0x{:x}", (pc - libmain.base) & ~uintptr_t(0xFF));
            }
            else
            {
                Dl_info info{};
                if (pc != 0 && dladdr(reinterpret_cast<void*>(pc), &info) != 0 && info.dli_fname != nullptr)
                {
                    const char* slash = strrchr(info.dli_fname, '/');
                    result.second = slash ? slash + 1 : info.dli_fname;
                    if (info.dli_sname != nullptr)
                        result.first = ShortName(info.dli_sname) + " (" + result.second + ")";
                    else
                        result.first = fmt::format("{}+0x{:x}", result.second, (pc - uintptr_t(info.dli_fbase)) & ~uintptr_t(0xFF));
                }
                else
                {
                    result.second = "?";
                    result.first = "?";
                }
            }
            return cache.emplace(pc, std::move(result)).first->second;
        }
    };

    void ReportSamples(Symbolizer& symbolizer, double seconds)
    {
        const uint32_t count = std::min(s_threadCount.load(std::memory_order_acquire), MaxThreads);
        for (uint32_t t = 0; t < count; t++)
        {
            SampledThread* thread = s_threads[t];
            if (thread == nullptr)
                continue;

            const uint32_t head = thread->head.load(std::memory_order_acquire);
            uint32_t tail = thread->tail.load(std::memory_order_relaxed);
            std::vector<Sample> samples;
            samples.reserve(head - tail);
            for (; tail != head; tail++)
                samples.push_back(thread->ring[tail % RingSize]);
            thread->tail.store(tail, std::memory_order_release);
            const uint32_t dropped = thread->dropped.exchange(0, std::memory_order_relaxed);

            if (samples.empty())
                continue;

            std::unordered_map<std::string, uint32_t> functions, libraries;
            std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> callers;
            uint64_t weighted = 0;
            for (const Sample& sample : samples)
            {
                const auto& [function, library] = symbolizer.Resolve(sample.pc);
                functions[function] += sample.weight;
                libraries[library] += sample.weight;
                const std::string& caller = sample.caller ? symbolizer.Resolve(sample.caller).first : std::string("?");
                callers[function][caller] += sample.weight;
                weighted += sample.weight;
            }

            const double total = double(weighted);
            auto sorted = [](const auto& map)
            {
                std::vector<std::pair<std::string, uint32_t>> result(map.begin(), map.end());
                std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
                return result;
            };

            std::string libraryText;
            for (const auto& [name, hits] : sorted(libraries))
            {
                if (hits * 200 < weighted)
                    break;
                libraryText += fmt::format(" {} {:.1f}%", name, 100.0 * hits / total);
            }
            LOGFN("[prof] {} {:.0f} s: {} samples for {} ms of thread CPU ({:.0f}% of one core), dropped {}; by library:{}",
                thread->label, seconds, samples.size(), weighted, 100.0 * total / (seconds * 1000.0), dropped, libraryText);

            const auto functionList = sorted(functions);
            for (size_t i = 0; i < functionList.size() && i < 25; i++)
            {
                const auto& [name, hits] = functionList[i];
                std::string callerText;
                const auto callerList = sorted(callers[name]);
                for (size_t c = 0; c < callerList.size() && c < 3; c++)
                    callerText += fmt::format("{}{} {:.0f}%", c ? ", " : "", callerList[c].first, 100.0 * callerList[c].second / hits);
                LOGFN("[prof] {} {:5.1f}% {} <- {}", thread->label, 100.0 * hits / total, name, callerText);
            }
        }
    }

    struct ThreadTimes
    {
        uint64_t runNs = 0, waitNs = 0;
    };

    bool ReadFile(const std::string& path, std::string& out)
    {
        std::ifstream stream(path);
        if (!stream)
            return false;
        std::getline(stream, out, '\0');
        return true;
    }

    void ReportThreads(std::unordered_map<pid_t, ThreadTimes>& previous, double seconds)
    {
        struct Row
        {
            std::string name;
            pid_t tid;
            double run, wait;
            int core;
        };
        std::vector<Row> rows;
        std::unordered_map<pid_t, ThreadTimes> current;
        DIR* dir = opendir("/proc/self/task");
        if (dir == nullptr)
            return;
        bool schedstatMissing = false;
        while (dirent* entry = readdir(dir))
        {
            const pid_t tid = pid_t(atoi(entry->d_name));
            if (tid <= 0)
                continue;
            const std::string base = std::string("/proc/self/task/") + entry->d_name;
            std::string text, name;
            ThreadTimes times;
            if (ReadFile(base + "/schedstat", text))
            {
                unsigned long long run = 0, wait = 0;
                sscanf(text.c_str(), "%llu %llu", &run, &wait);
                times = { run, wait };
            }
            else
            {
                schedstatMissing = true;
            }
            ReadFile(base + "/comm", name);
            while (!name.empty() && (name.back() == '\n' || name.back() == '\r'))
                name.pop_back();

            // Field 39 of stat is the core the thread last ran on.
            int core = -1;
            if (ReadFile(base + "/stat", text))
            {
                const size_t close = text.rfind(')');
                if (close != std::string::npos)
                {
                    const char* p = text.c_str() + close + 2;
                    for (int field = 3; field < 39 && *p; field++)
                    {
                        p = strchr(p, ' ');
                        if (p == nullptr)
                            break;
                        p++;
                    }
                    if (p != nullptr && *p)
                        core = atoi(p);
                }
            }

            current[tid] = times;
            auto it = previous.find(tid);
            if (it != previous.end())
            {
                const double run = double(times.runNs - it->second.runNs) / 1e6;
                const double wait = double(times.waitNs - it->second.waitNs) / 1e6;
                if (run > 0.0)
                    rows.push_back({ name, tid, run, wait, core });
            }
        }
        closedir(dir);
        const bool baseline = previous.empty();
        previous = std::move(current);
        if (baseline)
            return;

        if (schedstatMissing && rows.empty())
        {
            LOGN("[threads] /proc/self/task/*/schedstat is not readable on this device.");
            return;
        }

        std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.run > b.run; });
        double total = 0.0;
        for (const Row& row : rows)
            total += row.run;
        std::string text;
        for (size_t i = 0; i < rows.size() && i < 14; i++)
        {
            const Row& row = rows[i];
            text += fmt::format(" | {}:{} run {:.0f}% wait {:.0f}% core {}", row.name, row.tid,
                100.0 * row.run / (seconds * 1000.0), 100.0 * row.wait / (seconds * 1000.0), row.core);
        }
        LOGFN("[threads] {:.0f} s: process CPU {:.2f} cores{}", seconds, total / (seconds * 1000.0), text);
    }

    void ReportFrequencies()
    {
        std::string text;
        const long cores = sysconf(_SC_NPROCESSORS_CONF);
        for (long cpu = 0; cpu < cores && cpu < 16; cpu++)
        {
            std::string cur, max;
            const std::string base = fmt::format("/sys/devices/system/cpu/cpu{}/cpufreq/", cpu);
            if (!ReadFile(base + "scaling_cur_freq", cur))
                continue;
            ReadFile(base + "scaling_max_freq", max);
            text += fmt::format(" {}:{}/{}", cpu, atol(cur.c_str()) / 1000, atol(max.c_str()) / 1000);
        }
        if (text.empty())
            text = " unavailable";

        using AcquireFn = void* (*)();
        using StatusFn = int (*)(void*);
        using HeadroomFn = float (*)(void*, int);
        static void* library = dlopen("libandroid.so", RTLD_NOW);
        static auto acquire = library ? reinterpret_cast<AcquireFn>(dlsym(library, "AThermal_acquireManager")) : nullptr;
        static auto status = library ? reinterpret_cast<StatusFn>(dlsym(library, "AThermal_getCurrentThermalStatus")) : nullptr;
        static auto headroom = library ? reinterpret_cast<HeadroomFn>(dlsym(library, "AThermal_getThermalHeadroom")) : nullptr;
        static void* manager = acquire ? acquire() : nullptr;
        const int thermal = manager && status ? status(manager) : -2;
        const float room = manager && headroom ? headroom(manager, 0) : NAN;

        LOGFN("[cpufreq] MHz cur/max{}; thermal status {} (0 none .. 6 shutdown, -2 unavailable), headroom {:.2f}", text, thermal, room);
    }

    void ReportLoop()
    {
        pthread_setname_np(pthread_self(), "PerfReport");
        Symbolizer symbolizer;
        symbolizer.libmain.Load();
        LOGFN("[prof] libmain.so: {} exported functions, base 0x{:x}", symbolizer.libmain.entries.size(), symbolizer.libmain.base);

        std::unordered_map<pid_t, ThreadTimes> threadTimes;
        ReportThreads(threadTimes, 1.0); // baseline only, nothing is logged
        auto last = std::chrono::steady_clock::now();
        while (true)
        {
            std::this_thread::sleep_for(ReportInterval);
            const auto now = std::chrono::steady_clock::now();
            const double seconds = std::chrono::duration<double>(now - last).count();
            last = now;
            ReportSamples(symbolizer, seconds);
            ReportThreads(threadTimes, seconds);
            ReportFrequencies();
        }
    }
}

bool os::android::perf::IsCaptureRequested()
{
    static const bool requested = []
    {
        std::error_code error;
        return std::filesystem::exists(os::android::GetDataRoot() / "perf_capture.enabled", error);
    }();
    return requested;
}

std::string os::android::perf::ClusterClocks()
{
    // The first core of each run of cores sharing a maximum frequency.
    static const std::vector<int> s_clusters = []
    {
        std::vector<int> clusters;
        long previous = -1;
        const long cores = sysconf(_SC_NPROCESSORS_CONF);
        for (long cpu = 0; cpu < cores && cpu < 16; cpu++)
        {
            std::string max;
            if (!ReadFile(fmt::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq", cpu), max))
                continue;
            const long value = atol(max.c_str());
            if (value != previous)
                clusters.push_back(int(cpu));
            previous = value;
        }
        return clusters;
    }();

    std::string text;
    for (const int cpu : s_clusters)
    {
        std::string cur;
        const bool read = ReadFile(fmt::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_cur_freq", cpu), cur);
        text += fmt::format("{}{}", text.empty() ? "" : "/", read ? fmt::format("{}", atol(cur.c_str()) / 1000) : std::string("?"));
    }
    return text.empty() ? "n/a" : text;
}

int os::android::perf::ThreadCore(pid_t tid)
{
    if (tid <= 0)
        return -1;

    std::string text;
    if (!ReadFile(fmt::format("/proc/self/task/{}/stat", tid), text))
        return -1;

    const size_t close = text.rfind(')');
    if (close == std::string::npos || close + 2 >= text.size())
        return -1;

    // Fields after the name start at 3 (state); the last core is field 39.
    const char* p = text.c_str() + close + 2;
    for (int field = 3; field < 39 && p != nullptr && *p; field++)
    {
        p = strchr(p, ' ');
        if (p != nullptr)
            p++;
    }
    return p != nullptr && *p ? atoi(p) : -1;
}

void os::android::profiler::SampleCurrentThread(const char* label)
{
    std::error_code error;
    if (!std::filesystem::exists(os::android::GetDataRoot() / "perf_sampling.enabled", error))
        return;

    std::lock_guard lock(s_startMutex);
    const uint32_t index = s_threadCount.load(std::memory_order_relaxed);
    if (index >= MaxThreads)
        return;

    auto* thread = new SampledThread();
    strncpy(thread->label, label, sizeof(thread->label) - 1);
    thread->tid = gettid();
    pthread_attr_t attr;
    if (pthread_getattr_np(pthread_self(), &attr) == 0)
    {
        void* stack = nullptr;
        size_t size = 0;
        if (pthread_attr_getstack(&attr, &stack, &size) == 0)
        {
            thread->stackLow = uintptr_t(stack);
            thread->stackHigh = uintptr_t(stack) + size;
        }
        pthread_attr_destroy(&attr);
    }

    if (index == 0)
    {
        struct sigaction action{};
        action.sa_sigaction = OnSample;
        action.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGPROF, &action, nullptr) != 0)
        {
            LOGFN_WARNING("[prof] sigaction failed: {}", strerror(errno));
            delete thread;
            return;
        }
    }

    s_threads[index] = thread;
    s_threadCount.store(index + 1, std::memory_order_release);

    clockid_t clock;
    timer_t timer;
    sigevent event{};
    event.sigev_notify = SIGEV_THREAD_ID;
    event.sigev_signo = SIGPROF;
    event.sigev_notify_thread_id = thread->tid;
    if (pthread_getcpuclockid(pthread_self(), &clock) != 0 || timer_create(clock, &event, &timer) != 0)
    {
        LOGFN_WARNING("[prof] timer_create failed for \"{}\": {}", label, strerror(errno));
        return;
    }
    itimerspec interval{};
    interval.it_interval.tv_nsec = 1000000;
    interval.it_value.tv_nsec = 1000000;
    if (timer_settime(timer, 0, &interval, nullptr) != 0)
    {
        LOGFN_WARNING("[prof] timer_settime failed for \"{}\": {}", label, strerror(errno));
        return;
    }

    LOGFN("[prof] sampling \"{}\" (tid {}) every 1 ms of its CPU time.", label, thread->tid);

    if (index == 0)
        std::thread(ReportLoop).detach();
}
