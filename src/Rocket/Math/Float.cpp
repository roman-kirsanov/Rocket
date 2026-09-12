/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Math/Float.hpp>

namespace Rocket {

bool FloatIsEqualEps(float a, float b, float eps) {
    PROFILE

    return std::fabs(a - b) <= eps;
}

} /* namespace Rocket */
