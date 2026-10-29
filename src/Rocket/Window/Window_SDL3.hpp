/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <SDL3/SDL.h>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

/** Input goes to the innermost modal window only. */
void __HandleEvent(SDL_Event const& event);

/** Returns the highest refresh rate of the displays the painted windows are on, or 0 when no window is painted or no rate is known. */
float __GetRefreshRate();

/** Paints only shown, unminimized, unoccluded windows. */
void __PaintWindows();

} /* namespace Rocket */
