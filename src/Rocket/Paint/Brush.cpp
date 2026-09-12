/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Brush.hpp>

namespace Rocket {

bool ColorBrush::operator==(ColorBrush const& colorBrush) const {
    PROFILE

    return (color == colorBrush.color);
}

bool ColorBrush::operator!=(ColorBrush const& colorBrush) const {
    PROFILE

    return !operator==(colorBrush);
}

bool ImageBrush::operator==(ImageBrush const& imageBrush) const {
    PROFILE

    return (image == imageBrush.image)
        && (color == imageBrush.color)
        && (slice == imageBrush.slice)
        && (nPatch == imageBrush.nPatch)
        && (positionX == imageBrush.positionX)
        && (positionY == imageBrush.positionY)
        && (filterMag == imageBrush.filterMag)
        && (filterMin == imageBrush.filterMin)
        && (repeatX == imageBrush.repeatX)
        && (repeatY == imageBrush.repeatY)
        && (flipX == imageBrush.flipX)
        && (flipY == imageBrush.flipY)
        && (fit == imageBrush.fit);
}

bool ImageBrush::operator!=(ImageBrush const& imageBrush) const {
    PROFILE

    return !operator==(imageBrush);
}

bool GradientBrush::operator==(GradientBrush const& gradientBrush) const {
    PROFILE

    return (radial == gradientBrush.radial)
        && (startPosition == gradientBrush.startPosition)
        && (startColor == gradientBrush.startColor)
        && (stopPosition == gradientBrush.stopPosition)
        && (stopColor == gradientBrush.stopColor)
        && (stops == gradientBrush.stops);
}

bool GradientBrush::operator!=(GradientBrush const& gradientBrush) const {
    PROFILE

    return !operator==(gradientBrush);
}

} /* namespace Rocket */
