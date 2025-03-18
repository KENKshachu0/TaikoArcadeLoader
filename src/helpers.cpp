#include <codecvt>
#include "helpers.h"

std::wstring
replace (const std::wstring orignStr, const std::wstring &oldStr, const std::wstring &newStr) {
    size_t pos                              = 0;
    std::wstring tempStr                    = orignStr;
    const std::wstring::size_type newStrLen = newStr.length ();
    const std::wstring::size_type oldStrLen = oldStr.length ();
    while (true) {
        pos = tempStr.find (oldStr, pos);
        if (pos == std::wstring::npos) break;

        tempStr.replace (pos, oldStrLen, newStr);
        pos += newStrLen;
    }

    return tempStr;
}

std::string
replace (const std::string orignStr, const std::string &oldStr, const std::string &newStr) {
    size_t pos                             = 0;
    std::string tempStr                    = orignStr;
    const std::string::size_type newStrLen = newStr.length ();
    const std::string::size_type oldStrLen = oldStr.length ();
    while (true) {
        pos = tempStr.find (oldStr, pos);
        if (pos == std::string::npos) break;

        tempStr.replace (pos, oldStrLen, newStr);
        pos += newStrLen;
    }

    return tempStr;
}

const char *
GameVersionToString (const GameVersion version) {
    switch (version) {
    case GameVersion::JPN00: return "JPN00";
    case GameVersion::JPN08: return "JPN08";
    case GameVersion::JPN39: return "JPN39";
    case GameVersion::CHN00: return "CHN00";
    default: return "UNKNOWN";
    }
}

const char *
languageStr (const int language) {
    switch (language) {
    case 1: return "en_us";
    case 2: return "cn_tw";
    case 3: return "kor";
    case 4: return "cn_cn";
    default: return "jpn";
    }
}

std::string
ConvertWideToUtf8 (const std::wstring &wstr) {
    if (wstr.empty ()) return {};

    // Determine the size of the resulting UTF-8 string
    const int utf8Size = WideCharToMultiByte (CP_UTF8, 0, wstr.c_str (), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size <= 0) {
        LogMessage (LogLevel::ERROR, "Failed to convert wide string to UTF-8");
        return {};
    }

    // Allocate buffer and perform the conversion
    std::string utf8Str (utf8Size, '\0'); // -1 to exclude null terminator
    WideCharToMultiByte (CP_UTF8, 0, wstr.c_str (), -1, data (utf8Str), utf8Size, nullptr, nullptr);

    return utf8Str;
}

bool
AreAllBytesZero (const u8 *array, const size_t offset, const size_t length) {
    for (size_t i = 0; i < length; ++i)
        if (array[offset + i] != 0x00) return false;
    return true;
}

void
withFile (const char *fileName, std::function<void (u8 *)> callback) {
    FILE *file = fopen (fileName, "rb");
    if (file == nullptr) return;

    fseek (file, 0, SEEK_END);
    long size = ftell (file);
    fseek (file, 0, SEEK_SET);

    u8 *buffer = (u8 *)malloc(size);
    fread (buffer, 1, size, file);

    callback (buffer);

    free (buffer);
    fclose (file);
}

std::string
season () {
    std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* tm = std::localtime(&time);
    int month = tm->tm_mon + 1;
    return std::to_string (tm->tm_year + 1900) + (
        (month >= 3 && month <= 5) ? "spr" :
        (month >= 6 && month <= 8) ? "sum" :
        (month >= 9 && month <= 11) ? "aut" : "win"
    );
}