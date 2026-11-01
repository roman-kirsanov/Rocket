/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <string_view>

namespace Rocket {
namespace App {

void __InitWin32();

void __SetTimerPainting(bool painting);

std::wstring __ToWide(std::string_view string);

std::string __FromWide(std::wstring_view string);

} /* namespace App */
} /* namespace Rocket */
