/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <SDL3/SDL.h>
#include <Rocket/Window/Gamepad_Private.hpp>

namespace Rocket {

bool __IsGamepadEvent(SDL_Event const& event);
void __HandleGamepadEvent(SDL_Event const& event);

} /* namespace Rocket */
