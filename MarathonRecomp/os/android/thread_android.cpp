#include <os/thread.h>
#include <os/android/logger_android.h>
#include <os/android/thread_hints_android.h>
#include <pthread.h>

bool os::thread::SetCurrentThreadName(const char* name)
{
    // Bionic limits thread names to 15 characters, and will
    // reject any attempt to set a longer name with ERANGE.
    const auto truncated = std::string(name, std::min(strlen(name), size_t(15)));

    const bool result = pthread_setname_np(pthread_self(), truncated.c_str()) == 0;

    os::android::InstallCrashAltStackForCurrentThread();

    os::android::thread_hints::ApplyToCurrentThread(name);

    return result;
}
