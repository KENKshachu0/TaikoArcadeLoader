#include <iostream>
#include <regex>
#include "logger.h"

static Logger *loggerInstance = nullptr;
void *consoleHandle           = nullptr;
std::mutex logMutex; // Mutex for thread-safe logging

char timeStr[64];
time_t rawTime;
tm *timeInfo;
SYSTEMTIME systemTime;

void
InitializeLogger (const LogLevel level, const bool logToFile, const std::string& logPath) {
    if (loggerInstance == nullptr) {
        loggerInstance = static_cast<Logger *> (malloc (sizeof (Logger)));
        if (consoleHandle == nullptr) consoleHandle = GetStdHandle (STD_OUTPUT_HANDLE);
    }

    loggerInstance->logLevel = level;

    if (logToFile) {
        loggerInstance->logFile = fopen (logPath.c_str (), "w"); // Open in write mode
        if (!loggerInstance->logFile) LogMessage (LogLevel::WARN, std::string ("Failed to open TaikoArcadeLoader.log for writing."));
    } else loggerInstance->logFile = nullptr; // No file logging
}

void
LogMessageHandler (const char *function, const char *codeFile, int codeLine, LogLevel messageLevel, const char *format, ...) {
    // Return if no logger or log level is too high
    if (loggerInstance == nullptr || messageLevel > loggerInstance->logLevel) return;

    // Lock for thread safety
    std::lock_guard lock (logMutex);

    // Format the user-provided message
    va_list args;
    va_start (args, format);
    int requiredSize = vsnprintf (nullptr, 0, format, args) + 1; // +1 for null terminator
    std::unique_ptr<char[]> buffer (new char[requiredSize]);     // Allocate buffer dynamically
    vsnprintf (buffer.get (), requiredSize, format, args);       // Format the string
    std::string formattedMessage (buffer.get ());                // Convert to std::string
    va_end (args);

    // Determine log type string
    std::string logType = GetLogLevelString (messageLevel);

    // Shrink function name by regex
    std::string short_function (function);
    std::regex re(R"(.*? (([\w<>]+::)*[\w]+( [()<>+-]+)?)\(\w+.*?\))");
    short_function = std::regex_replace (short_function, re, "$1");

    // Remove the absolute path of the build dir
    constexpr std::string_view build_dir = XSTRING (SOURCE_ROOT);
    std::string_view filename            = codeFile;
    filename.remove_prefix (build_dir.size ());

    // Get current time and milliseconds
    SYSTEMTIME systemTime;
    GetSystemTime (&systemTime);
    time_t rawTime = time (nullptr);
    tm *timeInfo   = localtime (&rawTime);
    std::ostringstream timeStamp;
    timeStamp << std::put_time (timeInfo, "%Y/%m/%d %H:%M:%S") << "." << std::setw (3) << std::setfill ('0') << systemTime.wMilliseconds;

    // Construct the log message
    std::ostringstream logStream;
    logStream << short_function << " (" << filename << ":" << codeLine << "): " << formattedMessage;
    std::string logMessage = logStream.str ();

    // Print the log message
    std::cout << "[" << timeStamp.str () << "] ";                                                                            // Timestamp
    SetConsoleTextAttribute (consoleHandle, GetLogLevelColor (messageLevel));                                                // Set Level color
    std::cout << logType;                                                                                                    // Level
    if (messageLevel != LogLevel::ERROR) {
        SetConsoleTextAttribute (consoleHandle, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Reset console color
    }
    std::cout << logMessage << std::endl;                                                                                    // Log message
    if (messageLevel == LogLevel::ERROR) {
        SetConsoleTextAttribute (consoleHandle, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Reset console color
    }
    std::cout.flush (); // Flush to ensure immediate writing

    if (loggerInstance->logFile != nullptr) {
        fprintf (loggerInstance->logFile, "[%s] %s%s\n", timeStamp.str ().c_str (), logType.c_str (), logMessage.c_str ());
        fflush (loggerInstance->logFile); // Flush to ensure immediate writing
    }
}

void
LogMessageHandler (const char *function, const char *codeFile, const int codeLine, const LogLevel messageLevel, const wchar_t *format, ...) {
    const std::string utf8Message = ConvertWideToUtf8 (format); // Convert wide string to UTF-8

    va_list args;
    va_start (args, format);
    LogMessageHandler (function, codeFile, codeLine, messageLevel, utf8Message.c_str (), args); // Delegate to the original handler
    va_end (args);
}

bool
EnableLogging (LogLevel messageLevel) {
    return loggerInstance != nullptr && messageLevel <= loggerInstance->logLevel;
}

void
CleanupLogger () {
    if (loggerInstance != nullptr) {
        if (loggerInstance->logFile) fclose (loggerInstance->logFile);
        free (loggerInstance);
        loggerInstance = nullptr;
    }
}
