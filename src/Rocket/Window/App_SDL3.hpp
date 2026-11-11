/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>

namespace Rocket {
namespace App {

void __InitVideo();

std::uint64_t __GetNextFrame(std::uint64_t paintedAt, std::uint64_t period);

} /* namespace App */
} /* namespace Rocket */
