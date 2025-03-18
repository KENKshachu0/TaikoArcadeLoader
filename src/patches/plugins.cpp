#include "constants.h"
#include "helpers.h"
#include "patches.h"

extern int exited;

namespace patches::Plugins {
    std::vector<HMODULE> plugins = {};

    typedef void   (*BasicEvent)          ();
    typedef void   (*WaitTouchEvent)      (CallBackTouchCard callback, uint64_t touchData);
    typedef void   (*SendVersionEvent)    (GameVersion gameVersion);
    typedef bool   (*CheckEvent)          ();
    typedef size_t (*CopyDataEvent)       (size_t size, uint8_t *buffer);
    typedef void   (*SendCardReaderEvent) (CommitCardCallback touch);
    typedef void   (*SendQRScannerEvent)  (CommitQrCallback scan);
    typedef void   (*SendQRLoginEvent)    (CommitQrLoginCallback login);
    typedef void   (*StatusChangeEvent)   (size_t type, bool status);

    std::mutex updateMutex;
    std::condition_variable updateCV;

    void
    Init () {
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "Init");
            if (event) ((BasicEvent)event) ();
        }
    }
    void
    Update () {
        updateCV.notify_all ();
    }
    void
    Exit () {
        updateCV.notify_all ();
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "Exit");
            if (event) ((BasicEvent)event) ();
        }
    }
    // Low Level Card API
    void
    WaitTouch (CallBackTouchCard callback, uint64_t touchData) {
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "WaitTouch");
            if (event) ((WaitTouchEvent)event) (callback, touchData);
        }
    }
    // New API
    void
    InitVersion (GameVersion gameVersion) {
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "InitVersion");
            if (event) ((SendVersionEvent)event) (gameVersion);
        }
    }
    void
    InitCardReader (CommitCardCallback touch) {
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "InitCardReader");
            if (event) ((SendCardReaderEvent)event) (touch);
        }
    }
    void
    InitQRScanner (CommitQrCallback scan) {
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "InitQRScanner");
            if (event) ((SendQRScannerEvent)event) (scan);
        }
    }
    void
    InitQRLogin (CommitQrLoginCallback login) {
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "InitQRLogin");
            if (event) ((SendQRLoginEvent)event) (login);
        }
    }
    void
    UpdateStatus (size_t type, bool status) {
        // printWarning ("Send UpdateStatus type=%d status=%d", type, status);
        for (auto plugin : plugins) {
            auto event = GetProcAddress (plugin, "UpdateStatus");
            if (event) ((StatusChangeEvent)event) (type, status);
        }
    }

    // Plugins Loader
    void LoadPlugins () {
        auto pluginPath = std::filesystem::current_path () / "plugins";

        if (std::filesystem::exists (pluginPath)) {
            for (const auto &entry : std::filesystem::directory_iterator (pluginPath)) {
                if (entry.path ().extension () == ".dll") {
                    auto name      = entry.path ().wstring ();
                    auto shortName = entry.path ().filename ().wstring ();
                    if (HMODULE hModule = LoadLibraryW (name.c_str ()); !hModule) {
                        LogMessage (LogLevel::ERROR, L"Failed to load plugin: {}", shortName);
                    } else {
                        plugins.push_back (hModule);
                        LogMessage (LogLevel::INFO, L"Using plugin: {}", shortName);
                    }
                }
            }
        }

        std::thread ([&](){
            std::unique_lock<std::mutex> updateLock(updateMutex);
            while (exited == 0) {
                updateCV.wait (updateLock);
                if (exited == 0) {
                    for (auto plugin : plugins) {
                        auto event = GetProcAddress (plugin, "Update");
                        if (event) ((BasicEvent)event) ();
                    }
                }
            }
        }).detach ();
    }
}