/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Filter.hpp>

namespace Rocket {

bool BlurFilter::operator==(BlurFilter const& filter) const {
    PROFILE

    return (radius == filter.radius);
}

bool BlurFilter::operator!=(BlurFilter const& filter) const {
    PROFILE

    return !operator==(filter);
}

bool ShadowFilter::operator==(ShadowFilter const& filter) const {
    PROFILE

    return (
        (radius == filter.radius) &&
        (color == filter.color) &&
        (offset == filter.offset) &&
        (spread == filter.spread) &&
        (inset == filter.inset)
    );
}

bool ShadowFilter::operator!=(ShadowFilter const& filter) const {
    PROFILE

    return !operator==(filter);
}

} /* namespace Rocket */
