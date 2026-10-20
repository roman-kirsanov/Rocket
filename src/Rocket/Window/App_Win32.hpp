/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <string_view>

/* <windows.h> stays out of this header: it defines macros (interface, min,
   max) that would leak into every file including it. */

namespace Rocket {
namespace App {

/**
 * Must run before any window exists, the clipboard's included (DPI
 * awareness), and on each thread that uses COM. Safe to call any number of times.
 */
void __InitWin32();

/**
 * Paints from a timer during the loops Windows runs inside a call of its own
 * (a move or resize, a menu, a file dialog), where __Tick does not run. Calls nest.
 */
void __SetTimerPainting(bool painting);

std::wstring __ToWide(std::string_view string);

std::string __FromWide(std::wstring_view string);

} /* namespace App */
} /* namespace Rocket */
