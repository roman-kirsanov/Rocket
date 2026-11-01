/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <Rocket/Window/Input.hpp>

namespace Rocket {

bool __IsCharacterScancode(Scancode scancode);
std::string __GetLayoutKeycode(Scancode scancode);

} /* namespace Rocket */
