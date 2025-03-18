#include <filesystem>
#include <istream>
#include <iostream>
#include <Windows.h>
#include <rfl/toml.hpp>
#include <map>
#include <stdexcept>

#include "banner.h"
#include "config.h"
#include "logger.h"

static bool usingKeyboard = false;
static bool usingMouse = false;
static bool usingController = false;


namespace Config {
bool
ConfigManager::validateConfigfile (std::string path) {
    std::filesystem::path configPath = path;
    if (!exists (configPath) || !configPath.has_filename ()) {
        std::cerr << configPath.string () << ": file does not exist" << std::endl;
        return false;
    }

    std::ifstream stream (configPath);
    if (!stream.is_open ()) {
        std::cerr << "Could not open " << configPath.string () << std::endl;
        return false;
    }

    auto result = rfl::toml::read<struct globalConfig> (stream);
    if (result) {
        return true;
    }
    auto error = result.error ().value ();
    std::cerr << "Validate config failed with error: " << error.what () << std::endl;
    return false;
}

bool
ConfigManager::loadConfig () {
    std::lock_guard lock (mutex);

    if (isLoaded) { return true; }

    const auto configPath = std::filesystem::current_path () / "config.toml";

    if (!exists (configPath) || !configPath.has_filename ()) {
        LogMessage (LogLevel::WARN, std::string (configPath.string ()) + ": file does not exist");
        return false;
    }

    std::ifstream stream (configPath);
    if (!stream.is_open ()) {
        LogMessage (LogLevel::WARN, "Could not open " + std::string (configPath.string ()));
        return false;
    }

    auto result = rfl::toml::read<struct globalConfig, rfl::DefaultIfMissing> (stream);
    if (result) {
        config   = result.value ();
        bool success = loadKeyBindings ();
        if (!success) {
            LogMessage (LogLevel::ERROR, "Failed to load keybindings");
            MessageBoxA (nullptr, "Failed to load keybindings, check logs for error", nullptr, MB_OK);
            ExitProcess (0);
        }
        isLoaded = true;
        return true;
    }

    auto error = result.error ().value ();
    LogMessage (LogLevel::WARN, "Read config failed with error: " + error.what ());
    return false;
}

const qrConfig &
ConfigManager::getQrConfig () {
    auto result = updateQrConfig ();
    if (!result) {
        LogMessage (LogLevel::ERROR, "Failed to update QR config");
        MessageBoxA (nullptr, "Failed to update QR config", nullptr, MB_OK);
        ExitProcess (0);
    }
    return config.qr;

}

ConfigManager::ConfigManager () {
    Banner::Version::instance ();
    Logger::InitializeLogger (LogLevel::INFO, false, "");
    auto result = loadConfig ();
    if (!result) {
        LogMessage (LogLevel::ERROR, "Failed to load config file");
        MessageBoxA (nullptr, "Failed to load config", nullptr, MB_OK);
        ExitProcess (0);
    }
    Logger::CleanupLogger ();
}

bool
ConfigManager::loadKeyBindings () {
    LogMessage (LogLevel::INFO, "Using Keyboard Layout: {}", config.keyboard.jp_layout ? "JP" : "US");
    ConfigKeyboardButtons = config.keyboard.jp_layout ?  std::map(ConfigKeyboardButtons_JP) : std::map(ConfigKeyboardButtons_US);

    const auto configPath = std::filesystem::current_path () / "keyconfig.toml";
    if (!exists (configPath) || !configPath.has_filename ()) {
        LogMessage (LogLevel::WARN, std::string (configPath.string ()) + ": file does not exist");
        return false;
    }

    std::ifstream stream (configPath);
    if (!stream.is_open ()) {
        LogMessage (LogLevel::WARN, "Could not open " + std::string (configPath.string ()));
        return false;
    }

    auto result = rfl::toml::read<struct keybindingConfig> (stream);
    if (result) {
        keyBindings = result.value ();
        keyBindings.usingKeyboard = usingKeyboard;
        keyBindings.usingMouse = usingMouse;
        keyBindings.usingController = usingController;
        LogMessage (LogLevel::DEBUG, "Finish Loading keyconfig.toml");
        if (usingKeyboard)   LogMessage (LogLevel::INFO, "Using Input Channel: Keyboard");
        if (usingMouse)      LogMessage (LogLevel::INFO, "Using Input Channel: Mouse Wheel");
        if (usingController) LogMessage (LogLevel::INFO, "Using Input Channel: Controller");
        return true;
    }

    auto error = result.error ().value ();
    LogMessage (LogLevel::WARN, "Read keybindings failed with error: " + error.what ());
    return false;
}

bool
ConfigManager::updateQrConfig () {
    const auto configPath = std::filesystem::current_path () / "config.toml";
    if (!exists (configPath) || !configPath.has_filename ()) {
        LogMessage (LogLevel::WARN, std::string (configPath.string ()) + ": file does not exist");
        return false;
    }

    std::ifstream stream (configPath);
    if (!stream.is_open ()) {
        LogMessage (LogLevel::WARN, "Could not open " + std::string (configPath.string ()));
        return false;
    }

    auto result = rfl::toml::read<struct qrConfig, rfl::DefaultIfMissing> (stream);
    if (result) {
        config.qr = result.value ();
        return true;
    }

    auto error = result.error ().value ();
    LogMessage (LogLevel::WARN, "Read config failed with error: " + error.what ());
    return false;
}
}

template <>
struct rfl::Reflector<Keybindings> {
    using ReflType = std::vector<std::string>;

    static ConfigValue StringToConfigEnum(const std::string& value) {
        ConfigValue ret = {};
        if (Config::ConfigManager::ConfigKeyboardButtons.contains (value)) {
            usingKeyboard = true;
            ret.type = EnumType::keycode;
            ret.keycode = Config::ConfigManager::ConfigKeyboardButtons.at (value);
            return ret;
        }

        if (Config::ConfigManager::ConfigControllerButtons.contains (value)) {
            usingController = true;
            ret.type = EnumType::button;
            ret.button = Config::ConfigManager::ConfigControllerButtons.at (value);
            return ret;
        }

        if (Config::ConfigManager::ConfigControllerAXIS.contains (value)) {
            usingController = true;
            ret.type = EnumType::axis;
            ret.axis = Config::ConfigManager::ConfigControllerAXIS.at (value);
            return ret;
        }

        if (Config::ConfigManager::ConfigMouseScroll.contains (value)) {
            usingMouse = true;
            ret.type = EnumType::scroll;
            ret.scroll = Config::ConfigManager::ConfigMouseScroll.at (value);
            return ret;
        }

        LogMessage(LogLevel::ERROR, "Unknown key config value: {}", value);
        throw std::exception(std::format ("Unknown key config value: {}", value).c_str());
    }

    static Keybindings to(const ReflType& value) {
        Keybindings result {};
        result.buttons.fill (SDL_GAMEPAD_BUTTON_INVALID);
        for (const auto& key : value) {
            const auto config = StringToConfigEnum(key);
            switch (config.type) {
            case EnumType::keycode:
                LogMessage (LogLevel::DEBUG, "config {} type=keycode value={}", key, static_cast<int>(config.keycode));
                for (auto &keycode : result.keycodes) {
                    if (keycode == 0) {
                        keycode = config.keycode;
                        break;
                    }
                }
                break;
            case EnumType::button:
                LogMessage(LogLevel::DEBUG, "config {} type=button value={}", key, static_cast<int>(config.button));
                for (auto &button : result.buttons) {
                    if (button == SDL_GAMEPAD_BUTTON_INVALID) {
                        button = config.button;
                        break;
                    }
                }
                break;
            case EnumType::axis:
                LogMessage (LogLevel::DEBUG, "config {} type=axis value={}", key, static_cast<int>(config.axis));
                for (auto &axis : result.axis) {
                    if (axis == SDLAxis::SDL_AXIS_NULL) {
                        axis = config.axis;
                        break;
                    }
                }
                break;
            case EnumType::scroll:
                LogMessage (LogLevel::DEBUG, "config {} type=scroll value={}", key, static_cast<int>(config.scroll));
                for (auto &scroll : result.scroll) {
                    if (scroll == Scroll::MOUSE_SCROLL_INVALID) {
                        scroll = config.scroll;
                        break;
                    }
                }
                break;
            case EnumType::none:
            default:
                throw std::exception("Invalid keybinding type");
            }
        }
        return result;
    }
};