#include "logger.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <memory>

extern GameVersion gameVersion;

bool          s_logMethodName          = Config::ConfigManager::instance ().getLoggingConfig ().log_method_name;
bool          s_logFullSource          = Config::ConfigManager::instance ().getLoggingConfig ().log_full_source;

// Static member definitions
LogLevel      Logger::s_logLevel       = LogLevel::NONE;
void*         Logger::s_consoleHandle  = nullptr;
bool          Logger::s_isInitialized  = false;
std::fstream* Logger::s_logFile        = nullptr;
std::mutex    Logger::s_logMutex;

bool          loggerInited             = false;

// FAST_HOOK_DYNAMIC (int, TaikoPrintf, const char *format, ...) {
//     va_list args;
//     va_start (args, format);
//     int requiredSize = vsnprintf (nullptr, 0, format, args) + 1; // +1 for null terminator
//     std::unique_ptr<char[]> buffer (new char[requiredSize]);
//     vsnprintf (buffer.get (), requiredSize, format, args);
//     Logger::LogMessageHandler ("printf", "Taiko.exe", -1, LogLevel::GAME, buffer.get ());
//     va_end (args);
//     return requiredSize;
// }

// i64 coutVal = 0;
// FAST_HOOK_DYNAMIC (i64 *, TaikoPrint, i64 *a1, const char *msg) {
//     if ((i64) a1 != coutVal) originalTaikoPrint.call<i64 *> (a1, msg);
//     Logger::LogMessageHandler ("std::cout", "Taiko.exe", -1, LogLevel::GAME, msg);
//     return a1;
// }

void
Logger::InitLoggerHook () {
    if (loggerInited) return;
    loggerInited = true;
    // switch (gameVersion) {
    // case GameVersion::JPN00: case GameVersion::JPN08: case GameVersion::CHN00: default: break;
    // case GameVersion::JPN39: {
    //     coutVal = *(i64 *)ASLR (0x1408CCBF8);
    //     INSTALL_FAST_HOOK_DYNAMIC (TaikoPrintf, ASLR (0x1400D3EE0));
    //     INSTALL_FAST_HOOK_DYNAMIC (TaikoPrint, ASLR (0x14004B4F0));
    // } break;
    // }
}

static std::string
ConvertWideToUtf8(const wchar_t* wstr) {
    if (!wstr) return {};
    std::wstring ws (wstr);
    return {ws.begin (), ws.end ()};
}

void
Logger::InitializeLogger(const LogLevel level, const bool logToFile, std::string logDir) {
    if (s_isInitialized) return;
    s_isInitialized = true;

    s_logLevel = level;
    if (s_consoleHandle == nullptr) s_consoleHandle = GetStdHandle (STD_OUTPUT_HANDLE);

    if (logToFile) {
        try {
            const std::filesystem::path logsDir ("logs");
            if (!std::filesystem::exists (logsDir)) std::filesystem::create_directories (logsDir);
        } catch (const std::exception& e) {
            std::cerr << "[Logger] Failed to create logs directory: " << e.what () << std::endl;
        }

        // Construct a daily log filename
        const auto now   = std::chrono::system_clock::now ();
        const auto timeT = std::chrono::system_clock::to_time_t (now);
        std::tm localTm  = {};
        localtime_s (&localTm, &timeT);

        char dateFilename[256];
        // Example format: "TaikoArcadeLoader-YYYYMMDD.log"
        std::strftime (dateFilename, sizeof (dateFilename), "TaikoArcadeLoader-%Y%m%d.log", &localTm);

        if (!logDir.ends_with ("/")) logDir += "/";
        const std::string fullLogPath = logDir + dateFilename;
        s_logFile = new std::fstream (fullLogPath.c_str (), std::ios::out | std::ios::app);
        if (!s_logFile->is_open ()) {
            std::cerr << "[Logger] Failed to open " << fullLogPath << " for writing.\n";
            s_logFile = nullptr;
        }
    }
}

bool
Logger::GuardianOutput (LogLevel messageLevel) {
    return !s_isInitialized || messageLevel > s_logLevel;
}

void
Logger::LogMessageHandler (const char* function, const char* codeFile, int codeLine, LogLevel messageLevel, const char* format, ...) {
    // Return if logger uninitialized or messageLevel is more verbose than the set level
    if (!s_isInitialized || messageLevel > s_logLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock (s_logMutex);

    va_list args;
    va_start (args, format);
    int requiredSize = vsnprintf (nullptr, 0, format, args) + 1; // +1 for null terminator
    std::unique_ptr<char[]> buffer (new char[requiredSize]);
    vsnprintf (buffer.get (), requiredSize, format, args);
    std::string formattedMessage (buffer.get());
    while (formattedMessage.ends_with ("\n")) {
        formattedMessage = formattedMessage.substr (0, formattedMessage.size () - 1);
    }
    va_end (args);

    std::string logType = Logger::GetLogLevelString(messageLevel);

    std::string short_function (function);
    std::regex re (R"(.*? (([\w<>]+::)*[\w]+( [()<>+-]+)?)\(\w+.*?\))");
    short_function = std::regex_replace (short_function, re, "$1");

    SYSTEMTIME systemTime;
    GetSystemTime (&systemTime);
    time_t rawTime = time (nullptr);
    tm* timeInfo   = localtime (&rawTime);

    std::ostringstream timeStamp;
    timeStamp << std::put_time (timeInfo, "%Y-%m-%d %H:%M:%S") << "."
              << std::setw (3) << std::setfill ('0') << systemTime.wMilliseconds;

    // Construct the log message
    std::ostringstream descStream;
    if (s_logMethodName) descStream << short_function << " ";
    if (s_logFullSource) {
        constexpr std::string_view build_dir = XSTRING (SOURCE_ROOT);
        std::string_view filename = codeFile;
        filename.remove_prefix (build_dir.size ());
        descStream << "(" << filename << ":" << codeLine << "): ";
    } else {
        std::string filename = std::string (codeFile);
        if (size_t begin = filename.find_last_of ("/\\"); begin != std::string::npos) {
            filename = filename.substr (begin + 1, filename.length ());
        }
        descStream << "(" << std::format("{: >20}", std::format("{}:{}", filename, codeLine)) << "): ";
    }
    std::string descMessage = descStream.str();

    // Print to console
    std::cout << "[" << timeStamp.str() << "] ";
    SetConsoleTextAttribute (s_consoleHandle, Logger::GetLogLevelColor(messageLevel));
    std::cout << logType;
    SetConsoleTextAttribute (s_consoleHandle, FOREGROUND_INTENSITY);
    std::cout << descMessage;
    if (messageLevel == LogLevel::WARN || messageLevel == LogLevel::ERROR) {
        SetConsoleTextAttribute (s_consoleHandle, Logger::GetLogLevelColor(messageLevel));
        std::cout << formattedMessage << std::endl;
        SetConsoleTextAttribute (s_consoleHandle, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    } else {
        SetConsoleTextAttribute (s_consoleHandle, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        std::cout << formattedMessage << std::endl;
    }
    std::cout.flush ();

    if (s_logFile != nullptr) {
        *s_logFile << "[" << timeStamp.str () << "] " << logType << descMessage << formattedMessage << std::endl;
        s_logFile->flush ();
    }
}

void
Logger::LogMessageHandler (const char* function, const char* codeFile, int codeLine, LogLevel messageLevel, const wchar_t* format, ...) {
	const std::string utf8Message = ConvertWideToUtf8 (format);

    va_list args;
    va_start (args, format);
    Logger::LogMessageHandler (function, codeFile, codeLine, messageLevel, utf8Message.c_str(), args);
    va_end (args);
}

void
Logger::CleanupLogger () {
    if (s_isInitialized) {
        if (s_logFile) {
            s_logFile->close ();
            s_logFile = nullptr;
        }
        s_isInitialized = false;
    }
}

LogLevel
Logger::GetLogLevel (const std::string& logLevelStr) {
    if (logLevelStr == "DEBUG") return LogLevel::DEBUG;
	if (logLevelStr == "INFO")  return LogLevel::INFO;
	if (logLevelStr == "WARN")  return LogLevel::WARN;
	if (logLevelStr == "ERROR") return LogLevel::ERROR;
    if (logLevelStr == "GAME")  return LogLevel::GAME;
	if (logLevelStr == "HOOKS") return LogLevel::HOOKS;
	return LogLevel::NONE;
}

std::string
Logger::GetLogLevelString (const LogLevel messageLevel) {
    switch (messageLevel) {
        case LogLevel::DEBUG: return "DEBUG: ";
        case LogLevel::INFO:  return "INFO : ";
        case LogLevel::WARN:  return "WARN : ";
        case LogLevel::ERROR: return "ERROR: ";
        case LogLevel::HOOKS: return "HOOKS: ";
        case LogLevel::GAME:  return "GAME : ";
        default:              return "NONE : ";
    }
}

int
Logger::GetLogLevelColor(const LogLevel messageLevel) {
    // Colors: https://i.sstatic.net/ZG625.png
    switch (messageLevel) {
        case LogLevel::DEBUG:
            return FOREGROUND_BLUE | FOREGROUND_INTENSITY;                  // Pale Blue
        case LogLevel::INFO:
            return FOREGROUND_GREEN | FOREGROUND_INTENSITY;                 // Pale Green
        case LogLevel::WARN:
            return FOREGROUND_RED | FOREGROUND_GREEN;                       // Bright Yellow
        case LogLevel::ERROR:
            return FOREGROUND_RED;                                          // Bright RED
        case LogLevel::HOOKS:
            return FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY; // Pale Purple
        case LogLevel::GAME:
            return FOREGROUND_BLUE | FOREGROUND_RED;                        // Purple
        default:
            return FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    }
}
