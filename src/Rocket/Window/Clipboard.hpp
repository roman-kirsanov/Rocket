/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>

namespace Rocket {

/**
 * Returns the system clipboard's current text contents.
 *
 * Returns an empty string when the clipboard is empty or holds no text.
 */
std::string GetClipboardString();

/**
 * Replaces the system clipboard contents with the given text.
 *
 * @param string The UTF-8 text to place on the clipboard.
 */
void SetClipboardString(std::string const& string);

} /* namespace Rocket */
