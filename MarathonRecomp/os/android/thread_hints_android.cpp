#include "thread_hints_android.h"
#include <os/android/storage_android.h>
#include <os/logger.h>

#include <cerrno>
#include <cstring>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

// Android has no app-level way to raise the system boost, and on firmware without ADPF hint
// sessions the scheduler only sees utilisation. The frame's critical path then runs at low
// frequency, and short guest workers the main thread joins every frame (frustum culling,
// resource update, frame synchronisation) are placed on the little cores.
namespace
{
    // Linux UAPI struct sched_attr (SCHED_ATTR_SIZE_VER1) and flags, declared locally so the
    // build does not depend on the NDK sysroot carrying the utilisation clamp fields.
    struct SchedAttr
    {
        uint32_t size;
        uint32_t schedPolicy;
        uint64_t schedFlags;
        int32_t schedNice;
        uint32_t schedPriority;
        uint64_t schedRuntime;
        uint64_t schedDeadline;
        uint64_t schedPeriod;
        uint32_t schedUtilMin;
        uint32_t schedUtilMax;
    };

    constexpr uint64_t SchedFlagKeepPolicy = 0x08;
    constexpr uint64_t SchedFlagKeepParams = 0x10;
    constexpr uint64_t SchedFlagUtilClampMin = 0x20;

    const std::vector<std::string> DefaultWorkerPrefixes = { "FrustumCulling", "UpdateResource", "FrameSynchroniz" };

    struct UclampSettings
    {
        bool enabled = false;
        uint32_t main = 512;
        uint32_t render = 512;
        uint32_t worker = 512;
        std::vector<std::string> workerPrefixes = DefaultWorkerPrefixes;
    };

    struct AffinitySettings
    {
        bool enabled = false;
        cpu_set_t cpus{};
        std::string cpuList;
        std::vector<std::string> workerPrefixes = DefaultWorkerPrefixes;
    };

    std::string Trim(std::string_view value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos)
            return {};

        const auto last = value.find_last_not_of(" \t\r\n");
        return std::string(value.substr(first, last - first + 1));
    }

    std::vector<std::string> SplitList(const std::string& value)
    {
        std::vector<std::string> result;
        size_t start = 0;
        while (start <= value.size())
        {
            const auto comma = value.find(',', start);
            auto item = Trim(std::string_view(value).substr(start, comma == std::string::npos ? std::string::npos : comma - start));
            if (!item.empty())
                result.emplace_back(std::move(item));

            if (comma == std::string::npos)
                break;

            start = comma + 1;
        }

        return result;
    }

    std::string JoinList(const std::vector<std::string>& items)
    {
        std::string result;
        for (const auto& item : items)
            result += (result.empty() ? "" : ",") + item;

        return result;
    }

    // Reads "key=value" lines from a marker; returns false if the marker does not exist.
    template<typename TCallback>
    bool ReadMarker(const char* fileName, const TCallback& callback)
    {
        std::error_code error;
        const auto path = os::android::GetDataRoot() / fileName;
        if (!std::filesystem::exists(path, error))
            return false;

        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line))
        {
            const auto separator = line.find('=');
            if (separator != std::string::npos)
                callback(Trim(std::string_view(line).substr(0, separator)), Trim(std::string_view(line).substr(separator + 1)));
        }

        return true;
    }

    void ParseUtil(const std::string& key, const std::string& value, uint32_t& out)
    {
        char* end = nullptr;
        const unsigned long parsed = strtoul(value.c_str(), &end, 10);
        if (end == value.c_str() || *end != '\0' || parsed > 1024)
        {
            LOGFN_WARNING("thread_uclamp.enabled: ignoring {}={} (expected 0..1024).", key, value);
            return;
        }

        out = uint32_t(parsed);
    }

    const UclampSettings& GetUclampSettings()
    {
        static const UclampSettings settings = []
        {
            UclampSettings result;
            result.enabled = ReadMarker("thread_uclamp.enabled", [&](const std::string& key, const std::string& value)
            {
                if (key == "main")
                    ParseUtil(key, value, result.main);
                else if (key == "render")
                    ParseUtil(key, value, result.render);
                else if (key == "worker")
                    ParseUtil(key, value, result.worker);
                else if (key == "workers")
                    result.workerPrefixes = SplitList(value);
            });

            if (result.enabled)
                LOGFN("Thread uclamp enabled: main={} render={} worker={} workers={}.", result.main, result.render, result.worker, JoinList(result.workerPrefixes));

            return result;
        }();

        return settings;
    }

    // Parses "4-7" or "4,5,6,7" (ranges and single CPUs may be mixed).
    bool ParseCpuList(const std::string& value, cpu_set_t& out)
    {
        CPU_ZERO(&out);
        for (const auto& item : SplitList(value))
        {
            int first = 0;
            int last = 0;
            char dash = 0;
            const int fields = sscanf(item.c_str(), "%d%c%d", &first, &dash, &last);
            if (fields == 1)
                last = first;
            else if (fields != 3 || dash != '-')
                return false;

            if (first < 0 || last < first || last >= CPU_SETSIZE)
                return false;

            for (int cpu = first; cpu <= last; cpu++)
                CPU_SET(cpu, &out);
        }

        return CPU_COUNT(&out) > 0;
    }

    std::string FormatCpus(const cpu_set_t& cpus);

    // A core counts as little only when it is clearly weaker than the strongest one: below half
    // its capacity, like Cortex-A510/A520 next to A7xx/X cores. Mid cores stay usable.
    constexpr uint32_t LittleCapacityPercent = 50;

    // The workers share their cores with the main and render threads; with fewer cores than this
    // outside the little cluster (e.g. 2+6 budget SoCs) pinning would crowd them together.
    constexpr int MinWorkerCpus = 3;

    // Chooses the worker CPU set from the core topology, so one APK adapts to the SoC:
    // - little cores present and enough faster cores left: the workers use the faster cores
    //   (SM7475: capacities 271/827/1024 -> CPUs 4-7, measured 46-50 -> 60 FPS in the HUB);
    // - no little cores (Snapdragon 8 Elite 792/1024, Dimensity 9300): off, every core is fast
    //   and pinning to the strongest ones would only crowd the main and render threads;
    // - too few faster cores, or no cpu_capacity: off, the scheduler decides as before.
    // `decision` receives the reason, which is logged either way.
    bool ChooseWorkerCpus(cpu_set_t& out, std::string& decision)
    {
        std::vector<std::pair<int, uint32_t>> capacities;
        for (int cpu = 0; cpu < CPU_SETSIZE; cpu++)
        {
            std::ifstream file(fmt::format("/sys/devices/system/cpu/cpu{}/cpu_capacity", cpu));
            uint32_t capacity = 0;
            if (!(file >> capacity))
                break;

            capacities.emplace_back(cpu, capacity);
        }

        if (capacities.empty())
        {
            decision = "cpu_capacity is unavailable";
            return false;
        }

        uint32_t highest = 0;
        for (const auto& [cpu, capacity] : capacities)
            highest = std::max(highest, capacity);

        std::string topology;
        cpu_set_t little;
        CPU_ZERO(&little);
        CPU_ZERO(&out);
        for (const auto& [cpu, capacity] : capacities)
        {
            topology += fmt::format("{}{}:{}", topology.empty() ? "" : " ", cpu, capacity);
            if (uint64_t(capacity) * 100 < uint64_t(highest) * LittleCapacityPercent)
                CPU_SET(cpu, &little);
            else
                CPU_SET(cpu, &out);
        }

        if (CPU_COUNT(&little) == 0)
        {
            decision = fmt::format("no little cores (all at least {}% of the strongest; capacities {})", LittleCapacityPercent, topology);
            return false;
        }

        if (CPU_COUNT(&out) < MinWorkerCpus)
        {
            decision = fmt::format("only {} cores outside the little cores {} (need {}; capacities {})",
                CPU_COUNT(&out), FormatCpus(little), MinWorkerCpus, topology);
            return false;
        }

        decision = fmt::format("little cores {} below {}% of the strongest; capacities {}", FormatCpus(little), LittleCapacityPercent, topology);
        return true;
    }

    std::string FormatCpus(const cpu_set_t& cpus)
    {
        std::string result;
        for (int cpu = 0; cpu < CPU_SETSIZE; cpu++)
        {
            if (CPU_ISSET(cpu, &cpus))
                result += (result.empty() ? "" : ",") + std::to_string(cpu);
        }

        return result;
    }

    const AffinitySettings& GetAffinitySettings()
    {
        static const AffinitySettings settings = []
        {
            AffinitySettings result;

            // On by default: on a POCO F5 (SM7475) it took the HUB from 46-50 to 60 FPS without a
            // system boost, as the joined workers stopped running on the little cores.
            std::error_code error;
            if (std::filesystem::exists(os::android::GetDataRoot() / "thread_affinity.disabled", error))
            {
                LOGN("Thread affinity disabled by thread_affinity.disabled.");
                return result;
            }

            std::string cpuValue;
            ReadMarker("thread_affinity.enabled", [&](const std::string& key, const std::string& value)
            {
                if (key == "cpus")
                    cpuValue = value;
                else if (key == "workers")
                    result.workerPrefixes = SplitList(value);
            });

            if (!cpuValue.empty() && !ParseCpuList(cpuValue, result.cpus))
            {
                LOGFN_WARNING("thread_affinity.enabled: ignoring cpus={} (expected e.g. 4-7 or 4,5,6,7).", cpuValue);
                cpuValue.clear();
            }

            std::string decision = "cpus= from thread_affinity.enabled";
            if (cpuValue.empty() && !ChooseWorkerCpus(result.cpus, decision))
            {
                LOGFN("Thread affinity off: {}.", decision);
                return result;
            }

            result.enabled = true;

            result.cpuList = FormatCpus(result.cpus);
            LOGFN("Thread affinity enabled: cpus={} ({}) workers={}.", result.cpuList, decision, JoinList(result.workerPrefixes));
            return result;
        }();

        return settings;
    }

    // Threads named before the guest starts (the render thread is a static std::thread, started
    // while the library loads) cannot read the markers yet: GetDataRoot() needs SDL and JNI.
    // They are remembered by tid and handled when the main thread is ready.
    std::mutex g_pendingMutex;
    bool g_ready = false;
    std::vector<std::pair<pid_t, std::string>> g_pending;

    bool MatchesPrefix(std::string_view name, const std::vector<std::string>& prefixes)
    {
        for (const auto& prefix : prefixes)
        {
            if (name.starts_with(prefix))
                return true;
        }

        return false;
    }

    void ApplyUclamp(const char* role, pid_t tid, const char* name, uint32_t utilMin)
    {
        SchedAttr attr{};
        attr.size = sizeof(attr);
        attr.schedFlags = SchedFlagKeepPolicy | SchedFlagKeepParams | SchedFlagUtilClampMin;
        attr.schedUtilMin = utilMin;

        if (syscall(__NR_sched_setattr, tid, &attr, 0) != 0)
        {
            const int error = errno;
            LOGFN_WARNING("Thread uclamp: sched_setattr failed for {} \"{}\" (tid {}): errno {} ({}).", role, name, tid, error, strerror(error));
            return;
        }

        // Read back: the effective value can still be limited by the cgroup clamp.
        SchedAttr current{};
        if (syscall(__NR_sched_getattr, tid, &current, sizeof(current), 0) == 0)
            LOGFN("Thread uclamp: {} \"{}\" (tid {}) uclamp_min {} (read back {}, nice {}).", role, name, tid, utilMin, current.schedUtilMin, current.schedNice);
        else
            LOGFN("Thread uclamp: {} \"{}\" (tid {}) uclamp_min {} (read back failed, errno {}).", role, name, tid, utilMin, errno);
    }

    // Worker threads that got the mask, for re-applying it after returning to the foreground.
    std::mutex g_affinityThreadsMutex;
    std::vector<std::pair<pid_t, std::string>> g_affinityThreads;

    bool ApplyAffinity(const AffinitySettings& settings, pid_t tid, const char* name)
    {
        if (sched_setaffinity(tid, sizeof(settings.cpus), &settings.cpus) != 0)
        {
            const int error = errno;
            LOGFN_WARNING("Thread affinity: sched_setaffinity({}) failed for \"{}\" (tid {}): errno {} ({}).", settings.cpuList, name, tid, error, strerror(error));
            return false;
        }

        // Read back: the effective mask is also limited by the cpuset (e.g. cores paused by core_ctl).
        cpu_set_t current{};
        if (sched_getaffinity(tid, sizeof(current), &current) == 0)
            LOGFN("Thread affinity: \"{}\" (tid {}) cpus {} (read back {}).", name, tid, settings.cpuList, FormatCpus(current));
        else
            LOGFN("Thread affinity: \"{}\" (tid {}) cpus {} (read back failed, errno {}).", name, tid, settings.cpuList, errno);

        return true;
    }

    // A cpuset change while the app is in the background (top-app -> background -> top-app) may
    // reset the mask on some kernels. Restore it on return and log how many threads had lost it.
    // Each tid is checked against its thread name first, so a reused tid is never touched.
    void ReapplyAffinity()
    {
        const auto& settings = GetAffinitySettings();
        std::lock_guard lock(g_affinityThreadsMutex);

        int checked = 0;
        int lost = 0;
        for (const auto& [tid, name] : g_affinityThreads)
        {
            std::ifstream commFile(fmt::format("/proc/self/task/{}/comm", tid));
            std::string comm;
            if (!std::getline(commFile, comm) || comm != name.substr(0, 15))
                continue;

            ++checked;
            cpu_set_t current{};
            if (sched_getaffinity(tid, sizeof(current), &current) == 0 && CPU_EQUAL(&current, &settings.cpus))
                continue;

            ++lost;
            sched_setaffinity(tid, sizeof(settings.cpus), &settings.cpus);
        }

        LOGFN("Thread affinity: back in the foreground, {} of {} worker threads had lost cpus {} and were restored.",
            lost, checked, settings.cpuList);
    }

    bool SDLCALL OnSdlEvent(void*, SDL_Event* event)
    {
        if (event->type == SDL_EVENT_DID_ENTER_FOREGROUND)
            ReapplyAffinity();

        return true;
    }

    void ApplyByName(pid_t tid, const char* name)
    {
        const std::string_view view(name);

        const auto& uclamp = GetUclampSettings();
        if (uclamp.enabled)
        {
            if (view == "Render Thread")
                ApplyUclamp("render", tid, name, uclamp.render);
            else if (MatchesPrefix(view, uclamp.workerPrefixes))
                ApplyUclamp("worker", tid, name, uclamp.worker);
        }

        const auto& affinity = GetAffinitySettings();
        if (affinity.enabled && MatchesPrefix(view, affinity.workerPrefixes) && ApplyAffinity(affinity, tid, name))
        {
            std::lock_guard lock(g_affinityThreadsMutex);
            g_affinityThreads.emplace_back(tid, name);
        }
    }
}

void os::android::thread_hints::ApplyToMainThread()
{
    std::vector<std::pair<pid_t, std::string>> pending;
    {
        std::lock_guard lock(g_pendingMutex);
        g_ready = true;
        pending.swap(g_pending);
    }

    const auto& uclamp = GetUclampSettings();
    if (uclamp.enabled)
        ApplyUclamp("main", gettid(), "guest main", uclamp.main);

    for (const auto& [tid, name] : pending)
        ApplyByName(tid, name.c_str());

    if (GetAffinitySettings().enabled)
        SDL_AddEventWatch(OnSdlEvent, nullptr);
}

void os::android::thread_hints::ApplyToCurrentThread(const char* name)
{
    if (name == nullptr)
        return;

    {
        std::lock_guard lock(g_pendingMutex);
        if (!g_ready)
        {
            g_pending.emplace_back(gettid(), name);
            return;
        }
    }

    ApplyByName(gettid(), name);
}
