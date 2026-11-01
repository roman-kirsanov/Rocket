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

void __HandleEvent(::SDL_Event const& event);
float __GetRefreshRate();
void __PaintWindows();

} /* namespace Rocket */
