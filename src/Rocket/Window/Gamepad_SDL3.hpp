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

/** Whether the event is one of SDL's gamepad events, which __HandleGamepadEvent handles. */
bool __IsGamepadEvent(SDL_Event const& event);

/** Sends one of SDL's gamepad events on: a connect or disconnect to App::OnEvent, a button or axis to the focused window. */
void __HandleGamepadEvent(SDL_Event const& event);

} /* namespace Rocket */
