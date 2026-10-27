#pragma once

namespace os::android
{
    // The crash reporter in logger_android.cpp runs on an alternate signal stack, so a stack
    // overflow can still be written to log.txt. sigaltstack() is per thread: call this at the
    // start of every long-lived thread. Repeated calls on the same thread do nothing.
    void InstallCrashAltStackForCurrentThread();

    // Short description of what the current thread is doing (e.g. the pipeline being compiled),
    // printed by the crash reporter when a driver crashes inside that work. nullptr clears it.
    void SetCrashContext(const char* text);
}
