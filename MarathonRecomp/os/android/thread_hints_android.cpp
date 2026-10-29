#include "thread_hints_android.h"
#include <os/android/storage_android.h>
#include <os/logger.h>

#include <atomic>
#include <cerrno>
#include <chrono>
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

    // The guest threads the main thread waits for every frame ([wakers] in perf10 on a POCO F5:
    // model, resource and particle updates, culling, frame synchronisation, physics, document work).
    const std::vector<std::string> DefaultWorkerPrefixes = { "FrustumCulling", "UpdateResource", "FrameSynchroniz",
        "ModelStandard", "ManageParticle", "HavokRunner", "DocMarathon" };

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
        // The guest main and render threads get the same mask. A frame-capped thread that waits
        // for the GPU or the pacing sleep looks lightly loaded, so the scheduler keeps it on the
        // little cores: Pixel 7 Pro (Tensor G2, capacities 158/763/1024) ran both on the A55s at
        // 0.9-1.2 GHz with the big cores idle, 36-41 FPS, and 45-59 FPS whenever they moved.
        bool mainAndRender = true;
        // The guest main thread's mask: `cpus`, or the strongest cores with
        // thread_affinity_main_strongest.enabled.
        cpu_set_t mainCpus{};
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

    // The cores sharing the highest cpu_capacity (the prime cluster).
    bool StrongestCpus(cpu_set_t& out)
    {
        std::vector<std::pair<int, uint32_t>> capacities;
        uint32_t highest = 0;
        for (int cpu = 0; cpu < CPU_SETSIZE; cpu++)
        {
            std::ifstream file(fmt::format("/sys/devices/system/cpu/cpu{}/cpu_capacity", cpu));
            uint32_t capacity = 0;
            if (!(file >> capacity))
                break;

            capacities.emplace_back(cpu, capacity);
            highest = std::max(highest, capacity);
        }

        CPU_ZERO(&out);
        for (const auto& [cpu, capacity] : capacities)
        {
            if (capacity == highest)
                CPU_SET(cpu, &out);
        }

        return CPU_COUNT(&out) > 0;
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
                else if (key == "main_render")
                    result.mainAndRender = value != "0";
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
            LOGFN("Thread affinity enabled: cpus={} ({}) workers={} main_render={}.", result.cpuList, decision, JoinList(result.workerPrefixes), result.mainAndRender ? 1 : 0);

            result.mainCpus = result.cpus;
            if (std::filesystem::exists(os::android::GetDataRoot() / "thread_affinity_main_strongest.enabled", error))
            {
                // A single strongest core (SM7475's prime core) is often paused by core_ctl; the
                // main thread would then lose its only core, so at least two are required.
                cpu_set_t strongest;
                cpu_set_t inside;
                if (!StrongestCpus(strongest))
                {
                    LOGN("Thread affinity: main thread on the strongest cores requested, but cpu_capacity is unavailable; keeping the common set.");
                }
                else
                {
                    CPU_AND(&inside, &strongest, &result.cpus);
                    if (!CPU_EQUAL(&inside, &strongest) || CPU_COUNT(&strongest) < 2)
                    {
                        LOGFN("Thread affinity: main thread on the strongest cores requested, but the strongest cores {} are fewer than 2 or outside cpus {}; keeping the common set.",
                            FormatCpus(strongest), result.cpuList);
                    }
                    else
                    {
                        result.mainCpus = strongest;
                        LOGFN("Thread affinity: main thread on the strongest cores {} (thread_affinity_main_strongest.enabled).", FormatCpus(strongest));
                    }
                }
            }
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

    // Threads that got a mask, for re-applying it after returning to the foreground.
    struct PinnedThread
    {
        pid_t tid;
        std::string name;
        const cpu_set_t* cpus;
    };

    std::mutex g_affinityThreadsMutex;
    std::vector<PinnedThread> g_affinityThreads;

    bool ApplyAffinity(const cpu_set_t& cpus, pid_t tid, const char* name)
    {
        const std::string cpuList = FormatCpus(cpus);
        if (sched_setaffinity(tid, sizeof(cpus), &cpus) != 0)
        {
            const int error = errno;
            LOGFN_WARNING("Thread affinity: sched_setaffinity({}) failed for \"{}\" (tid {}): errno {} ({}).", cpuList, name, tid, error, strerror(error));
            return false;
        }

        // Read back: the effective mask is also limited by the active cores (e.g. cores paused by core_ctl).
        cpu_set_t current{};
        if (sched_getaffinity(tid, sizeof(current), &current) == 0)
            LOGFN("Thread affinity: \"{}\" (tid {}) cpus {} (read back {}).", name, tid, cpuList, FormatCpus(current));
        else
            LOGFN("Thread affinity: \"{}\" (tid {}) cpus {} (read back failed, errno {}).", name, tid, cpuList, errno);

        return true;
    }

    // sched_getaffinity reports the mask limited to the active cores, so a core paused by core_ctl
    // (SM7475 pauses idle big cores) is missing from it; only cores outside the set mean the mask
    // was reset.
    bool HasCpusOutside(const cpu_set_t& current, const cpu_set_t& allowed)
    {
        for (int cpu = 0; cpu < CPU_SETSIZE; cpu++)
        {
            if (CPU_ISSET(cpu, &current) && !CPU_ISSET(cpu, &allowed))
                return true;
        }

        return false;
    }

    // A cpuset change (top-app -> background -> top-app) resets the mask of every thread moved, and
    // the move back to top-app can come after the activity's resume (seen on a Pixel 7 Pro: the
    // restore on resume found all threads at the background cores, and the render thread later ran
    // on a little core). Restores the mask of the pinned threads that lost it; a thread with a
    // changed mask is checked against its name first, so a reused tid is never touched.
    // `found` receives the mask of the first thread that had lost its own.
    int RestoreAffinity(int& checked, std::string& found)
    {
        std::lock_guard lock(g_affinityThreadsMutex);

        checked = 0;
        int lost = 0;
        for (const auto& thread : g_affinityThreads)
        {
            cpu_set_t current{};
            const bool read = sched_getaffinity(thread.tid, sizeof(current), &current) == 0;
            if (read && !HasCpusOutside(current, *thread.cpus))
            {
                ++checked;
                continue;
            }

            std::ifstream commFile(fmt::format("/proc/self/task/{}/comm", thread.tid));
            std::string comm;
            if (!std::getline(commFile, comm) || comm != thread.name.substr(0, 15))
                continue;

            ++checked;
            if (lost++ == 0)
                found = read ? FormatCpus(current) : std::string("unreadable");
            sched_setaffinity(thread.tid, sizeof(*thread.cpus), thread.cpus);
        }

        return lost;
    }

    std::atomic<int64_t> g_foregroundAtNs{ 0 };

    int64_t NowNs()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void ReapplyAffinity()
    {
        g_foregroundAtNs = NowNs();

        int checked = 0;
        std::string found;
        const int lost = RestoreAffinity(checked, found);
        LOGFN("Thread affinity: back in the foreground, {} of {} pinned threads had lost their cpus{} and were restored.",
            lost, checked, lost ? fmt::format(" (first found with {})", found) : std::string());
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
        const bool pinned = MatchesPrefix(view, affinity.workerPrefixes) || (affinity.mainAndRender && view == "Render Thread");
        if (affinity.enabled && pinned && ApplyAffinity(affinity.cpus, tid, name))
        {
            std::lock_guard lock(g_affinityThreadsMutex);
            g_affinityThreads.push_back({ tid, name, &affinity.cpus });
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

    const auto& affinity = GetAffinitySettings();
    if (affinity.enabled && affinity.mainAndRender)
    {
        // Registered under its kernel name, which ReapplyAffinity compares with.
        const pid_t tid = gettid();
        std::ifstream commFile(fmt::format("/proc/self/task/{}/comm", tid));
        std::string comm;
        std::getline(commFile, comm);
        if (ApplyAffinity(affinity.mainCpus, tid, comm.empty() ? "guest main" : comm.c_str()) && !comm.empty())
        {
            std::lock_guard lock(g_affinityThreadsMutex);
            g_affinityThreads.push_back({ tid, comm, &affinity.mainCpus });
        }
    }

    if (affinity.enabled)
        SDL_AddEventWatch(OnSdlEvent, nullptr);
}

void os::android::thread_hints::CheckAffinity()
{
    static int64_t s_nextCheckNs = 0;
    static uint32_t s_logged = 0;

    const auto& settings = GetAffinitySettings();
    if (!settings.enabled)
        return;

    const int64_t now = NowNs();
    if (now < s_nextCheckNs)
        return;
    s_nextCheckNs = now + 2000000000LL;

    int checked = 0;
    std::string found;
    const int lost = RestoreAffinity(checked, found);
    if (lost == 0 || s_logged >= 100)
        return;

    ++s_logged;
    const int64_t foregroundAt = g_foregroundAtNs.load();
    LOGFN("Thread affinity: {} of {} pinned threads had lost their cpus (first found with {}; {}) and were restored{}.",
        lost, checked, found,
        foregroundAt != 0 ? fmt::format("{:.1f} s after returning to the foreground", double(now - foregroundAt) / 1e9) : std::string("no return from the background yet"),
        s_logged == 100 ? "; further restores are not logged" : "");
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

std::string os::android::thread_hints::ThreadUclamp(pid_t tid)
{
    SchedAttr attr{};
    if (tid <= 0 || syscall(__NR_sched_getattr, tid, &attr, sizeof(attr), 0) != 0 || attr.size < sizeof(attr))
        return "n/a";

    return fmt::format("{}/{}", attr.schedUtilMin, attr.schedUtilMax);
}
