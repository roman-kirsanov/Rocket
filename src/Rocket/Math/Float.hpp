/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace Rocket {

/** The mathematical constant π. */
float constexpr PI = 3.14159265358979323846f;

/**
 * Returns true if two floats are equal within an epsilon tolerance.
 *
 * @param a   First value.
 * @param b   Second value.
 * @param eps Maximum allowed absolute difference (default 1e-4).
 */
bool FloatIsEqualEps(float a, float b, float eps = 1e-4f);

} /* namespace Rocket */
