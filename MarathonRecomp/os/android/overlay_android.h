#pragma once

#include <cstdint>
#include <sys/types.h>

// Performance overlay, turned on by <game root>/overlay.enabled (a launcher switch). A background
// thread samples twice a second what Android lets an app read: core frequencies, the CPU time of
// the game's threads and of the process, the thermal status, and, where the kernel exposes it to
// apps, the GPU busy percentage and the battery temperature. Frame and GPU times come from the
// renderer's own profilers; the panel itself is drawn in video.cpp.
namespace os::android::overlay
{
    enum class Role
    {
        Guest,
        Render,
        Count
    };

    struct Stats
    {
        static constexpr int MaxCpus = 16;

        int cpuCount = 0;
        uint32_t curMHz[MaxCpus]{};
        uint32_t maxMHz[MaxCpus]{};
        uint32_t capacity[MaxCpus]{};

        // Share of one core over the last interval and the core the thread last ran on.
        float threadLoad[int(Role::Count)]{};
        int threadCore[int(Role::Count)]{ -1, -1 };
        float processCores = 0.0f;

        int gpuBusyPercent = -1;      // -1: not readable on this device
        int thermalStatus = -2;       // AThermal status, -2: unavailable
        float thermalHeadroom = -1.0f; // < 0: unavailable
        float batteryCelsius = -1000.0f;
        double residentMB = 0.0;
        bool valid = false;
    };

    bool IsEnabled();

    // Safe before the game root is known (the render thread starts during static initialisation).
    void SetThread(Role role, pid_t tid);

    // Latest sample; the first call starts the sampling thread.
    Stats GetStats();
}
