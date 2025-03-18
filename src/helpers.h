#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
// ReSharper disable once CppUnusedIncludeDirective
#include <safetyhook.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <MinHook.h>
#include <string>
#include <windows.h>
#include <functional>
#include "constants.h"
#include "logger.h"

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define FUNCTION_PTR(returnType, function, location, ...) returnType (*function) (__VA_ARGS__) = (returnType (*) (__VA_ARGS__)) (location)
#define FUNCTION_PTR_H(returnType, function, ...)         extern returnType (*function) (__VA_ARGS__)

#define PROC_ADDRESS(libraryName, procName)      GetProcAddress (LoadLibrary (TEXT (libraryName)), procName)
#define PROC_ADDRESS_OFFSET(libraryName, offset) (u64) ((u64)GetModuleHandle (TEXT (libraryName)) + offset)

#define BASE_ADDRESS 0x140000000
#ifdef BASE_ADDRESS
const HMODULE MODULE_HANDLE = GetModuleHandle (nullptr);
#define ASLR(address) ((u64)MODULE_HANDLE + (u64)address - (u64)BASE_ADDRESS)
#endif

#define HOOK(returnType, functionName, location, ...)         \
    typedef returnType (*functionName) (__VA_ARGS__);         \
    functionName original##functionName = nullptr;            \
    void *where##functionName           = (void *)(location); \
    returnType implOf##functionName (__VA_ARGS__)

#define HOOK_DYNAMIC(returnType, functionName, ...)   \
    typedef returnType (*functionName) (__VA_ARGS__); \
    functionName original##functionName = nullptr;    \
    void *where##functionName           = nullptr;    \
    returnType implOf##functionName (__VA_ARGS__)

#define VTABLE_HOOK(returnType, className, functionName, ...)                      \
    typedef returnType (*className##functionName) (className * This, __VA_ARGS__); \
    className##functionName original##className##functionName = nullptr;           \
    void *where##className##functionName                      = nullptr;           \
    returnType implOf##className##functionName (className *This, __VA_ARGS__)

#define MID_HOOK(functionName, location, ...)   \
    typedef void (*functionName) (__VA_ARGS__); \
    SafetyHookMid midHook##functionName{};      \
    u64 where##functionName = (location);       \
    void implOf##functionName (SafetyHookContext &ctx)

#define MID_HOOK_DYNAMIC(functionName, ...)           \
    typedef void (*functionName) (__VA_ARGS__);       \
    std::map<u64, SafetyHookMid> mapOf##functionName; \
    void implOf##functionName (SafetyHookContext &ctx)

#define FAST_HOOK(returnType, functionName, location, ...) \
    typedef returnType (*functionName) (__VA_ARGS__);      \
    SafetyHookInline original##functionName;               \
    void *where##functionName = (void *)(location);        \
    returnType implOf##functionName (__VA_ARGS__)

#define FAST_HOOK_DYNAMIC(returnType, functionName, ...)   \
    typedef returnType (*functionName) (__VA_ARGS__);      \
    SafetyHookInline original##functionName;               \
    returnType implOf##functionName (__VA_ARGS__)

#define INSTALL_HOOK(functionName)                                                                                     \
    {                                                                                                                  \
        LogMessage (LogLevel::DEBUG, std::string ("Installing hook for ") + #functionName);                            \
        MH_Initialize ();                                                                                              \
        MH_CreateHook ((void *)where##functionName, (void *)implOf##functionName, (void **)(&original##functionName)); \
        MH_EnableHook ((void *)where##functionName);                                                                   \
    }

#define INSTALL_HOOK_DYNAMIC(functionName, location) \
    {                                                \
        where##functionName = (void *)location;      \
        INSTALL_HOOK (functionName);                 \
    }

#define INSTALL_HOOK_DIRECT(location, locationOfHook)                                          \
    {                                                                                          \
        LogMessage (LogLevel::DEBUG, std::string ("Installing direct hook for ") + #location); \
        MH_Initialize ();                                                                      \
        MH_CreateHook ((void *)(location), (void *)(locationOfHook), NULL);                    \
        MH_EnableHook ((void *)(location));                                                    \
    }

#define INSTALL_VTABLE_HOOK(className, object, functionName, functionIndex)                     \
    {                                                                                           \
        where##className##functionName = (*(className##functionName ***)object)[functionIndex]; \
        INSTALL_HOOK (className##functionName);                                                 \
    }

#define INSTALL_MID_HOOK(functionName)                                                              \
    {                                                                                               \
        LogMessage (LogLevel::DEBUG, std::string ("Installing mid hook for ") + #functionName);     \
        midHook##functionName = safetyhook::create_mid (where##functionName, implOf##functionName); \
    }

#define INSTALL_MID_HOOK_DYNAMIC(functionName, location) \
    { mapOf##functionName[location] = safetyhook::create_mid (location, implOf##functionName); }

#define INSTALL_FAST_HOOK(functionName)                                                                        \
    {                                                                                                          \
        LogMessage (LogLevel::DEBUG, std::string ("Installing fast hook for ") + #functionName);               \
        original##functionName = safetyhook::create_inline (where##functionName, (void*)implOf##functionName); \
    }

#define INSTALL_FAST_HOOK_DYNAMIC(functionName, location)                                                      \
    {                                                                                                          \
        LogMessage (LogLevel::DEBUG, std::string ("Installing fast hook dynamic for ") + #functionName);       \
        original##functionName = safetyhook::create_inline ((void *)location, (void*)implOf##functionName);    \
    }

#define READ_MEMORY(location, type) *(type *)location

#define WRITE_MEMORY(location, type, ...)                                                        \
    {                                                                                            \
        const type data[] = {__VA_ARGS__};                                                       \
        DWORD oldProtect;                                                                        \
        VirtualProtect ((void *)(location), sizeof (data), PAGE_EXECUTE_READWRITE, &oldProtect); \
        memcpy ((void *)(location), data, sizeof (data));                                        \
        VirtualProtect ((void *)(location), sizeof (data), oldProtect, &oldProtect);             \
    }

#define WRITE_MEMORY_STRING(location, data, length)                                       \
    {                                                                                     \
        DWORD oldProtect;                                                                 \
        VirtualProtect ((void *)(location), length, PAGE_EXECUTE_READWRITE, &oldProtect); \
        memcpy ((void *)(location), data, length);                                        \
        VirtualProtect ((void *)(location), length, oldProtect, &oldProtect);             \
    }

#define WRITE_NOP(location, count)                                                                 \
    {                                                                                              \
        DWORD oldProtect;                                                                          \
        VirtualProtect ((void *)(location), (size_t)(count), PAGE_EXECUTE_READWRITE, &oldProtect); \
        for (size_t i = 0; i < (size_t)(count); i++)                                               \
            *((u8 *)(location) + i) = 0x90;                                                        \
        VirtualProtect ((void *)(location), (size_t)(count), oldProtect, &oldProtect);             \
    }

#define WRITE_NULL(location, count)                                                                \
    {                                                                                              \
        DWORD oldProtect;                                                                          \
        VirtualProtect ((void *)(location), (size_t)(count), PAGE_EXECUTE_READWRITE, &oldProtect); \
        for (size_t i = 0; i < (size_t)(count); i++)                                               \
            *((u8 *)(location) + i) = 0x00;                                                        \
        VirtualProtect ((void *)(location), (size_t)(count), oldProtect, &oldProtect);             \
    }

#define round(num) ((num > 0) ? (int)(num + 0.5) : (int)(num - 0.5))
#define timestamp() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now ().time_since_epoch ()).count ())

std::wstring replace (const std::wstring orignStr, const std::wstring &oldStr, const std::wstring &newStr);
std::string replace (const std::string orignStr, const std::string &oldStr, const std::string &newStr);
const char *GameVersionToString (GameVersion version);
const char *languageStr (int language);
std::string ConvertWideToUtf8 (const std::wstring &wstr);
bool AreAllBytesZero (const u8 *array, size_t offset, size_t length);
void withFile (const char *fileName, std::function<void (u8 *)> callback);
std::string season ();
