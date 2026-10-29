#include "performance_hint_android.h"
#include <os/android/storage_android.h>
#include <os/logger.h>
#include <user/config.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <string>
#include <unistd.h>

namespace
{
    enum class State
    {
        Starting,
        Active,
        ActiveWithGpu,
        Disabled,
        Unavailable,
        Failed
    };

    std::atomic<pid_t> s_threads[int(os::android::hint::Role::Count)]{};
    std::atomic<State> s_state{ State::Starting };

    // Resolved with dlsym: libmain.so targets API 29, the functions appeared in API 33 (session)
    // and API 35 (AWorkDuration with the GPU time).
    struct Api
    {
        void* (*getManager)() = nullptr;
        void* (*createSession)(void*, const int32_t*, size_t, int64_t) = nullptr;
        int64_t (*getPreferredUpdateRate)(void*) = nullptr;
        int (*updateTarget)(void*, int64_t) = nullptr;
        int (*report)(void*, int64_t) = nullptr;
        void* (*workCreate)() = nullptr;
        void (*workSetStart)(void*, int64_t) = nullptr;
        void (*workSetTotal)(void*, int64_t) = nullptr;
        void (*workSetCpu)(void*, int64_t) = nullptr;
        void (*workSetGpu)(void*, int64_t) = nullptr;
        int (*report2)(void*, void*) = nullptr;
        int (*setThreads)(void*, const int32_t*, size_t) = nullptr;
    };

    template<typename T>
    void Resolve(void* library, T& function, const char* name)
    {
        function = reinterpret_cast<T>(dlsym(library, name));
    }

    // Unlimited (or out of range) caps aim at 120 FPS.
    int64_t TargetFor(int32_t fpsCap)
    {
        const int32_t fps = (fpsCap >= FPS_MIN && fpsCap < FPS_MAX) ? fpsCap : 120;
        return 1000000000LL / fps;
    }

    // The threads a session is created with: the guest main thread, the thread calling Present and
    // the render thread, without duplicates. The first frames can be presented by another guest
    // thread than the one running the game (seen on a Pixel 7 Pro), so the registered guest thread
    // is always included.
    constexpr size_t MaxSessionThreads = 4;

    size_t CollectThreads(int32_t (&threads)[MaxSessionThreads], pid_t caller)
    {
        size_t count = 0;
        auto add = [&](pid_t tid)
        {
            if (tid <= 0)
                return;
            for (size_t i = 0; i < count; i++)
            {
                if (threads[i] == tid)
                    return;
            }
            threads[count++] = int32_t(tid);
        };
        add(s_threads[int(os::android::hint::Role::Guest)].load());
        add(caller);
        add(s_threads[int(os::android::hint::Role::Render)].load());
        return count;
    }

    bool ThreadAlive(int32_t tid)
    {
        return tid > 0 && access(fmt::format("/proc/self/task/{}", tid).c_str(), F_OK) == 0;
    }

    std::string FormatThreads(const int32_t* threads, size_t count)
    {
        std::string text;
        for (size_t i = 0; i < count; i++)
        {
            const char* role = threads[i] == s_threads[int(os::android::hint::Role::Guest)].load() ? "guest"
                : threads[i] == s_threads[int(os::android::hint::Role::Render)].load() ? "render" : "presenting";
            text += fmt::format("{}{} {}", i ? ", " : "", role, threads[i]);
        }
        return text;
    }

    struct Session
    {
        Api api;
        void* session = nullptr;
        void* work = nullptr;
        int64_t target = 0;
        uint32_t framesWaited = 0;
        uint32_t failures = 0;
        int32_t threads[MaxSessionThreads]{};
        size_t threadCount = 0;
        pid_t lastCaller = 0;
        bool threadsFixed = false;

        State Start(int32_t fpsCap)
        {
            std::error_code error;
            if (std::filesystem::exists(os::android::GetDataRoot() / "performance_hint.disabled", error))
            {
                LOGN("ADPF: disabled by performance_hint.disabled.");
                return State::Disabled;
            }

            void* library = dlopen("libandroid.so", RTLD_NOW);
            if (library != nullptr)
            {
                Resolve(library, api.getManager, "APerformanceHint_getManager");
                Resolve(library, api.createSession, "APerformanceHint_createSession");
                Resolve(library, api.getPreferredUpdateRate, "APerformanceHint_getPreferredUpdateRateNanos");
                Resolve(library, api.updateTarget, "APerformanceHint_updateTargetWorkDuration");
                Resolve(library, api.report, "APerformanceHint_reportActualWorkDuration");
                Resolve(library, api.workCreate, "AWorkDuration_create");
                Resolve(library, api.workSetStart, "AWorkDuration_setWorkPeriodStartTimestampNanos");
                Resolve(library, api.workSetTotal, "AWorkDuration_setActualTotalDurationNanos");
                Resolve(library, api.workSetCpu, "AWorkDuration_setActualCpuDurationNanos");
                Resolve(library, api.workSetGpu, "AWorkDuration_setActualGpuDurationNanos");
                Resolve(library, api.report2, "APerformanceHint_reportActualWorkDuration2");
                Resolve(library, api.setThreads, "APerformanceHint_setThreads");
            }

            if (!api.getManager || !api.createSession || !api.updateTarget || !api.report)
            {
                LOGN("ADPF: not available (needs Android 13).");
                return State::Unavailable;
            }

            void* manager = api.getManager();
            if (manager == nullptr)
            {
                LOGN("ADPF: not available (no performance hint manager).");
                return State::Unavailable;
            }

            const pid_t caller = gettid();
            lastCaller = caller;
            threadCount = CollectThreads(threads, caller);
            const double preferredRate = api.getPreferredUpdateRate ? double(api.getPreferredUpdateRate(manager)) / 1e6 : -1.0;

            target = TargetFor(fpsCap);
            session = api.createSession(manager, threads, threadCount, target);
            if (session == nullptr && threadCount > 1)
            {
                // The guest main thread carries the frame's critical path; the presenting thread
                // stands in only when no guest thread is registered.
                const pid_t guest = s_threads[int(os::android::hint::Role::Guest)].load();
                LOGFN("ADPF: createSession returned no session for threads {} (errno {}); retrying with the {} thread only.",
                    FormatThreads(threads, threadCount), errno, guest > 0 ? "guest" : "presenting");
                threads[0] = int32_t(guest > 0 ? guest : caller);
                threadCount = 1;
                session = api.createSession(manager, threads, threadCount, target);
                threadsFixed = true;
            }
            if (session == nullptr)
            {
                LOGFN("ADPF: not supported on this device (createSession returned no session; errno {}, preferred update rate {:.1f} ms).",
                    errno, preferredRate);
                return State::Unavailable;
            }

            const bool gpu = api.workCreate && api.workSetStart && api.workSetTotal && api.workSetCpu && api.workSetGpu && api.report2;
            if (gpu)
                work = api.workCreate();

            LOGFN("ADPF: session for threads {}, target {:.2f} ms, GPU time {}, preferred update rate {:.1f} ms.",
                FormatThreads(threads, threadCount), double(target) / 1e6,
                work ? "reported" : "not reported (needs Android 15)", preferredRate);

            return work ? State::ActiveWithGpu : State::Active;
        }

        // A new presenting thread joins the session (Android 14+); threads are only added, so
        // presenting threads that take turns do not change the session every frame. Checked once
        // per frame against the last caller, so a steady presenting thread costs one comparison.
        void UpdateThreads()
        {
            const pid_t caller = gettid();
            if (caller == lastCaller || threadsFixed)
                return;
            lastCaller = caller;

            if (std::find(threads, threads + threadCount, int32_t(caller)) != threads + threadCount)
                return;

            if (threadCount == MaxSessionThreads)
            {
                LOGFN("ADPF: frames now come from thread {}, the session is full.", caller);
                threadsFixed = true;
                return;
            }

            int32_t wanted[MaxSessionThreads]{};
            std::copy(threads, threads + threadCount, wanted);
            size_t wantedCount = threadCount + 1;
            wanted[threadCount] = int32_t(caller);

            if (api.setThreads == nullptr)
            {
                LOGFN("ADPF: frames now come from thread {}, which the session cannot take (needs Android 14).", caller);
                threadsFixed = true;
                return;
            }

            int result = api.setThreads(session, wanted, wantedCount);
            if (result != 0)
            {
                // The list replaces the session's threads, so one exited thread may fail the whole
                // call; log which are alive and retry once without the exited ones.
                std::string liveness;
                int32_t alive[MaxSessionThreads]{};
                size_t aliveCount = 0;
                for (size_t i = 0; i < wantedCount; i++)
                {
                    const bool isAlive = ThreadAlive(wanted[i]);
                    liveness += fmt::format("{}{} {}", i ? ", " : "", wanted[i], isAlive ? "alive" : "exited");
                    if (isAlive)
                        alive[aliveCount++] = wanted[i];
                }

                LOGFN_WARNING("ADPF: setting the session threads to {} failed: {} ({}); threads: {}.",
                    FormatThreads(wanted, wantedCount), result, strerror(result), liveness);

                if (aliveCount != 0 && aliveCount < wantedCount)
                {
                    result = api.setThreads(session, alive, aliveCount);
                    if (result == 0)
                    {
                        std::copy(alive, alive + aliveCount, wanted);
                        wantedCount = aliveCount;
                    }
                    else
                    {
                        LOGFN_WARNING("ADPF: retrying with the alive threads {} failed: {} ({}).",
                            FormatThreads(alive, aliveCount), result, strerror(result));
                    }
                }
            }

            if (result == 0)
            {
                std::copy(wanted, wanted + wantedCount, threads);
                threadCount = wantedCount;
                LOGFN("ADPF: session threads now {}.", FormatThreads(threads, threadCount));
            }
            else
            {
                threadsFixed = true;
            }
        }

        void Report(int64_t startNs, int64_t totalNs, int64_t cpuNs, int64_t gpuNs, int32_t fpsCap)
        {
            UpdateThreads();

            const int64_t newTarget = TargetFor(fpsCap);
            if (newTarget != target)
            {
                const int result = api.updateTarget(session, newTarget);
                if (result == 0)
                    LOGFN("ADPF: target {:.2f} ms (FPS cap {}).", double(newTarget) / 1e6, fpsCap);
                else
                    LOGFN_WARNING("ADPF: updating the target failed: {} ({}).", result, strerror(result));
                target = newTarget;
            }

            // Longer frames are loads or the return from the background, not work worth boosting for.
            if (totalNs <= 0 || totalNs > 1000000000LL)
                return;

            int result;
            if (work != nullptr)
            {
                // Both parts may not be zero; the GPU time belongs to an earlier frame of the queue.
                cpuNs = std::clamp<int64_t>(cpuNs, 1, totalNs);
                api.workSetStart(work, std::max<int64_t>(startNs, 1));
                api.workSetTotal(work, totalNs);
                api.workSetCpu(work, cpuNs);
                api.workSetGpu(work, std::max<int64_t>(gpuNs, 0));
                result = api.report2(session, work);
            }
            else
            {
                result = api.report(session, totalNs);
            }

            if (result == 0)
            {
                failures = 0;
                return;
            }

            if (failures == 0)
                LOGFN_WARNING("ADPF: reporting the frame failed: {} ({}).", result, strerror(result));

            // A dead session (EPIPE: the service went away) stays dead; stop after a run of errors.
            if (++failures >= 100)
            {
                LOGN_WARNING("ADPF: stopped after 100 failed reports in a row.");
                s_state = State::Failed;
            }
        }
    };

    Session s_session;
}

void os::android::hint::SetThread(Role role, pid_t tid)
{
    s_threads[int(role)].store(tid);
}

pid_t os::android::hint::GetThread(Role role)
{
    return s_threads[int(role)].load();
}

void os::android::hint::ReportFrame(int64_t startNs, int64_t totalNs, int64_t cpuNs, int64_t gpuNs, int32_t fpsCap)
{
    State state = s_state.load(std::memory_order_relaxed);

    if (state == State::Starting)
    {
        // The render thread names itself on its first command; give it a few seconds of frames.
        if (s_threads[int(Role::Render)].load() <= 0 && ++s_session.framesWaited < 300)
            return;

        state = s_session.Start(fpsCap);
        s_state = state;
    }

    if (state == State::Active || state == State::ActiveWithGpu)
        s_session.Report(startNs, totalNs, cpuNs, gpuNs, fpsCap);
}

const char* os::android::hint::GetStatus()
{
    switch (s_state.load(std::memory_order_relaxed))
    {
        case State::Active:        return "on";
        case State::ActiveWithGpu: return "on+gpu";
        case State::Disabled:      return "off";
        case State::Unavailable:   return "n/a";
        case State::Failed:        return "failed";
        case State::Starting:
        default:                   return "starting";
    }
}
