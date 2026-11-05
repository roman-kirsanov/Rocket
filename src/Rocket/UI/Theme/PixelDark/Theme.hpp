/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/UI/Theme.hpp>

namespace Rocket {

/** Returns the PixelDark theme: every component drawn dark with the PixelDark material (see GetPixelDarkMaterial). Built on first use. */
Theme const& GetPixelDarkTheme();

} /* namespace Rocket */
