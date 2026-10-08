/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <algorithm>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Filter.hpp>

namespace Rocket {

/* both kernels run out to three standard deviations; the radius-to-sigma
   mapping differs because it follows the CSS feature each filter mirrors */
static float _GetGaussianExtent(float sigma) {
    return std::ceil(sigma * 3.0f);
}

float BlurFilter::getSigma() const {
    return std::max(radius, 0.0f);
}

float BlurFilter::getExtent() const {
    return _GetGaussianExtent(getSigma());
}

bool BlurFilter::operator==(BlurFilter const& filter) const {
    PROFILE

    return (radius == filter.radius);
}

bool BlurFilter::operator!=(BlurFilter const& filter) const {
    PROFILE

    return !operator==(filter);
}

float ShadowFilter::getSigma() const {
    return (std::max(radius, 0.0f) / 2.0f);
}

float ShadowFilter::getExtent() const {
    return _GetGaussianExtent(getSigma());
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
