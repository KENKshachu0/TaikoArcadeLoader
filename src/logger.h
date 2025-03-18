#pragma once

#include "helpers.h"
#include <source_location>
#include <string_view>
#include <format>
#include <fstream>
#include <wincon.h>
#include <string>
#include <mutex>

#ifdef ERROR
#undef ERROR
#endif

#define STRING(x)  #x
#define XSTRING(x) STRING (x)

enum class LogLevel {
    NONE = 0,
    ERROR,
    WARN,
    INFO,
    GAME,
    DEBUG,
    HOOKS
};

class Logger {
public:
    static void InitLoggerHook ();

    static void InitializeLogger (LogLevel level, bool logToFile, std::string logDir);

    static bool GuardianOutput (LogLevel messageLevel);

    static void LogMessageHandler (const char* function, const char* codeFile, int codeLine, LogLevel messageLevel, const char* format, ...);

    static void LogMessageHandler (const char* function, const char* codeFile, int codeLine, LogLevel messageLevel, const wchar_t* format, ...);

    static void CleanupLogger ();

    static LogLevel GetLogLevel (const std::string& logLevelStr);

    static std::string GetLogLevelString (LogLevel messageLevel);

    static int GetLogLevelColor (LogLevel messageLevel);

private:
    // Private constructor to disallow instantiation
    Logger() = default;

    static LogLevel s_logLevel;
    static std::fstream* s_logFile;
    static void* s_consoleHandle;
    static std::mutex s_logMutex;
    static bool s_isInitialized;
};

template <typename... Args>
struct LogMessage {
    LogMessage (const LogLevel level, const std::string_view format, Args&&... args,
               const std::source_location& loc = std::source_location::current()) {
        if (Logger::GuardianOutput (level)) return;
        std::string formatted_message =
            std::vformat(std::string(format), std::make_format_args(args...));

        Logger::LogMessageHandler(loc.function_name(), loc.file_name(), loc.line(), level, formatted_message.c_str());
    }

    LogMessage (const LogLevel level, const std::wstring_view format, Args&&... args,
               const std::source_location& loc = std::source_location::current()) {
        if (Logger::GuardianOutput (level)) return;
        std::wstring formatted_message =
            std::vformat(std::wstring(format), std::make_wformat_args(args...));

        Logger::LogMessageHandler(loc.function_name(), loc.file_name(), loc.line(), level, formatted_message.c_str());
    }
};

template <>
struct LogMessage<void> {
    LogMessage(const LogLevel level, const std::string_view format,
               const std::source_location& loc = std::source_location::current()) {
        if (Logger::GuardianOutput (level)) return;
        Logger::LogMessageHandler(loc.function_name(), loc.file_name(), loc.line(), level, format.data());
    }

    LogMessage(const LogLevel level, const std::wstring_view format,
               const std::source_location& loc = std::source_location::current()) {
        if (Logger::GuardianOutput (level)) return;
        Logger::LogMessageHandler(loc.function_name(), loc.file_name(), loc.line(), level, format.data());
    }
};

LogMessage(LogLevel level, std::string_view format) -> LogMessage<void>;
LogMessage(LogLevel level, std::wstring_view format) -> LogMessage<void>;

template <typename... Args>
LogMessage(LogLevel level, std::string_view format, Args&&... ts) -> LogMessage<Args...>;

template <typename... Args>
LogMessage(LogLevel level, std::wstring_view format, Args&&... ts) -> LogMessage<Args...>;
