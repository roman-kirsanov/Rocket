/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/Clipboard_Private.hpp>

namespace Rocket {

std::string GetClipboardString() {
    PROFILE

    return __GetClipboardString();
}

void SetClipboardString(std::string const& string) {
    PROFILE

    __SetClipboardString(string);
}

} /* namespace Rocket */