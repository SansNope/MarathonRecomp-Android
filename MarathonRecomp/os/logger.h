#pragma once

#include <source_location>

#define LOG_IMPL(type, func, str)       os::logger::Log(str, type, func)
#define LOGF_IMPL(type, func, str, ...) os::logger::Log(fmt::format(str, __VA_ARGS__), type, func)

// Utility lines (a "Loading file" per file request, stub calls, surface variants) are frequent
// enough to slow loading down on Android; IsUtilityEnabled() is checked before formatting.
#define LOG_UTILITY_IMPL(func, str)       do { if (os::logger::IsUtilityEnabled()) LOG_IMPL(os::logger::ELogType::Utility, func, str); } while (0)
#define LOGF_UTILITY_IMPL(func, str, ...) do { if (os::logger::IsUtilityEnabled()) LOGF_IMPL(os::logger::ELogType::Utility, func, str, __VA_ARGS__); } while (0)

// Function-specific logging.

#define LOG(str)               LOG_IMPL(os::logger::ELogType::Info, __func__, str)
#define LOG_WARNING(str)       LOG_IMPL(os::logger::ELogType::Warning, __func__, str)
#define LOG_ERROR(str)         LOG_IMPL(os::logger::ELogType::Error, __func__, str)

#if _DEBUG
#define LOG_UTILITY(str)       LOG_UTILITY_IMPL(__func__, str)
#else
#define LOG_UTILITY(str)       LOG_UTILITY_IMPL(__func__, str)
#endif

#define LOGF(str, ...)         LOGF_IMPL(os::logger::ELogType::Info, __func__, str, __VA_ARGS__)
#define LOGF_WARNING(str, ...) LOGF_IMPL(os::logger::ELogType::Warning, __func__, str, __VA_ARGS__)
#define LOGF_ERROR(str, ...)   LOGF_IMPL(os::logger::ELogType::Error, __func__, str, __VA_ARGS__)

#if _DEBUG
#define LOGF_UTILITY(str, ...) LOGF_UTILITY_IMPL(__func__, str, __VA_ARGS__)
#else
#define LOGF_UTILITY(str, ...) LOGF_UTILITY_IMPL(__func__, str, __VA_ARGS__)
#endif

// Non-function-specific logging.

#define LOGN(str)               LOG_IMPL(os::logger::ELogType::Info, "*", str)
#define LOGN_WARNING(str)       LOG_IMPL(os::logger::ELogType::Warning, "*", str)
#define LOGN_ERROR(str)         LOG_IMPL(os::logger::ELogType::Error, "*", str)

#if _DEBUG
#define LOGN_UTILITY(str)       LOG_UTILITY_IMPL("*", str)
#else
#define LOGN_UTILITY(str)       LOG_UTILITY_IMPL("*", str)
#endif

#define LOGFN(str, ...)         LOGF_IMPL(os::logger::ELogType::Info, "*", str, __VA_ARGS__)
#define LOGFN_WARNING(str, ...) LOGF_IMPL(os::logger::ELogType::Warning, "*", str, __VA_ARGS__)
#define LOGFN_ERROR(str, ...)   LOGF_IMPL(os::logger::ELogType::Error, "*", str, __VA_ARGS__)

#if _DEBUG
#define LOGFN_UTILITY(str, ...) LOGF_UTILITY_IMPL("*", str, __VA_ARGS__)
#else
#define LOGFN_UTILITY(str, ...) LOGF_UTILITY_IMPL("*", str, __VA_ARGS__)
#endif

namespace os::logger
{
    enum class ELogType
    {
        Info,
        Utility,
        Warning,
        Error
    };

    void Init();
    void Log(const std::string_view str, ELogType type = ELogType::Info, const char* func = nullptr);

#ifdef __ANDROID__
    // Off unless <game root>/log_utility.enabled exists (see os/android/logger_android.cpp).
    bool IsUtilityEnabled();
#else
    inline bool IsUtilityEnabled() { return true; }
#endif
}
