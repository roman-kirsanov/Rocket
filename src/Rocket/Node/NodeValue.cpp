/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Node/NodeValue.hpp>

namespace Rocket {

bool PixelValue::operator==(PixelValue const& other) const {
    PROFILE

    return (value == other.value);
}

bool PixelValue::operator!=(PixelValue const& other) const {
    PROFILE

    return (value != other.value);
}

bool PercentValue::operator==(PercentValue const& other) const {
    PROFILE

    return (value == other.value);
}

bool PercentValue::operator!=(PercentValue const& other) const {
    PROFILE

    return (value != other.value);
}

} /* namespace Rocket */
