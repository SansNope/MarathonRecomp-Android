#pragma once

namespace os::android
{
    // The crash reporter in logger_android.cpp runs on an alternate signal stack, so a stack
    // overflow can still be written to log.txt. sigaltstack() is per thread: call this at the
    // start of every long-lived thread. Repeated calls on the same thread do nothing.
    void InstallCrashAltStackForCurrentThread();
}
