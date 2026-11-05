/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/UI/ColorPalette.hpp>

namespace Rocket {

/** Returns the palette the Default themes are built on. Built on first use. */
ColorPalette const& GetDefaultPalette();

} /* namespace Rocket */
