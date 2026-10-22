/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>
#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Private.hpp>
#include <Rocket/Window/App_SDL3.hpp>
#include <Rocket/Window/Window_SDL3.hpp>
#include <Rocket/Window/Gamepad_SDL3.hpp>

namespace Rocket {
namespace App {

static auto constexpr _DEFAULT_REFRESH_RATE = 60.0f;

static auto _isExited = false;
static auto _nextFrame = std::uint64_t(0);

static std::uint64_t _GetFramePeriod() {
    PROFILE

    auto const mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    auto const refreshRate = ((mode != nullptr) && (mode->refresh_rate > 0.0f)) ? mode->refresh_rate : _DEFAULT_REFRESH_RATE;

    return static_cast<std::uint64_t>(1000000000.0 / static_cast<double>(refreshRate));
}

static void _HandleEvent(SDL_Event const& event) {
    PROFILE

    if (event.type == SDL_EVENT_QUIT) {
        _Close();
        return;
    }

    if (__IsGamepadEvent(event)) {
        __HandleGamepadEvent(event);
        return;
    }

    __HandleEvent(event);
}

/* Moves and wheel turns arrive at the mouse's own rate, often far above the
   frame rate, and every one of them can cost a layout. macOS coalesces them to
   the display, and so does the run loop: they are held until the frame is due
   or another event comes, consecutive moves of a window collapse into the last
   one and consecutive turns into their sum. */
static bool _IsCoalescedEvent(SDL_Event const& event) {
    PROFILE

    return (event.type == SDL_EVENT_MOUSE_MOTION)
        || (event.type == SDL_EVENT_MOUSE_WHEEL);
}

static void _CoalesceEvent(std::vector<SDL_Event>& held, SDL_Event const& event) {
    PROFILE

    if (held.empty() == false) {
        auto& last = held.back();

        if (
            (event.type == SDL_EVENT_MOUSE_MOTION) &&
            (last.type == SDL_EVENT_MOUSE_MOTION) &&
            (last.motion.windowID == event.motion.windowID) &&
            (last.motion.state == event.motion.state)
        ) {
            last = event;
            return;
        }

        if (
            (event.type == SDL_EVENT_MOUSE_WHEEL) &&
            (last.type == SDL_EVENT_MOUSE_WHEEL) &&
            (last.wheel.windowID == event.wheel.windowID)
        ) {
            last.wheel.x += event.wheel.x;
            last.wheel.y += event.wheel.y;
            last.wheel.mouse_x = event.wheel.mouse_x;
            last.wheel.mouse_y = event.wheel.mouse_y;
            return;
        }
    }

    held.push_back(event);
}

/* Held events carry no strings, so they outlive SDL's next poll, which frees
   the strings of the events before it: every other event is handled at once. */
static void _HandleHeldEvents(std::vector<SDL_Event>& held) {
    PROFILE

    /* taken out first: a handler may run a nested loop (a modal window), which holds its own */
    auto const events = std::move(held);
    held.clear();

    for (auto const& event : events) {
        _HandleEvent(event);
    }
}

/* SDL returns folders with a trailing separator; Rocket's have none. */
static std::string _TrimSeparator(std::string path) {
    PROFILE

    while ((path.size() > 1) && (path.back() == '/')) {
        path.pop_back();
    }

    return path;
}

void __InitVideo() {
    PROFILE

    static auto _initialized = false;

    if (_initialized) {
        return;
    }

    /* A window never closes by itself, so the last one asking to close must not quit the app. */
    SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0");
    /* The document draws the composition itself (see Window::setInputArea). */
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) == false) {
        throw std::runtime_error(SDL_GetError());
    }

    _initialized = true;
}

std::string const& __GetExecPath() {
    PROFILE

    /* SDL has no function for it: the one POSIX call of the backend */
    static auto const _execPath = [] {
        auto error = std::error_code();
        auto const path = std::filesystem::read_symlink("/proc/self/exe", error);

        return error ? std::string() : path.string();
    }();

    return _execPath;
}

std::string const& __GetExecDir() {
    PROFILE

    static auto const _execDir = [] {
        auto const basePath = SDL_GetBasePath();

        return (basePath != nullptr) ? _TrimSeparator(basePath) : std::string();
    }();

    return _execDir;
}

std::string const& __GetWorkDir() {
    PROFILE

    static auto _workDir = std::string();

    if (auto const currentDirectory = SDL_GetCurrentDirectory()) {
        _workDir = _TrimSeparator(currentDirectory);
        SDL_free(currentDirectory);
    }

    return _workDir;
}

std::string const& __GetUserDir() {
    PROFILE

    static auto const _userDir = [] {
        auto const configHome = std::getenv("XDG_CONFIG_HOME");

        if ((configHome != nullptr) && (configHome[0] == '/')) {
            return _TrimSeparator(configHome);
        }

        auto const homeFolder = SDL_GetUserFolder(SDL_FOLDER_HOME);

        return (homeFolder != nullptr) ? (_TrimSeparator(homeFolder) + "/.config") : std::string();
    }();

    return _userDir;
}

void __Exit() {
    PROFILE

    _isExited = true;
}

void __Run(int argc, char const** argv) {
    // PROFILE

    __InitVideo();

    _isExited = false;

    while (_isExited == false) {
        __Tick(true);
    }
}

void __Tick(bool update) {
    PROFILE

    __InitVideo();

    static thread_local auto _held = std::vector<SDL_Event>();

    auto event = SDL_Event{};

    for (;;) {
        auto const now = SDL_GetTicksNS();
        auto const wait = (_nextFrame > now) ? (_nextFrame - now) : 0;

        if (SDL_WaitEventTimeout(&event, static_cast<Sint32>((wait + 999999) / 1000000)) == false) {
            break;
        }

        if (_IsCoalescedEvent(event)) {
            _CoalesceEvent(_held, event);
            continue;
        }

        _HandleHeldEvents(_held);
        _HandleEvent(event);
        break;
    }

    while (SDL_PollEvent(&event)) {
        if (_IsCoalescedEvent(event)) {
            _CoalesceEvent(_held, event);
        } else {
            _HandleHeldEvents(_held);
            _HandleEvent(event);
        }
    }

    _HandleHeldEvents(_held);

    if (update) {
        _Update();
    }

    if (SDL_GetTicksNS() >= _nextFrame) {
        _nextFrame = (SDL_GetTicksNS() + _GetFramePeriod());
        __PaintWindows();
    }
}

} /* namespace App */
} /* namespace Rocket */
