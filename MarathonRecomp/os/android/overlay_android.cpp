#include "overlay_android.h"
#include <os/android/storage_android.h>
#include <os/logger.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <pthread.h>
#include <string>
#include <thread>
#include <unistd.h>

namespace
{
    std::atomic<pid_t> s_threads[int(os::android::overlay::Role::Count)]{};
    std::mutex s_statsMutex;
    os::android::overlay::Stats s_stats;
    std::once_flag s_startOnce;

    bool ReadText(const char* path, std::string& out)
    {
        std::ifstream stream(path);
        if (!stream)
            return false;
        std::getline(stream, out, '\0');
        return !out.empty();
    }

    // First integer in a file, e.g. "57 %" or "57".
    bool ReadInt(const char* path, long& value)
    {
        std::string text;
        if (!ReadText(path, text))
            return false;
        const char* p = text.c_str();
        while (*p && !(*p >= '0' && *p <= '9') && *p != '-')
            p++;
        if (!*p)
            return false;
        value = strtol(p, nullptr, 10);
        return true;
    }

    // Run time (ns) from schedstat and the last core (stat field 39) of one of our threads.
    bool ReadThread(pid_t tid, uint64_t& runNs, int& core)
    {
        char path[64];
        snprintf(path, sizeof(path), "/proc/self/task/%d/schedstat", int(tid));
        std::string text;
        if (!ReadText(path, text))
            return false;
        runNs = strtoull(text.c_str(), nullptr, 10);

        core = -1;
        snprintf(path, sizeof(path), "/proc/self/task/%d/stat", int(tid));
        if (ReadText(path, text))
        {
            const size_t close = text.rfind(')');
            if (close != std::string::npos)
            {
                const char* p = text.c_str() + close + 2;
                for (int field = 3; field < 39 && p != nullptr && *p; field++)
                {
                    p = strchr(p, ' ');
                    if (p != nullptr)
                        p++;
                }
                if (p != nullptr && *p)
                    core = atoi(p);
            }
        }
        return true;
    }

    // utime + stime of the whole process, in clock ticks (stat fields 14 and 15).
    bool ReadProcessTicks(uint64_t& ticks)
    {
        std::string text;
        if (!ReadText("/proc/self/stat", text))
            return false;
        const size_t close = text.rfind(')');
        if (close == std::string::npos)
            return false;
        const char* p = text.c_str() + close + 2;
        for (int field = 3; field < 14 && p != nullptr && *p; field++)
        {
            p = strchr(p, ' ');
            if (p != nullptr)
                p++;
        }
        if (p == nullptr)
            return false;
        char* end = nullptr;
        const uint64_t user = strtoull(p, &end, 10);
        const uint64_t system = strtoull(end, nullptr, 10);
        ticks = user + system;
        return true;
    }

    // Kernel files with the GPU busy percentage; most firmware hides them from apps.
    constexpr const char* GpuBusyPaths[] =
    {
        "/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage", // Adreno
        "/sys/kernel/gpu/gpu_busy",                     // some Samsung and MediaTek kernels
        "/sys/module/ged/parameters/gpu_loading",       // MediaTek GED
        "/sys/class/misc/mali0/device/utilization",     // some Mali kernels
    };

    void SampleLoop()
    {
        pthread_setname_np(pthread_self(), "OverlayStats");

        using AcquireFn = void* (*)();
        using StatusFn = int (*)(void*);
        using HeadroomFn = float (*)(void*, int);
        void* library = dlopen("libandroid.so", RTLD_NOW);
        auto acquire = library ? reinterpret_cast<AcquireFn>(dlsym(library, "AThermal_acquireManager")) : nullptr;
        auto status = library ? reinterpret_cast<StatusFn>(dlsym(library, "AThermal_getCurrentThermalStatus")) : nullptr;
        auto headroom = library ? reinterpret_cast<HeadroomFn>(dlsym(library, "AThermal_getThermalHeadroom")) : nullptr;
        void* manager = acquire ? acquire() : nullptr;

        os::android::overlay::Stats stats;
        stats.cpuCount = std::min<int>(int(sysconf(_SC_NPROCESSORS_CONF)), os::android::overlay::Stats::MaxCpus);
        for (int cpu = 0; cpu < stats.cpuCount; cpu++)
        {
            char path[96];
            long value = 0;
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpu_capacity", cpu);
            if (ReadInt(path, value))
                stats.capacity[cpu] = uint32_t(value);
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu);
            if (ReadInt(path, value))
                stats.maxMHz[cpu] = uint32_t(value / 1000);
        }

        const char* gpuPath = nullptr;
        for (const char* path : GpuBusyPaths)
        {
            long value = 0;
            if (ReadInt(path, value))
            {
                gpuPath = path;
                break;
            }
        }
        LOGFN("Overlay: GPU busy {}.", gpuPath ? gpuPath : "not readable on this device");

        const long ticksPerSecond = sysconf(_SC_CLK_TCK);
        const long pageSize = sysconf(_SC_PAGESIZE);
        uint64_t lastRun[int(os::android::overlay::Role::Count)]{};
        uint64_t lastTicks = 0;
        ReadProcessTicks(lastTicks);
        auto last = std::chrono::steady_clock::now();
        uint32_t iteration = 0;

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            const auto now = std::chrono::steady_clock::now();
            const double seconds = std::chrono::duration<double>(now - last).count();
            last = now;

            for (int cpu = 0; cpu < stats.cpuCount; cpu++)
            {
                char path[96];
                long value = 0;
                snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu);
                stats.curMHz[cpu] = ReadInt(path, value) ? uint32_t(value / 1000) : 0;
            }

            for (int role = 0; role < int(os::android::overlay::Role::Count); role++)
            {
                const pid_t tid = s_threads[role].load();
                uint64_t run = 0;
                int core = -1;
                if (tid > 0 && ReadThread(tid, run, core))
                {
                    stats.threadLoad[role] = lastRun[role] ? float(double(run - lastRun[role]) / (seconds * 1e9)) : 0.0f;
                    stats.threadCore[role] = core;
                    lastRun[role] = run;
                }
            }

            uint64_t ticks = 0;
            if (ReadProcessTicks(ticks))
            {
                stats.processCores = float(double(ticks - lastTicks) / double(ticksPerSecond) / seconds);
                lastTicks = ticks;
            }

            long value = 0;
            stats.gpuBusyPercent = (gpuPath && ReadInt(gpuPath, value)) ? int(value) : -1;

            // The headroom API returns NaN when asked more often than once a second.
            if (manager && (iteration % 2) == 0)
            {
                stats.thermalStatus = status ? status(manager) : -2;
                const float room = headroom ? headroom(manager, 0) : -1.0f;
                stats.thermalHeadroom = room == room ? room : -1.0f;
            }

            stats.batteryCelsius = ReadInt("/sys/class/power_supply/battery/temp", value) ? float(value) / 10.0f : -1000.0f;

            std::string statm;
            if (ReadText("/proc/self/statm", statm))
            {
                const char* p = strchr(statm.c_str(), ' ');
                if (p != nullptr)
                    stats.residentMB = double(strtoull(p + 1, nullptr, 10)) * double(pageSize) / (1024.0 * 1024.0);
            }

            stats.valid = true;
            iteration++;
            {
                std::lock_guard lock(s_statsMutex);
                s_stats = stats;
            }
        }
    }
}

bool os::android::overlay::IsEnabled()
{
    static const bool enabled = []
    {
        std::error_code error;
        const bool result = std::filesystem::exists(os::android::GetDataRoot() / "overlay.enabled", error);
        if (result)
            LOGN("Overlay: enabled by overlay.enabled.");
        return result;
    }();
    return enabled;
}

void os::android::overlay::SetThread(Role role, pid_t tid)
{
    s_threads[int(role)].store(tid);
}

os::android::overlay::Stats os::android::overlay::GetStats()
{
    std::call_once(s_startOnce, [] { std::thread(SampleLoop).detach(); });
    std::lock_guard lock(s_statsMutex);
    return s_stats;
}
