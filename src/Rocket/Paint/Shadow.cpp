/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Shadow.hpp>

namespace Rocket {

bool Shadow::operator==(Shadow const& shadow) const {
    PROFILE

    return (color == shadow.color)
        && (offset == shadow.offset)
        && (spread == shadow.spread)
        && (blur == shadow.blur)
        && (inset == shadow.inset);
}

bool Shadow::operator!=(Shadow const& shadow) const {
    PROFILE

    return !operator==(shadow);
}

} /* namespace Rocket */
