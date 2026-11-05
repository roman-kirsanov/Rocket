/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace Storybook {

/** Light or dark look of the Storybook; provided as a context by App. */
enum class ColorScheme {
    Light,
    Dark
};

/**
 * Returns the color scheme provided by the nearest ColorScheme context, or
 * Light when there is none. Must be called during a render.
 */
ColorScheme UseColorScheme();

} /* namespace Storybook */
