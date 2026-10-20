/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Window/Clipboard.hpp>

namespace Rocket {

std::string __GetClipboardString();

void __SetClipboardString(std::string const& string);

} /* namespace Rocket */