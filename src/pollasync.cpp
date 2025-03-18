#include "poll.h"

#ifdef ASYNC_IO
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>

#include "config.h"

static bool jpLayout       = Config::ConfigManager::instance ().getKeyboardConfig ().jp_layout;
static bool autoIme        = Config::ConfigManager::instance ().getKeyboardConfig ().auto_ime;
static bool globalKeyboard = Config::ConfigManager::instance ().getControllerConfig ().global_keyboard;
static bool simpleInput    = Config::ConfigManager::instance ().getControllerConfig ().simple_input;
static bool emulateUsio    = Config::ConfigManager::instance ().getEmulationConfig ().usio;

extern float axisThreshold;
extern HKL currentLayout;

bool wndForeground = false;
bool usingKeyboard = false;
bool usingMouse = false;
bool usingController = false;
bool usingSDLEvent = false;

bool currentKeyboardState[0xFF] = { false };
uint8_t keyboardCount[0xFF] = { 0 };
uint8_t keyboardDiff[0xFF] = { 0 };
bool keyboardClean = false;
bool currentControllerButtonsState[SDL_GAMEPAD_BUTTON_COUNT] = { false };
uint8_t controllerCount[SDL_GAMEPAD_BUTTON_COUNT] = { 0 };
uint8_t controllerDiff[SDL_GAMEPAD_BUTTON_COUNT] = { 0 };
char currentMouseWheelDirection = 0;
uint8_t mouseWheelCount[2] = { 0 };
uint8_t mouseWheelDiff[2] = { 0 };
// SDLAxisState currentControllerAxisState;
float currentControllerAxisState[static_cast<size_t>(SDLAxis::SDL_AXIS_MAX)] = { 0.0 };
uint8_t controllerAxisCount[static_cast<size_t>(SDLAxis::SDL_AXIS_MAX)]      = { 0 };
uint8_t controllerAxisDiff[static_cast<size_t>(SDLAxis::SDL_AXIS_MAX)]       = { 0 };

int maxCount = 10;

SDL_Window *window;
SDL_Gamepad *controllers[255];
bool flipped[255];

std::thread updatePollThread;

// 钩子句柄
HHOOK keyboardHook;
HHOOK mouseHook;

// 钩子回调函数
LRESULT CALLBACK InputProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        switch (wParam) {
            case WM_KEYDOWN: if (globalKeyboard || wndForeground) {
                auto* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
                DWORD keyCode = pKeyboard->vkCode;
                if (!currentKeyboardState[keyCode]) keyboardCount[keyCode] = std::min (keyboardCount[keyCode] + 1, maxCount);
                currentKeyboardState[keyCode] = true;
            } break;
            // keyup will accepted even if you're not focus in this window
            case WM_KEYUP: {
                KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
                DWORD keyCode = pKeyboard->vkCode;
                currentKeyboardState[keyCode] = false;
            } break;
        default: break;
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

HWND CreateInvisibleWindow(HINSTANCE hInstance) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "InvisibleWindowClass";
    if (!RegisterClass(&wc)) return nullptr;
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        "Invisible Window",
        0, 0, 0, 0, 0,
        NULL, NULL, hInstance, NULL
    );
    if (hwnd) SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    return hwnd;
}

void KeyboardMainLoop() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HWND hwnd = CreateInvisibleWindow(hInstance);
    if (!hwnd) LogMessage (LogLevel::ERROR, "Failed to create window!\n");
    if (usingKeyboard) {
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, InputProc, nullptr, 0);
        if (keyboardHook == nullptr) LogMessage (LogLevel::ERROR, "Failed to install keyboard hook!\n");
        else LogMessage (LogLevel::DEBUG, "KeyboardLL hook installed!");
    }
    if (maxCount > 2) LogMessage (LogLevel::WARN, "Using Cache count max={}", maxCount);
    LogMessage (LogLevel::INFO, "Using Async Keyboard");
    MSG msg;
    while (GetMessage(&msg, hwnd, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (keyboardHook) UnhookWindowsHookEx(keyboardHook);
}

void
InitializeKeyboard () {
    std::thread (KeyboardMainLoop).detach ();
}

void
CheckFlipped (uint32_t which, SDL_Gamepad *gamepad) {
    if (SDL_GetGamepadButtonLabel(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_B) {
        flipped[which] = true;
    } else flipped[which] = false;
}

bool
InitializePoll (HWND windowHandle) {
    wndForeground = windowHandle == GetForegroundWindow ();
    if (!emulateUsio) return false;

    usingKeyboard = Config::ConfigManager::instance ().getKeyBindings ().usingKeyboard();
    usingMouse = Config::ConfigManager::instance ().getKeyBindings ().usingMouse();
    usingController = Config::ConfigManager::instance ().getKeyBindings ().usingController();
    usingSDLEvent = usingMouse || usingController;

    if (simpleInput) LogMessage (LogLevel::INFO, "Using Simple Input (1-1 bind)");

    atexit ([](){ if (currentLayout != nullptr) ActivateKeyboardLayout (currentLayout, KLF_SETFORPROCESS);});
    if (usingKeyboard) InitializeKeyboard ();
    if (usingSDLEvent) {
        LogMessage (LogLevel::DEBUG, "InitializePoll");
        bool hasRumble = true;
        SDL_SetMainReady ();

        SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
        SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
        SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
        SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");

        if (!SDL_Init (SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
            if (SDL_Init (SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
                hasRumble = false;
            } else {
                LogMessage (LogLevel::ERROR, std::string ("SDL_Init (SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_VIDEO): ") + SDL_GetError ());
                return false;
            }
        }

        auto configPath = std::filesystem::current_path () / "gamecontrollerdb.txt";
        if (SDL_AddGamepadMappingsFromFile (configPath.string ().c_str ()) == -1)
            LogMessage (LogLevel::ERROR, "Cannot read gamecontrollerdb.txt");
        SDL_SetGamepadEventsEnabled (true);
        SDL_SetJoystickEventsEnabled (true);

        int numJoysticks = 0;
        auto* joyStickIds = SDL_GetJoysticks(&numJoysticks);
        if (joyStickIds == nullptr) {
            LogMessage (LogLevel::ERROR, "Error getting joystick ids");
            exit(1);
        }
        for (int i = 0; i < numJoysticks; i++) {
            auto id = joyStickIds[i];
            if (!SDL_IsGamepad (id)) continue;
            SDL_Gamepad *controller = SDL_OpenGamepad (id);

            if (!controller) {
                LogMessage (LogLevel::WARN, std::string ("Could not open gamecontroller ") + SDL_GetGamepadNameForID (id) + ": " + SDL_GetError ());
                continue;
            }

            LogMessage (LogLevel::DEBUG, "Init Controller connected!");
            CheckFlipped (id, controller);
            controllers[id] = controller;
        }
        SDL_free (joyStickIds);

        SDL_PropertiesID props = SDL_CreateProperties();
        if(props == 0) {
            LogMessage (LogLevel::ERROR, "Unable to create SDL_Properties");
            exit(1);
        }
        SDL_SetPointerProperty (props, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, windowHandle);
        window = SDL_CreateWindowWithProperties (props);
        if (window == NULL) LogMessage (LogLevel::ERROR, std::string ("SDL_CreateWindowFrom (windowHandle): ") + SDL_GetError ());

        atexit (DisposePoll);
        return hasRumble;
    } else return false;
}

bool
CheckForegroundWindow (HWND processWindow) {
    if (processWindow == nullptr) return false;
    HWND foregroundWnd = GetForegroundWindow ();
    if (wndForeground ^ (processWindow == foregroundWnd)) {
        wndForeground = !wndForeground;
        LogMessage (LogLevel::DEBUG, "window focus={}", wndForeground);
        if (autoIme) {
            if (wndForeground) {
                currentLayout  = GetKeyboardLayout (0);
                auto engLayout = LoadKeyboardLayout (TEXT ("00000409"), KLF_ACTIVATE);
                ActivateKeyboardLayout (engLayout, KLF_SETFORPROCESS);
                LogMessage (LogLevel::DEBUG, "(experimental) auto change KeyboardLayout {}", LOWORD(engLayout));
            }
        }
    }
    return wndForeground;
}

uint8_t
CorrectButton (uint32_t which, uint8_t button) {
    if (!flipped[which]) return button;
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return SDL_GAMEPAD_BUTTON_EAST;
    case SDL_GAMEPAD_BUTTON_EAST: return SDL_GAMEPAD_BUTTON_SOUTH;
    case SDL_GAMEPAD_BUTTON_WEST: return SDL_GAMEPAD_BUTTON_NORTH;
    case SDL_GAMEPAD_BUTTON_NORTH: return SDL_GAMEPAD_BUTTON_WEST;
    default: return button;
    }
}

void
CleanPoll () {
    if (usingKeyboard) {
        for (int i=0; i<0xFF; i++) keyboardCount[i] -= (bool)keyboardDiff[i];
        memset (keyboardDiff, 0, 0XFF);
    }

    if (usingMouse) {
        currentMouseState.ScrolledUp   = false;
        currentMouseState.ScrolledDown = false;
        if (mouseWheelCount[0] > 0) mouseWheelCount[0] -= (bool)mouseWheelDiff[0];
        if (mouseWheelCount[1] > 0) mouseWheelCount[1] -= (bool)mouseWheelDiff[1];
        memset (mouseWheelDiff, 0, 2);
    }

    if (usingController) {
        for (int i=0; i<SDL_GAMEPAD_BUTTON_COUNT; i++) controllerCount[i] -= (bool)controllerDiff[i];
        for (int i=1; i< static_cast<int>(SDLAxis::SDL_AXIS_MAX); i++) controllerAxisCount[i] -= (bool)controllerAxisDiff[i];
        memset (controllerDiff, 0, SDL_GAMEPAD_BUTTON_COUNT);
        memset (controllerAxisDiff, 0, static_cast<size_t>(SDLAxis::SDL_AXIS_MAX));
    }
}

void
UpdatePoll (HWND windowHandle) {
    if (!CheckForegroundWindow (windowHandle)) return;
    if (!emulateUsio) return;
    if (!simpleInput) CleanPoll ();
    if (usingSDLEvent) {
        SDL_Event event;
        SDL_Gamepad *controller;
        while (SDL_PollEvent (&event) != 0) {
            switch (event.type) {
            case SDL_EVENT_MOUSE_WHEEL: {
                if (event.wheel.y > 0) {
                    mouseWheelCount[0] = std::min (mouseWheelCount[0] + 1, maxCount);
                    currentMouseState.ScrolledUp = true;
                } else if (event.wheel.y < 0) {
                    mouseWheelCount[1] = std::min (mouseWheelCount[1] + 1, maxCount);
                    currentMouseState.ScrolledDown = true;
                }
            } break;
            case SDL_EVENT_GAMEPAD_ADDED:
                if (!SDL_IsGamepad (event.gdevice.which)) break;
                controller = SDL_OpenGamepad (event.gdevice.which);
                if (!controller) {
                    LogMessage (LogLevel::ERROR, "Could not open gamecontroller {}: {}", SDL_GetGamepadNameForID (event.gdevice.which), SDL_GetError ());
                    continue;
                }
                LogMessage (LogLevel::DEBUG, "Controller connected!");
                CheckFlipped (event.gdevice.which, controller);
                controllers[event.gdevice.which] = controller;
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                LogMessage (LogLevel::DEBUG, "Controller removed!");
                if (!SDL_IsGamepad (event.gdevice.which)) break;
                SDL_CloseGamepad (controllers[event.gdevice.which]);
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_UP: {
                uint8_t button = CorrectButton (event.gbutton.which, event.gbutton.button);
                currentControllerButtonsState[button] = false;
            } break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                if (wndForeground) {
                    uint8_t button = CorrectButton (event.gbutton.which, event.gbutton.button);
                    LogMessage (LogLevel::DEBUG, "Controller {}: button {} pressed, remapped to {}", event.gbutton.which, event.gbutton.button, button);
                    if (!currentControllerButtonsState[button]) {
                        controllerCount[button] = std::min (controllerCount[button] + 1, maxCount);
                    }
                    currentControllerButtonsState[button] = true;
                } else LogMessage (LogLevel::DEBUG, "Controller button pressed but window not focused!");
                break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                float sensorValue = event.gaxis.value == 0 ? 0 : event.gaxis.value / (event.gaxis.value > 0 ? 32767.0f : -32768.0f);
                if (wndForeground || sensorValue < axisThreshold) {
                    switch (event.gaxis.axis) {
                    case SDL_GAMEPAD_AXIS_LEFTX: case SDL_GAMEPAD_AXIS_LEFTY: case SDL_GAMEPAD_AXIS_RIGHTX: case SDL_GAMEPAD_AXIS_RIGHTY: {
                        int direction = event.gaxis.axis * 2 + (event.gaxis.value < 0 ? 1 : 2);
                        int reverse = event.gaxis.axis * 2 + (event.gaxis.value < 0 ? 2 : 1);
                        currentControllerAxisState[reverse] = 0.0;
                        if (sensorValue > axisThreshold && currentControllerAxisState[direction] <= axisThreshold) {
                            LogMessage (LogLevel::DEBUG, "direction: {} value: {}", direction, sensorValue);
                            controllerAxisCount[direction] = std::min (controllerAxisCount[direction] + 1, maxCount);
                        }
                        currentControllerAxisState[direction] = sensorValue;
                    } break;
                    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: {
                        int button = 5 + event.gaxis.axis;
                        if (sensorValue > axisThreshold && currentControllerAxisState[button] <= axisThreshold) {
                            controllerAxisCount[button] = std::min (controllerAxisCount[button] + 1, maxCount);
                        }
                        currentControllerAxisState[button] = sensorValue;
                    } break;
                    } break;
                }
            }
        }
    }
}

void
DisposePoll () {
    SDL_DestroyWindow (window);
    SDL_Quit ();
}

void
SetRumble (int left, int right, int length) {
    for (auto &controller : controllers) {
        const auto props = SDL_GetGamepadProperties(controller);
        const bool hasRumble = SDL_GetBooleanProperty (props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
        if (!controller || !hasRumble) continue;
        SDL_RumbleGamepad (controller, left, right, length);
    }
}

bool
KeyboardIsDown (const uint8_t keycode) {
    return currentKeyboardState[keycode];
}

int maxKeyboardCount = 1;
bool
KeyboardIsTapped (const uint8_t keycode) {
    if (keyboardCount[keycode] > 0) {
        if (simpleInput) keyboardCount[keycode] --;
        else {
            keyboardDiff[keycode] = 1;
            keyboardClean = true;
        }
        return true;
    } return false;
}

bool
GetMouseScrollUp () {
    return currentMouseState.ScrolledUp;
}

bool
GetMouseScrollDown () {
    return currentMouseState.ScrolledDown;
}

bool
GetMouseScrollIsDown (const Scroll scroll) {
    if (scroll == Scroll::MOUSE_SCROLL_UP) return GetMouseScrollUp ();
    else return GetMouseScrollDown ();
}

int maxWheelCount = 1;
bool
GetMouseScrollIsTapped (const Scroll scroll) {
    if (scroll == Scroll::MOUSE_SCROLL_INVALID) return false;
    if (mouseWheelCount[(int)scroll - 1] > 0) {
        if (simpleInput) mouseWheelCount[(int)scroll - 1] --;
        else mouseWheelDiff[(int)scroll - 1] = 1;
        return true;
    } return false;
}

bool
ControllerButtonIsDown (const SDL_GamepadButton button) {
    return currentControllerButtonsState[button];
}

int maxButtonCount = 1;
bool
ControllerButtonIsTapped (const SDL_GamepadButton button) {
    if (controllerCount[button] > 0) {
        if (simpleInput) controllerCount[button] --;
        else controllerDiff[button] = 1;
        return true;
    } return false;
}

float
ControllerAxisIsDown (const SDLAxis axis) {
    return currentControllerAxisState[(int)axis];
}

bool
ControllerAxisIsTapped (const SDLAxis axis) {
    if (controllerAxisCount[(int)axis] > 0) {
        if (simpleInput) controllerAxisCount[(int)axis] --;
        else controllerAxisDiff[(int)axis] = 1;
        return true;
    } return false;
}

bool
IsButtonTapped (const Keybindings &bindings) {
    for (const auto keycode : bindings.keycodes) {
        if (keycode == 0) continue;
        if (KeyboardIsTapped (keycode)) return true;
    }

    for (const auto button : bindings.buttons) {
        if (button == SDL_GAMEPAD_BUTTON_INVALID) continue;
        if (ControllerButtonIsTapped (button)) return true;
    }

    for (const auto axis : bindings.axis) {
        if (axis == SDLAxis::SDL_AXIS_NULL) continue;
        if (ControllerAxisIsTapped (axis)) return true;
    }

    for (const auto scroll : bindings.scroll) {
        if (scroll == Scroll::MOUSE_SCROLL_INVALID) continue;
        if (GetMouseScrollIsTapped (scroll)) return true;
    }

    return false;
}

float
IsButtonDown (const Keybindings &bindings) {
    for (const auto keycode : bindings.keycodes) {
         if (keycode == 0) continue;
         if (KeyboardIsDown (keycode)) return 1.0f;
    }

    for (const auto button : bindings.buttons) {
        if (button == SDL_GAMEPAD_BUTTON_INVALID) continue;
        if (ControllerButtonIsDown (button)) return 1.0f;
    }

    for (const auto axis : bindings.axis) {
        if (axis == SDLAxis::SDL_AXIS_NULL) continue;
        if (float val = ControllerAxisIsDown (axis); val > 0) return val;
    }

    for (const auto scroll : bindings.scroll) {
        if (scroll == Scroll::MOUSE_SCROLL_INVALID) continue;
        if (GetMouseScrollIsDown (scroll)) return 1.0f;
    }

    return 0.0f;
}
#endif