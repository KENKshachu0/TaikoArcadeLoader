#pragma once
#include <SDL3/SDL.h>
#include <array>
#include "helpers.h"
#include "bnusio.h"

#ifdef ASYNC_UPDATE
#define ASYNC_IO
#endif

// #define ASYNC_IO

enum class SDLAxis {
    SDL_AXIS_NULL = 0,
    SDL_AXIS_LEFT_LEFT,
    SDL_AXIS_LEFT_RIGHT,
    SDL_AXIS_LEFT_UP,
    SDL_AXIS_LEFT_DOWN,
    SDL_AXIS_RIGHT_LEFT,
    SDL_AXIS_RIGHT_RIGHT,
    SDL_AXIS_RIGHT_UP,
    SDL_AXIS_RIGHT_DOWN,
    SDL_AXIS_LTRIGGER_DOWN,
    SDL_AXIS_RTRIGGER_DOWN,
    SDL_AXIS_MAX
};

struct SDLAxisState {
    float LeftLeft;
    float LeftRight;
    float LeftUp;
    float LeftDown;
    float RightLeft;
    float RightRight;
    float RightUp;
    float RightDown;
    float LTriggerDown;
    float RTriggerDown;
};

enum class Scroll { MOUSE_SCROLL_INVALID = 0, MOUSE_SCROLL_UP, MOUSE_SCROLL_DOWN };

struct Keybindings {
    std::array<u8, 255> keycodes = { };
    std::array<SDL_GamepadButton, 255> buttons = { SDL_GAMEPAD_BUTTON_INVALID };
    std::array<SDLAxis, 255> axis = {SDLAxis::SDL_AXIS_NULL };
    std::array<Scroll,2> scroll = {Scroll::MOUSE_SCROLL_INVALID };
};

enum class EnumType { none, keycode, button, axis, scroll };

struct ConfigValue {
    EnumType type;
    union {
        u8 keycode;
        SDL_GamepadButton button;
        SDLAxis axis;
        Scroll scroll;
    };
};

struct InternalButtonState {
    float Down;
    bool Released;
    bool Tapped;
};

struct KeyCodePair {
    std::string string;
    u8 keycode;
};

struct MouseState {
    POINT Position;
    POINT RelativePosition;
    bool ScrolledUp;
    bool ScrolledDown;
} inline currentMouseState, lastMouseState;

bool InitializePoll (HWND windowHandle);
void DisposePoll ();
InternalButtonState GetInternalButtonState (const Keybindings &bindings);
void SetRumble (int left, int right, int length);

bool KeyboardIsDown (uint8_t keycode);
bool KeyboardIsTapped (uint8_t keycode);
bool GetMouseScrollUp ();
bool GetMouseScrollDown ();
bool GetMouseScrollIsDown (Scroll scroll);
bool GetMouseScrollIsTapped (Scroll scroll);
bool ControllerButtonIsDown (SDL_GamepadButton button);
bool ControllerButtonIsTapped (SDL_GamepadButton button);
float ControllerAxisIsDown (SDLAxis axis);
bool ControllerAxisIsTapped (SDLAxis axis);
bool IsButtonTapped (const Keybindings &bindings);
float IsButtonDown (const Keybindings &bindings);

#ifndef ASYNC_IO
void UpdatePoll (HWND windowHandle);
bool KeyboardIsUp (const u8 keycode);
bool KeyboardIsReleased (u8 keycode);
bool KeyboardWasDown (u8 keycode);
bool KeyboardWasUp (u8 keycode);
POINT GetMousePosition ();
POINT GetLastMousePosition ();
POINT GetMouseRelativePosition ();
POINT GetLastMouseRelativePosition ();
void SetMousePosition (const POINT newPosition);
bool GetWasMouseScrollUp ();
bool GetWasMouseScrollDown ();
bool GetMouseScrollIsReleased (const Scroll scroll);
bool ControllerButtonWasDown (const SDL_GamepadButton button);
bool ControllerButtonWasUp (const SDL_GamepadButton button);
bool ControllerButtonIsReleased (const SDL_GamepadButton button);
bool ControllerAxisIsUp (const SDLAxis axis);
float ControllerAxisWasDown (const SDLAxis axis);
bool ControllerAxisWasUp (const SDLAxis axis);
bool ControllerAxisIsReleased (const SDLAxis axis);
bool IsButtonReleased (const Keybindings &bindings);
#else
void UpdatePoll (HWND windowHandle);
void CleanPoll ();
void InitializeKeyboard ();
#endif