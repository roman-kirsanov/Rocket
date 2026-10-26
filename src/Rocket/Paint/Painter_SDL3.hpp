/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>

struct SDL_Window;

/* What an SDL3 window needs from the painter it is drawn with: SDL_GPU
   (Painter_SDL3.cpp) or OpenGL ES (Painter_GLES.cpp), chosen when Rocket is
   built. */

namespace Rocket {

/** Makes a new window a target of the painter; false if it cannot, with the reason in SDL_GetError(). */
bool __ClaimWindow(SDL_Window* window);

/** Lets go of a window the painter claimed, before the window is destroyed. */
void __ReleaseWindow(SDL_Window* window);

/** Returns the SDL_WindowFlags every window is created with, so the painter can draw into it. */
std::uint64_t __GetWindowFlags();

} /* namespace Rocket */
