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

void _GamepadDown(std::uint32_t gamepad, GamepadButton button);
void _GamepadUp(std::uint32_t gamepad, GamepadButton button);
void _GamepadAxis(std::uint32_t gamepad, GamepadAxis axis, float value);

void __InitGamepads();
void __DoneGamepads();
void __UpdateGamepads();

} /* namespace Rocket */
