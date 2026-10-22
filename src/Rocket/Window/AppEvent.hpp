/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>
#include <Rocket/Base/Object.hpp>

namespace Rocket {

/** Base class for application lifecycle events published on App::OnEvent. */
class AppEvent : public Object {};

/** Published once after the run loop ends, just before App::Run returns. */
class ExitAppEvent : public AppEvent {};

/** Published once at the start of App::Run, before the loop begins. */
class ReadyAppEvent : public AppEvent {};

/** Published when the user asks the application to quit (e.g. the Quit menu item); call App::Exit to honor it. */
class CloseAppEvent : public AppEvent {};

/** Published on every run-loop tick, after timeouts and intervals have run. */
class UpdateAppEvent : public AppEvent {};

/** Published when a gamepad is connected, and for every gamepad already connected when App::Run starts. */
class GamepadConnectAppEvent : public AppEvent {
public:
    /**
     * An event for the given gamepad.
     *
     * @param gamepad The gamepad's id: it names the gamepad in its button and axis events until it is disconnected.
     */
    GamepadConnectAppEvent(std::uint32_t gamepad);

    /** Returns the gamepad's id. */
    std::uint32_t getGamepad() const;
private:
    std::uint32_t _gamepad;
};

/** Published when a gamepad is disconnected. Its buttons are released and its axes centered before it. */
class GamepadDisconnectAppEvent : public AppEvent {
public:
    /**
     * An event for the given gamepad.
     *
     * @param gamepad The id the gamepad had (see GamepadConnectAppEvent).
     */
    GamepadDisconnectAppEvent(std::uint32_t gamepad);

    /** Returns the id the gamepad had. */
    std::uint32_t getGamepad() const;
private:
    std::uint32_t _gamepad;
};

} /* namespace Rocket */
