#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <rfl.hpp>

#include "poll.h"

namespace Config {

using chassisIdLimit = rfl::Validator<std::string, rfl::Size<rfl::Maximum<15>>>;
struct amauthConfig {
    std::string server = "127.0.0.1";
    std::string port = "54430";
    chassisIdLimit chassis_id = "284111080000";
    std::string shop_id = "TAIKO ARCADE LOADER";
    std::string game_ver = "00.00";
    std::string country_code = "JPN";

    std::string full_address() const {
        auto result = server + (port.empty() ? "" : ":" + port);
        if (result.size() > 255) {
            result.resize(255);
        }
        return result;
    }

    std::string place_id() const {
        // Truncate to max 15 characters
        auto result = country_code + "0FF0";
        if (result.size() > 15) {
            result.resize(15);
        }
        return result;
    }
};

using patchVersions = rfl::Literal<"auto", "JPN00", "JPN08", "JPN39", "CHN00">;

struct patchesConfig {
    patchVersions version{"auto"};
    bool unlock_songs = true;
    bool local_files = true;
    // int maximum_song = 1600;
    struct chn00Config {
        bool fix_language = false;
        bool demo_movie = true;
        bool mode_collabo025 = false;
        bool mode_collabo026 = false;
    };
    chn00Config chn00;
};

struct emulationConfig {
    bool usio = true;
    bool card_reader = true;
    bool accept_invalid = false;
    bool qr = true;
};

struct graphicsConfig {
    bool windowed = false;
    bool cursor = true;
    struct resConfig {
        int x = 1920;
        int y = 1080;
    };
    resConfig res;
    bool vsync = false;
    float model_res_rate = 1.0f;
};

struct audioConfig {
    bool real = true;
    bool wasapi_shared = true;
    bool asio = false;
    std::string asio_driver = "ASIO4ALL v2";
};

struct qrConfig {
    std::string image_path = "";
    struct qrDataConfig {
        std::string serial = "";
        int type = 0;
        std::vector<int> song_no;
    };
    qrDataConfig data;
};

struct controllerConfig {
    int wait_period = 0;
    bool analog_input = false;
    bool global_keyboard = false;
    bool simple_input = false;
};

struct keyboardConfig {
    bool auto_ime = true;
    bool jp_layout = false;
};

struct layeredfsConfig {
    bool enabled = false;
    std::string mod_dir = "Data_mods";
};

using logLevels = rfl::Literal<"NONE", "ERROR", "WARN", "INFO", "GAME", "DEBUG", "HOOKS">;

struct loggingConfig {
    logLevels log_level{ "GAME" };
    bool log_to_file = false;
    bool log_method_name = false;
    bool log_full_source = false;
    std::string log_dir = "./logs/";
};

struct globalConfig {
    amauthConfig amauth;
    patchesConfig patches;
    emulationConfig emulation;
    graphicsConfig graphics;
    audioConfig audio;
    qrConfig qr;
    controllerConfig controller;
    keyboardConfig keyboard;
    layeredfsConfig layeredfs;
    loggingConfig logging;
};

struct keybindingConfig {
    rfl::Rename<"EXIT", Keybindings> exit                    = Keybindings{.keycodes = {VK_ESCAPE}};
    rfl::Rename<"TEST", Keybindings> test                    = Keybindings{.keycodes = {VK_F1}};
    rfl::Rename<"SERVICE", Keybindings> service              = Keybindings{.keycodes = {VK_F2}};
    rfl::Rename<"DEBUG_UP", Keybindings> debug_up            = Keybindings{.keycodes = {VK_UP}};
    rfl::Rename<"DEBUG_DOWN", Keybindings> debug_down        = Keybindings{.keycodes = {VK_DOWN}};
    rfl::Rename<"DEBUG_ENTER", Keybindings> debug_enter      = Keybindings{.keycodes = {VK_RETURN}};
    rfl::Rename<"COIN_ADD", Keybindings> coin_add            = Keybindings{.keycodes = {VK_RETURN}, .buttons = {SDL_GAMEPAD_BUTTON_START}};
    rfl::Rename<"CARD_INSERT_1", Keybindings> card_insert_1  = Keybindings{.keycodes = {'P'}};
    rfl::Rename<"CARD_INSERT_2", Keybindings> card_insert_2  = Keybindings{.keycodes = {}};
    rfl::Rename<"QR_DATA_READ", Keybindings> qr_data_read    = Keybindings{.keycodes = {'Q'}};
    rfl::Rename<"QR_IMAGE_READ", Keybindings> qr_image_read  = Keybindings{.keycodes = {'W'}};
    rfl::Rename<"P1_LEFT_BLUE", Keybindings> p1_left_blue    = Keybindings{.keycodes = {'D'}, .axis = {SDLAxis::SDL_AXIS_LEFT_DOWN}};
    rfl::Rename<"P1_LEFT_RED", Keybindings> p1_left_red      = Keybindings{.keycodes = {'F'}, .axis = {SDLAxis::SDL_AXIS_LEFT_RIGHT}};
    rfl::Rename<"P1_RIGHT_RED", Keybindings> p1_right_red    = Keybindings{.keycodes = {'J'}, .axis = {SDLAxis::SDL_AXIS_RIGHT_RIGHT}};
    rfl::Rename<"P1_RIGHT_BLUE", Keybindings> p1_right_blue  = Keybindings{.keycodes = {'K'}, .axis = {SDLAxis::SDL_AXIS_RIGHT_DOWN}};
    rfl::Rename<"P2_LEFT_BLUE", Keybindings> p2_left_blue    = Keybindings{.keycodes = {'Z'}};
    rfl::Rename<"P2_LEFT_RED", Keybindings> p2_left_red      = Keybindings{.keycodes = {'X'}};
    rfl::Rename<"P2_RIGHT_RED", Keybindings> p2_right_red    = Keybindings{.keycodes = {'C'}};
    rfl::Rename<"P2_RIGHT_BLUE", Keybindings> p2_right_blue  = Keybindings{.keycodes = {'V'}};
    rfl::Skip<bool> usingKeyboard;
    rfl::Skip<bool> usingMouse;
    rfl::Skip<bool> usingController;
};

class ConfigManager {
public:
    inline static const std::map<std::string, u8> ConfigKeyboardButtons_US = {
    // Reference:https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    // Wayback Machine:https://web.archive.org/web/20231223135232/https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    // Row 1
    {"ESCAPE", VK_ESCAPE},
    {"F1", VK_F1},
    {"F2", VK_F2},
    {"F3", VK_F3},
    {"F4", VK_F4},
    {"F5", VK_F5},
    {"F6", VK_F6},
    {"F7", VK_F7},
    {"F8", VK_F8},
    {"F9", VK_F9},
    {"F10", VK_F10},
    {"F11", VK_F11},
    {"F12", VK_F12},

    // Row 2
    {"`", VK_OEM_3},
    {"1", '1'},
    {"2", '2'},
    {"3", '3'},
    {"4", '4'},
    {"5", '5'},
    {"6", '6'},
    {"7", '7'},
    {"8", '8'},
    {"9", '9'},
    {"0", '0'},
    {"-", VK_OEM_MINUS},
    {"=", VK_OEM_PLUS},
    {"BACKSPACE", VK_BACK},

    // Row 3
    {"TAB", VK_TAB},
    {"Q", 'Q'},
    {"W", 'W'},
    {"E", 'E'},
    {"R", 'R'},
    {"T", 'T'},
    {"Y", 'Y'},
    {"U", 'U'},
    {"I", 'I'},
    {"O", 'O'},
    {"P", 'P'},
    {"[", VK_OEM_4},
    {"]", VK_OEM_6},
    {"BACKSLASH", VK_OEM_5},

    // Row 4
    {"CAPS_LOCK", VK_CAPITAL},
    {"A", 'A'},
    {"S", 'S'},
    {"D", 'D'},
    {"F", 'F'},
    {"G", 'G'},
    {"H", 'H'},
    {"J", 'J'},
    {"K", 'K'},
    {"L", 'L'},
    {";", VK_OEM_1},
    {"'", VK_OEM_7},
    {"ENTER", VK_RETURN},

    // Row 5
    {"SHIFT", VK_SHIFT},
    {"Z", 'Z'},
    {"X", 'X'},
    {"C", 'C'},
    {"V", 'V'},
    {"B", 'B'},
    {"N", 'N'},
    {"M", 'M'},
    {",", VK_OEM_COMMA},
    {".", VK_OEM_PERIOD},
    {"SLASH", VK_OEM_2},

    // Row 6
    {"CONTROL", VK_CONTROL},
    {"L_WIN", VK_LWIN},
    {"ALT", VK_MENU},
    {"SPACE", VK_SPACE},
    {"R_WIN", VK_RWIN},
    {"MENU", VK_APPS},

    // Other Keys
    // PrtSc is more important when making snapshots, therefore comment it as reserved
    //{"PRINT_SCREEN", VK_SNAPSHOT},
    {"SCROLL_LOCK", VK_SCROLL},
    {"PAUSE", VK_PAUSE},
    {"INSERT", VK_INSERT},
    {"DELETE", VK_DELETE},
    {"HOME", VK_HOME},
    {"END", VK_END},
    {"PAGE_UP", VK_PRIOR},
    {"PAGE_DOWN", VK_NEXT},

    // Arrow Keys
    {"UPARROW", VK_UP},
    {"LEFTARROW", VK_LEFT},
    {"DOWNARROW", VK_DOWN},
    {"RIGHTARROW", VK_RIGHT},

    // NUMPAD Keys
    {"NUM_LOCK", VK_NUMLOCK},
    {"DIVIDE", VK_DIVIDE},
    {"MULTIPLY", VK_MULTIPLY},
    {"SUBTRACT", VK_SUBTRACT},
    {"NUM7", VK_NUMPAD7},
    {"NUM8", VK_NUMPAD8},
    {"NUM9", VK_NUMPAD9},
    {"ADD", VK_ADD},
    {"NUM4", VK_NUMPAD4},
    {"NUM5", VK_NUMPAD5},
    {"NUM6", VK_NUMPAD6},
    {"NUM1", VK_NUMPAD1},
    {"NUM2", VK_NUMPAD2},
    {"NUM3", VK_NUMPAD3},
    {"NUM0", VK_NUMPAD0},
    {"DECIMAL", VK_DECIMAL},
};
    inline static const std::map<std::string, u8> ConfigKeyboardButtons_JP = {
    // Reference:https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    // Wayback Machine:https://web.archive.org/web/20231223135232/https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    // Row 1
    {"ESCAPE", VK_ESCAPE},
    {"F1", VK_F1},
    {"F2", VK_F2},
    {"F3", VK_F3},
    {"F4", VK_F4},
    {"F5", VK_F5},
    {"F6", VK_F6},
    {"F7", VK_F7},
    {"F8", VK_F8},
    {"F9", VK_F9},
    {"F10", VK_F10},
    {"F11", VK_F11},
    {"F12", VK_F12},

    // Row 2
    {"1", '1'},
    {"2", '2'},
    {"3", '3'},
    {"4", '4'},
    {"5", '5'},
    {"6", '6'},
    {"7", '7'},
    {"8", '8'},
    {"9", '9'},
    {"0", '0'},
    {"-", VK_OEM_MINUS},
    {"^", VK_OEM_7},
    {"YEN", VK_OEM_5},
    {"BACKSPACE", VK_BACK},

    // Row 3
    {"TAB", VK_TAB},
    {"Q", 'Q'},
    {"W", 'W'},
    {"E", 'E'},
    {"R", 'R'},
    {"T", 'T'},
    {"Y", 'Y'},
    {"U", 'U'},
    {"I", 'I'},
    {"O", 'O'},
    {"P", 'P'},
    {"@", VK_OEM_3},
    {"[", VK_OEM_4},

    // Row 4
    {"CAPS_LOCK", VK_CAPITAL},
    {"A", 'A'},
    {"S", 'S'},
    {"D", 'D'},
    {"F", 'F'},
    {"G", 'G'},
    {"H", 'H'},
    {"J", 'J'},
    {"K", 'K'},
    {"L", 'L'},
    {";", VK_OEM_PLUS},
    {":", VK_OEM_1},
    {"]", VK_OEM_6},
    {"ENTER", VK_RETURN},

    // Row 5
    {"SHIFT", VK_SHIFT},
    {"Z", 'Z'},
    {"X", 'X'},
    {"C", 'C'},
    {"V", 'V'},
    {"B", 'B'},
    {"N", 'N'},
    {"M", 'M'},
    {",", VK_OEM_COMMA},
    {".", VK_OEM_PERIOD},
    {"SLASH", VK_OEM_2},
    {"BACKSLASH", VK_OEM_102},

    // Row 6
    {"CONTROL", VK_CONTROL},
    {"L_WIN", VK_LWIN},
    {"ALT", VK_MENU},
    {"SPACE", VK_SPACE},
    {"R_WIN", VK_RWIN},
    {"MENU", VK_APPS},

    // Other Keys
    // PrtSc is more important when making snapshots, therefore comment it as reserved
    //{"PRINT_SCREEN", VK_SNAPSHOT},
    {"SCROLL_LOCK", VK_SCROLL},
    {"PAUSE", VK_PAUSE},
    {"INSERT", VK_INSERT},
    {"DELETE", VK_DELETE},
    {"HOME", VK_HOME},
    {"END", VK_END},
    {"PAGE_UP", VK_PRIOR},
    {"PAGE_DOWN", VK_NEXT},

    // Arrow Keys
    {"UPARROW", VK_UP},
    {"LEFTARROW", VK_LEFT},
    {"DOWNARROW", VK_DOWN},
    {"RIGHTARROW", VK_RIGHT},

    // NUMPAD Keys
    {"NUM_LOCK", VK_NUMLOCK},
    {"DIVIDE", VK_DIVIDE},
    {"MULTIPLY", VK_MULTIPLY},
    {"SUBTRACT", VK_SUBTRACT},
    {"NUM7", VK_NUMPAD7},
    {"NUM8", VK_NUMPAD8},
    {"NUM9", VK_NUMPAD9},
    {"ADD", VK_ADD},
    {"NUM4", VK_NUMPAD4},
    {"NUM5", VK_NUMPAD5},
    {"NUM6", VK_NUMPAD6},
    {"NUM1", VK_NUMPAD1},
    {"NUM2", VK_NUMPAD2},
    {"NUM3", VK_NUMPAD3},
    {"NUM0", VK_NUMPAD0},
    {"DECIMAL", VK_DECIMAL},
};

    inline static std::map<std::string, u8> ConfigKeyboardButtons{};

    inline static const std::map<std::string, SDL_GamepadButton> ConfigControllerButtons = {
        {"SDL_A", SDL_GAMEPAD_BUTTON_SOUTH},
        {"SDL_B", SDL_GAMEPAD_BUTTON_EAST},
        {"SDL_X", SDL_GAMEPAD_BUTTON_WEST},
        {"SDL_Y", SDL_GAMEPAD_BUTTON_NORTH},
        {"SDL_BACK", SDL_GAMEPAD_BUTTON_BACK},
        {"SDL_GUIDE", SDL_GAMEPAD_BUTTON_GUIDE},
        {"SDL_START", SDL_GAMEPAD_BUTTON_START},
        {"SDL_LSTICK_PRESS", SDL_GAMEPAD_BUTTON_LEFT_STICK},
        {"SDL_RSTICK_PRESS", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
        {"SDL_LSHOULDER", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
        {"SDL_RSHOULDER", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
        {"SDL_DPAD_UP", SDL_GAMEPAD_BUTTON_DPAD_UP},
        {"SDL_DPAD_DOWN", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
        {"SDL_DPAD_LEFT", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
        {"SDL_DPAD_RIGHT", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
        {"SDL_MISC", SDL_GAMEPAD_BUTTON_MISC1},
        {"SDL_PADDLE1", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1},
        {"SDL_PADDLE2", SDL_GAMEPAD_BUTTON_LEFT_PADDLE1},
        {"SDL_PADDLE3", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2},
        {"SDL_PADDLE4", SDL_GAMEPAD_BUTTON_LEFT_PADDLE2},
        {"SDL_TOUCHPAD", SDL_GAMEPAD_BUTTON_TOUCHPAD},
    };
    inline static const std::map<std::string, SDLAxis>  ConfigControllerAXIS = {
        {"SDL_LSTICK_LEFT", SDLAxis::SDL_AXIS_LEFT_LEFT},   {"SDL_LSTICK_UP", SDLAxis::SDL_AXIS_LEFT_UP},        {"SDL_LSTICK_DOWN", SDLAxis::SDL_AXIS_LEFT_DOWN},
        {"SDL_LSTICK_RIGHT", SDLAxis::SDL_AXIS_LEFT_RIGHT}, {"SDL_RSTICK_LEFT", SDLAxis::SDL_AXIS_RIGHT_LEFT},   {"SDL_RSTICK_UP", SDLAxis::SDL_AXIS_RIGHT_UP},
        {"SDL_RSTICK_DOWN", SDLAxis::SDL_AXIS_RIGHT_DOWN},  {"SDL_RSTICK_RIGHT", SDLAxis::SDL_AXIS_RIGHT_RIGHT}, {"SDL_LTRIGGER", SDLAxis::SDL_AXIS_LTRIGGER_DOWN},
        {"SDL_RTRIGGER", SDLAxis::SDL_AXIS_RTRIGGER_DOWN},
    };
    inline static const std::map<std::string, Scroll> ConfigMouseScroll = {
        {"SCROLL_UP", Scroll::MOUSE_SCROLL_UP},
        {"SCROLL_DOWN", Scroll::MOUSE_SCROLL_DOWN},
    };

    // Singleton pattern for global access
    static ConfigManager& instance() {
        try {
            static ConfigManager instance;  // Guaranteed to be destroyed, instantiated on first use
            return instance;
        } catch (std::runtime_error &e) {
            std::cerr << "[Logger] Failed to parse Default Config: " << e.what () << std::endl;
            exit (1);
        }
    }

    static bool validateConfigfile(std::string path);

    // Load configuration from a file (deserialization happens here)
    bool loadConfig();

    // Accessor methods
    const globalConfig& getConfig() const { return config; }

    // To access specific parts of the config
    const amauthConfig& getAmauthConfig() const { return config.amauth; }
    const patchesConfig& getPatchesConfig() const { return config.patches; }
    const emulationConfig& getEmulationConfig() const { return config.emulation; }
    const graphicsConfig& getGraphicsConfig() const { return config.graphics; }
    const audioConfig& getAudioConfig() const { return config.audio; }
    const qrConfig& getQrConfig();
    const controllerConfig& getControllerConfig() const { return config.controller; }
    const keyboardConfig& getKeyboardConfig() const { return config.keyboard; }
    const layeredfsConfig& getLayeredFsConfig() const { return config.layeredfs; }
    const loggingConfig& getLoggingConfig() const { return config.logging; }
    const keybindingConfig& getKeyBindings() const { return keyBindings; }

    void setRes(i32 x, i32 y) {
        config.graphics.res.x = x;
        config.graphics.res.y = y;
    }

private:

    ConfigManager();
    ~ConfigManager() = default;

    // Prevent copy/move
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool loadKeyBindings();
    bool updateQrConfig();

    globalConfig config;
    keybindingConfig keyBindings;
    bool isLoaded;

    // Mutex for thread safety (if needed)
    mutable std::mutex mutex;
};
}
