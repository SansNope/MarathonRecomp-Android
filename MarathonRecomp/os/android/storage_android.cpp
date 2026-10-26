#include <os/android/storage_android.h>

#include <jni.h>

namespace os::android
{
    // A directory can exist but be unusable: e.g. created via `adb shell mkdir` it's owned
    // by the shell uid, and the app gets EACCES through FUSE. std::filesystem calls on such
    // paths throw all over the codebase (Config::Load etc.), so catch this case up front.
    static bool ProbeDirWritable(const std::filesystem::path& dir)
    {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        std::filesystem::path probePath = dir / ".write_probe";
        FILE* file = fopen(probePath.c_str(), "wb");
        if (file == nullptr)
            return false;

        fclose(file);
        remove(probePath.c_str());
        return true;
    }

    const std::filesystem::path& GetInternalFilesDir()
    {
        static std::filesystem::path path = []() -> std::filesystem::path
        {
            const char* storagePath = SDL_GetAndroidInternalStoragePath();
            return (storagePath != nullptr) ? std::filesystem::path(storagePath) : std::filesystem::path();
        }();

        return path;
    }

    const std::filesystem::path& GetExternalFilesDir()
    {
        static std::filesystem::path path = []() -> std::filesystem::path
        {
            const char* storagePath = SDL_GetAndroidExternalStoragePath();
            return (storagePath != nullptr) ? std::filesystem::path(storagePath) : std::filesystem::path();
        }();

        return path;
    }

    const std::filesystem::path& GetExternalMediaDir()
    {
        static std::filesystem::path path = []() -> std::filesystem::path
        {
            // SDL has no accessor for Android/media, so derive it from the external
            // files path: .../Android/data/<pkg>/files -> .../Android/media/<pkg>.
            const std::filesystem::path& external = GetExternalFilesDir();
            if (external.empty())
                return {};

            std::string str = external.string();
            const std::string marker = "/Android/data/";
            size_t markerPos = str.find(marker);
            if (markerPos == std::string::npos)
                return {};

            size_t pkgBegin = markerPos + marker.size();
            size_t pkgEnd = str.find('/', pkgBegin);
            std::string pkg = (pkgEnd == std::string::npos)
                ? str.substr(pkgBegin)
                : str.substr(pkgBegin, pkgEnd - pkgBegin);
            if (pkg.empty())
                return {};

            return std::filesystem::path(str.substr(0, markerPos)) / "Android" / "media" / pkg;
        }();

        return path;
    }

    // Optional launcher-provided storage choice; the method does not have to exist.
    static std::string QueryActivityStoragePath()
    {
        auto env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
        auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
        std::string selected;

        if (env && activity)
        {
            jclass activityClass = env->GetObjectClass(activity);
            jmethodID method = env->GetMethodID(activityClass, "getGameStoragePath", "()Ljava/lang/String;");

            if (method == nullptr)
            {
                env->ExceptionClear();
            }
            else
            {
                auto path = static_cast<jstring>(env->CallObjectMethod(activity, method));

                if (env->ExceptionCheck())
                {
                    env->ExceptionClear();
                }
                else if (path)
                {
                    const char* chars = env->GetStringUTFChars(path, nullptr);

                    if (chars)
                    {
                        selected = chars;
                        env->ReleaseStringUTFChars(path, chars);
                    }
                }

                if (path)
                    env->DeleteLocalRef(path);
            }

            env->DeleteLocalRef(activityClass);
        }

        if (env && activity)
            env->DeleteLocalRef(activity);

        return selected;
    }

    const std::filesystem::path& GetDataRoot()
    {
        static std::filesystem::path root = []() -> std::filesystem::path
        {
            std::filesystem::path selected = QueryActivityStoragePath();

            if (selected.empty() && !GetExternalFilesDir().empty())
                selected = GetExternalFilesDir() / "MarathonRecomp";

            if (selected.empty() || !ProbeDirWritable(selected))
            {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Marathon Recompiled",
                    "The game storage directory is unavailable or not writable.", nullptr);
                std::_Exit(1);
            }

            return selected;
        }();

        return root;
    }
}
