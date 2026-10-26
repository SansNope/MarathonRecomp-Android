#include <os/process.h>

#include <dlfcn.h>

std::filesystem::path os::process::GetExecutablePath()
{
    // /proc/self/exe is the zygote's app_process binary; the meaningful
    // "executable" of an Android app is this library inside its native lib dir.
    Dl_info info{};
    if (dladdr(reinterpret_cast<void*>(&os::process::GetExecutablePath), &info) != 0 && info.dli_fname != nullptr)
    {
        return std::filesystem::path(std::u8string_view((const char8_t*)(info.dli_fname)));
    }
    else
    {
        return std::filesystem::path();
    }
}

std::filesystem::path os::process::GetExecutableRoot()
{
    return GetExecutablePath().remove_filename();
}

std::filesystem::path os::process::GetWorkingDirectory()
{
    char cwd[PATH_MAX] = {};
    char *res = getcwd(cwd, sizeof(cwd));
    if (res != nullptr)
    {
        return std::filesystem::path(std::u8string_view((const char8_t*)(cwd)));
    }
    else
    {
        return std::filesystem::path();
    }
}

bool os::process::SetWorkingDirectory(const std::filesystem::path& path)
{
    return chdir(path.c_str()) == 0;
}

bool os::process::StartProcess(const std::filesystem::path& path, const std::vector<std::string>& args, std::filesystem::path work)
{
    // An app process must not fork/exec arbitrary binaries on Android.
    return false;
}

void os::process::CheckConsole()
{
    // No console on Android; logging goes to logcat and log.txt (see logger_android.cpp).
    g_consoleVisible = false;
}

void os::process::ShowConsole()
{
    // Unsupported on Android.
}
