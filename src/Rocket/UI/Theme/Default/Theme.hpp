/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/UI/Theme.hpp>

namespace Rocket {

/** Returns the default dark theme: every component drawn with plain colors from the default palette, and no material. Built on first use. */
Theme const& GetDefaultDarkTheme();

/** Returns the default light theme: every component drawn with plain colors from the default palette, and no material. Built on first use. */
Theme const& GetDefaultLightTheme();

} /* namespace Rocket */
