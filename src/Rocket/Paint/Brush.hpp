/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <tuple>
#include <vector>
#include <optional>
#include <Rocket/Base/Enum.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

/** A brush that fills with a solid color. */
struct ColorBrush {
    /** RGBA fill color. */
    Vec4 color;
    bool operator==(ColorBrush const&) const;
    bool operator!=(ColorBrush const&) const;
};

/**
 * A brush that fills with an image, with optional slicing, tinting,
 * positioning, sampling filters, tiling, flipping, and fit behavior.
 */
struct ImageBrush {
    /** Source image; a null image makes the brush degenerate and paint() no-ops. */
    Image const* image;
    /** Sub-region of the image to use (x, y, width, height). */
    std::optional<Vec4> slice;
    /** 9-patch inset (left, top, right, bottom) for stretchable borders. */
    std::optional<Vec4> nPatch;
    /** RGBA color that replaces image pixel RGB; its alpha is multiplied with the image pixel alpha. Ignored entirely (no tint) when its alpha is 0. */
    std::optional<Vec4> color;
    /** Horizontal alignment of the image within the shape. */
    std::optional<ImagePosition> positionX;
    /** Vertical alignment of the image within the shape. */
    std::optional<ImagePosition> positionY;
    /** Sampling filter used when the image is magnified. */
    std::optional<ImageFilter> filterMag;
    /** Sampling filter used when the image is minified. */
    std::optional<ImageFilter> filterMin;
    /** Tile the image horizontally when true. */
    std::optional<bool> repeatX;
    /** Tile the image vertically when true. */
    std::optional<bool> repeatY;
    /** Mirror the image horizontally when true. */
    std::optional<bool> flipX;
    /** Mirror the image vertically when true. */
    std::optional<bool> flipY;
    /** Shrink the image to fit inside the shape while preserving aspect ratio when true; never upscales. Has no effect on an axis whose position is ImagePosition::Stretch (the default). */
    std::optional<bool> fit;
    bool operator==(ImageBrush const&) const;
    bool operator!=(ImageBrush const&) const;
};

/**
 * A brush that fills with a linear or radial gradient.
 *
 * Simple two-stop gradients use startColor/stopColor with start/stopPosition.
 * Multi-stop gradients use the stops vector of (position, color) pairs.
 */
struct GradientBrush {
    /** Radial gradient when true; linear when false or unset. */
    std::optional<bool> radial;
    /** Start point (or center for radial) in normalized (0.0–1.0) local shape coordinates. */
    std::optional<Vec2> startPosition;
    /** End point (or radius endpoint for radial) in normalized (0.0–1.0) local shape coordinates. */
    std::optional<Vec2> stopPosition;
    /** Color at the start of a two-stop gradient. */
    std::optional<Vec4> startColor;
    /** Color at the end of a two-stop gradient. */
    std::optional<Vec4> stopColor;
    /** Multi-stop list of (position 0–1, color) pairs; overrides startColor/stopColor when non-empty. At most the first 10 stops are used. */
    std::vector<std::tuple<float, Vec4>> stops;
    bool operator==(GradientBrush const&) const;
    bool operator!=(GradientBrush const&) const;
};

/** A color, image, or gradient brush. */
using Brush = Enum<ColorBrush, ImageBrush, GradientBrush>;

} /* namespace Rocket */
