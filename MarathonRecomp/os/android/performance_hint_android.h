#pragma once

#include <cstdint>
#include <sys/types.h>

// Android Dynamic Performance Framework (APerformanceHint, Android 13+), on unless
// <game root>/performance_hint.disabled exists (a launcher switch). Once per frame the guest thread
// reports how long the frame's work took against the frame budget, and the system raises the
// clocks of the session's threads (on Android 15+ also of the GPU) when the work does not fit.
// Without it some governors (seen on a Pixel 7 Pro) keep the big cores at a third of their clock
// for this workload, whose CPU and GPU halves alternate and each look half idle.
namespace os::android::hint
{
    enum class Role
    {
        Guest,
        Render,
        Count
    };

    // Safe before the game root is known (the render thread starts during static initialisation).
    void SetThread(Role role, pid_t tid);

    // The registered thread, 0 when not known yet.
    pid_t GetThread(Role role);

    // From the presenting thread once per frame, in nanoseconds of CLOCK_MONOTONIC. totalNs excludes
    // waits for the display and the frame limiter, cpuNs also excludes waits for the GPU; gpuNs < 0
    // when unknown. The session is created on the first calls, once the render thread is known, and
    // covers the registered guest and render threads and the presenting thread; a later change of
    // the presenting thread is added to the session (Android 14+).
    void ReportFrame(int64_t startNs, int64_t totalNs, int64_t cpuNs, int64_t gpuNs, int32_t fpsCap);

    // Short state for the overlay and the [perf] log: "on", "on+gpu", "off", "n/a", "failed".
    const char* GetStatus();
}
