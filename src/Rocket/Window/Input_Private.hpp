/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <Rocket/Window/Input.hpp>

namespace Rocket {

/**
 * Whether the key types a character: the digit row, the letters, the
 * punctuation keys and Space. These are the keys whose keycode follows the
 * layout; every other key keeps its name.
 */
bool __IsCharacterScancode(Scancode scancode);

/** The keycode the scancode has on the current layout, without modifiers. */
std::string __GetLayoutKeycode(Scancode scancode);

} /* namespace Rocket */
