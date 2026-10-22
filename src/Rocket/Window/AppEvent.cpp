/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/AppEvent.hpp>

namespace Rocket {

GamepadConnectAppEvent::GamepadConnectAppEvent(std::uint32_t gamepad)
    : _gamepad(gamepad)
{
    PROFILE
}

std::uint32_t GamepadConnectAppEvent::getGamepad() const {
    PROFILE

    return _gamepad;
}

GamepadDisconnectAppEvent::GamepadDisconnectAppEvent(std::uint32_t gamepad)
    : _gamepad(gamepad)
{
    PROFILE
}

std::uint32_t GamepadDisconnectAppEvent::getGamepad() const {
    PROFILE

    return _gamepad;
}

} /* namespace Rocket */
