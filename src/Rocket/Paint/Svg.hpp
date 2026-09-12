/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/**
 * Rasterizes an SVG document into a raw RGBA pixel buffer.
 *
 * @param svg    The SVG source text.
 * @param size   Output image dimensions in pixels.
 * @param color  Color set as the SVG `color` CSS property before rasterization; ignored when alpha is 0.
 * @param buffer Output buffer filled with the RGBA pixel data.
 */
void ConvertSVGToBitmap(std::string const& svg, Vec2 const& size, Vec4 const& color, std::vector<std::uint8_t>& buffer);

} /* namespace Rocket */
