/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>

namespace Rocket {

/**
 * Returns the current wall-clock time as the number of milliseconds elapsed
 * since the Unix epoch (1970-01-01 UTC), matching JavaScript's Date.now().
 */
std::int64_t GetTime();

} /* namespace Rocket */
