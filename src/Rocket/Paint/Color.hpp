/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <cstdint>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/**
 * Returns the hex string representation of a color (e.g. "#rrggbbaa").
 *
 * @param color The color to convert.
 */
std::string ColorToHex(Vec4 const& color);

/**
 * Returns a color parsed from a hex string (e.g. "#rgb", "#rgba", "#rrggbb",
 * "#rrggbbaa"), or transparent black if the string is not one of those forms.
 *
 * @param hex The hex color string.
 */
Vec4 ColorFromHex(std::string const& hex);

/**
 * Returns a color from integer RGB components with full opacity.
 *
 * @param red    Red channel, clamped to 0–255.
 * @param green  Green channel, clamped to 0–255.
 * @param blue   Blue channel, clamped to 0–255.
 */
Vec4 ColorFromRGB(std::int32_t const& red, std::int32_t const& green, std::int32_t const& blue);

/**
 * Returns a color from integer RGB components and a float alpha.
 *
 * @param red    Red channel, clamped to 0–255.
 * @param green  Green channel, clamped to 0–255.
 * @param blue   Blue channel, clamped to 0–255.
 * @param alpha  Alpha channel, 0.0–1.0.
 */
Vec4 ColorFromRGBA(std::int32_t const& red, std::int32_t const& green, std::int32_t const& blue, float const& alpha);

/**
 * Predefined color constants in sRGB RGBA (0.0-1.0 per channel).
 */
auto constexpr COLOR_BLACK = Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
auto constexpr COLOR_WHITE = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
auto constexpr COLOR_TRANSPARENT = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

} /* namespace Rocket */
