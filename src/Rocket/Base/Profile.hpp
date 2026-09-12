/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define PROFILE ZoneScoped;
#else
#define PROFILE
#endif

namespace Rocket {

} /* namespace Rocket */
