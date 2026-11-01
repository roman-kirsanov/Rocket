/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>

struct SDL_Window;

namespace Rocket {

bool __ClaimWindow(::SDL_Window* window);
void __ReleaseWindow(::SDL_Window* window);
std::uint64_t __GetWindowFlags();

} /* namespace Rocket */
