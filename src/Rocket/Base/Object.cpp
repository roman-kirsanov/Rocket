/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/Object.hpp>

namespace Rocket {

Object::~Object() {
    PROFILE

    onDestroy.publish();
}

Object::Object()
    : onDestroy()
{
    PROFILE
}

} /* namespace Rocket */
