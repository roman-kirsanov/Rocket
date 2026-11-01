/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <vector>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

std::vector<void*> __GetPaintedWindows();
void __PaintWindows(std::vector<void*> const& handles);
void __PaintWindows();

} /* namespace Rocket */
