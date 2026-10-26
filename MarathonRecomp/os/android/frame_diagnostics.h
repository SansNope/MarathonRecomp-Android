#pragma once

// Opt-in timing only; these calls do nothing without perf_capture.enabled.
namespace os::android::perf
{
    void BeginDocUpdate();
    void EndDocUpdate();
}
