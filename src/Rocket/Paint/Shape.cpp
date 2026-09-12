/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Shape.hpp>

namespace Rocket {

bool QuadShape::operator==(QuadShape const& quadShape) const {
    PROFILE

    return (rect == quadShape.rect)
        && (borderRadius == quadShape.borderRadius)
        && (borderTopLeftRadius == quadShape.borderTopLeftRadius)
        && (borderTopRightRadius == quadShape.borderTopRightRadius)
        && (borderBottomLeftRadius == quadShape.borderBottomLeftRadius)
        && (borderBottomRightRadius == quadShape.borderBottomRightRadius);
}

bool QuadShape::operator!=(QuadShape const& quadShape) const {
    PROFILE

    return !operator==(quadShape);
}

bool EllipseShape::operator==(EllipseShape const& ellipseShape) const {
    PROFILE

    return (rect == ellipseShape.rect);
}

bool EllipseShape::operator!=(EllipseShape const& ellipseShape) const {
    PROFILE

    return !operator==(ellipseShape);
}

bool QuadOutlineShape::operator==(QuadOutlineShape const& frameShape) const {
    PROFILE

    return (rect == frameShape.rect)
        && (borderRadius == frameShape.borderRadius)
        && (borderTopLeftRadius == frameShape.borderTopLeftRadius)
        && (borderTopRightRadius == frameShape.borderTopRightRadius)
        && (borderBottomLeftRadius == frameShape.borderBottomLeftRadius)
        && (borderBottomRightRadius == frameShape.borderBottomRightRadius)
        && (border == frameShape.border)
        && (leftBorder == frameShape.leftBorder)
        && (topBorder == frameShape.topBorder)
        && (rightBorder == frameShape.rightBorder)
        && (bottomBorder == frameShape.bottomBorder);
}

bool QuadOutlineShape::operator!=(QuadOutlineShape const& frameShape) const {
    PROFILE

    return !operator==(frameShape);
}

bool EllipseOutlineShape::operator==(EllipseOutlineShape const& ellipseOutlineShape) const {
    PROFILE

    return (rect == ellipseOutlineShape.rect)
        && (border == ellipseOutlineShape.border);
}

bool EllipseOutlineShape::operator!=(EllipseOutlineShape const& ellipseOutlineShape) const {
    PROFILE

    return !operator==(ellipseOutlineShape);
}

} /* namespace Rocket */
