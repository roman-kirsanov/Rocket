/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <optional>
#include <algorithm>
#include <utility>
#include <vector>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App.hpp>
#include <Rocket/Window/Gamepad_SDL3.hpp>

namespace Rocket {

static auto _gamepads = std::vector<std::pair<SDL_JoystickID, SDL_Gamepad*>>();

static SDL_Gamepad* _FindGamepad(SDL_JoystickID id) {
    PROFILE

    auto const it = std::ranges::find(_gamepads, id, &std::pair<SDL_JoystickID, SDL_Gamepad*>::first);

    return (it != _gamepads.end()) ? it->second : nullptr;
}

static std::optional<GamepadButton> _ButtonConvert(Uint8 button) {
    PROFILE

    switch (button) {
        case SDL_GAMEPAD_BUTTON_SOUTH: return GamepadButton::South;
        case SDL_GAMEPAD_BUTTON_EAST: return GamepadButton::East;
        case SDL_GAMEPAD_BUTTON_WEST: return GamepadButton::West;
        case SDL_GAMEPAD_BUTTON_NORTH: return GamepadButton::North;
        case SDL_GAMEPAD_BUTTON_BACK: return GamepadButton::Back;
        case SDL_GAMEPAD_BUTTON_GUIDE: return GamepadButton::Guide;
        case SDL_GAMEPAD_BUTTON_START: return GamepadButton::Start;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return GamepadButton::LeftStick;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return GamepadButton::RightStick;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GamepadButton::LeftShoulder;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GamepadButton::RightShoulder;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return GamepadButton::DPadUp;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GamepadButton::DPadDown;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GamepadButton::DPadLeft;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GamepadButton::DPadRight;
        default: return std::nullopt;
    }
}

static std::optional<GamepadAxis> _AxisConvert(Uint8 axis) {
    PROFILE

    switch (axis) {
        case SDL_GAMEPAD_AXIS_LEFTX: return GamepadAxis::LeftX;
        case SDL_GAMEPAD_AXIS_LEFTY: return GamepadAxis::LeftY;
        case SDL_GAMEPAD_AXIS_RIGHTX: return GamepadAxis::RightX;
        case SDL_GAMEPAD_AXIS_RIGHTY: return GamepadAxis::RightY;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return GamepadAxis::LeftTrigger;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return GamepadAxis::RightTrigger;
        default: return std::nullopt;
    }
}

/* SDL gives a stick -32768..32767 and a trigger 0..32767. */
static float _AxisValue(Sint16 value) {
    PROFILE

    return std::max(-1.0f, static_cast<float>(value) / 32767.0f);
}

std::vector<std::uint32_t> GetGamepads() {
    PROFILE

    auto ids = std::vector<std::uint32_t>();

    for (auto const& [ id, gamepad ] : _gamepads) {
        ids.push_back(id);
    }

    return ids;
}

std::string GetGamepadName(std::uint32_t gamepad) {
    PROFILE

    auto const found = _FindGamepad(gamepad);
    auto const name = (found != nullptr) ? SDL_GetGamepadName(found) : nullptr;

    return (name != nullptr) ? std::string(name) : std::string();
}

bool __IsGamepadEvent(SDL_Event const& event) {
    PROFILE

    return (event.type >= SDL_EVENT_GAMEPAD_AXIS_MOTION)
        && (event.type <= SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED);
}

void __HandleGamepadEvent(SDL_Event const& event) {
    PROFILE

    switch (event.type) {
        case SDL_EVENT_GAMEPAD_ADDED: {
            if (auto const gamepad = SDL_OpenGamepad(event.gdevice.which); gamepad != nullptr) {
                _gamepads.push_back({ event.gdevice.which, gamepad });
                App::OnEvent.publish(GamepadConnectAppEvent(event.gdevice.which));
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            if (auto const gamepad = _FindGamepad(event.gdevice.which); gamepad != nullptr) {
                SDL_CloseGamepad(gamepad);
                std::erase(_gamepads, std::pair(event.gdevice.which, gamepad));
                App::OnEvent.publish(GamepadDisconnectAppEvent(event.gdevice.which));
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
            if (auto const button = _ButtonConvert(event.gbutton.button); button.has_value()) {
                _GamepadDown(event.gbutton.which, *button);
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP: {
            if (auto const button = _ButtonConvert(event.gbutton.button); button.has_value()) {
                _GamepadUp(event.gbutton.which, *button);
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            if (auto const axis = _AxisConvert(event.gaxis.axis); axis.has_value()) {
                _GamepadAxis(event.gaxis.which, *axis, _AxisValue(event.gaxis.value));
            }
            break;
        }
        default:
            break;
    }
}

void __InitGamepads() {
    PROFILE

    /* Without gamepads the app still runs: a failure leaves it with none. */
    SDL_InitSubSystem(SDL_INIT_GAMEPAD);
}

void __DoneGamepads() {
    PROFILE

    for (auto const& [ id, gamepad ] : _gamepads) {
        SDL_CloseGamepad(gamepad);
    }

    _gamepads.clear();

    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void __UpdateGamepads() {
    PROFILE

    SDL_PumpEvents();

    auto event = SDL_Event{};

    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_GAMEPAD_AXIS_MOTION, SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED) > 0) {
        __HandleGamepadEvent(event);
    }

    /* Nothing else reads SDL's queue here: drop the joystick events it keeps alongside. */
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
}

} /* namespace Rocket */
