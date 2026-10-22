/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>
#include <Rocket/Window/Input.hpp>

namespace Rocket {

/** Sends a gamepad button press to the focused window; it is dropped when no window has focus. */
void _GamepadDown(std::uint32_t gamepad, GamepadButton button);

/** Sends a gamepad button release to the focused window; it is dropped when no window has focus. */
void _GamepadUp(std::uint32_t gamepad, GamepadButton button);

/** Sends a gamepad axis move to the focused window; it is dropped when no window has focus. */
void _GamepadAxis(std::uint32_t gamepad, GamepadAxis axis, float value);

/** Starts the gamepad backend; App::Run calls it before ReadyAppEvent. */
void __InitGamepads();

/** Stops the gamepad backend; App::Run calls it after the run loop ends. */
void __DoneGamepads();

/**
 * Reads the gamepads and sends what changed. A run loop that does not read
 * SDL's event queue itself calls it on every tick; the SDL3 run loop hands
 * the gamepad events it reads to __HandleGamepadEvent instead.
 */
void __UpdateGamepads();

} /* namespace Rocket */
