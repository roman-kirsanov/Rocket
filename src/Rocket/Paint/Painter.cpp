/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

// Resolves a quad outline's per-edge widths (left, top, right, bottom), applying
// the uniform border as the fallback for any edge without an explicit override.
static Vec4 _ResolveBorders(QuadOutlineShape const& shape) {
    PROFILE

    auto const border = shape.border.value_or(0.0f);

    return Vec4{
        shape.leftBorder.value_or(border),
        shape.topBorder.value_or(border),
        shape.rightBorder.value_or(border),
        shape.bottomBorder.value_or(border)
    };
}

Painter::~Painter() {
    PROFILE

    __done();
}

Painter::Painter()
    : _impl(nullptr)
{
    PROFILE

    __init();
}

void Painter::beginPaint(PaintTarget const& target) {
    PROFILE

    __beginPaint(target);
}

void Painter::endPaint() {
    PROFILE

    __endPaint();
}

void Painter::paint(Shape const& shape, Brush const& brush, PaintOptions const& options) {
    PROFILE

    auto const colorBrush = brush.as<ColorBrush>();
    auto const imageBrush = brush.as<ImageBrush>();
    auto const gradientBrush = brush.as<GradientBrush>();

    auto shapeRect = Vec4{};
    auto shapeValid = false;

    shape.match(
        [&](QuadShape const& quadShape) {
            shapeRect  = quadShape.rect;
            shapeValid = (quadShape.rect.width > 0.0f) && (quadShape.rect.height > 0.0f);
        },
        [&](EllipseShape const& ellipseShape) {
            shapeRect  = ellipseShape.rect;
            shapeValid = (ellipseShape.rect.width > 0.0f) && (ellipseShape.rect.height > 0.0f);
        },
        [&](QuadOutlineShape const& quadOutlineShape) {
            auto const borders = _ResolveBorders(quadOutlineShape);
            shapeRect  = quadOutlineShape.rect;
            shapeValid = (quadOutlineShape.rect.width > 0.0f)
                      && (quadOutlineShape.rect.height > 0.0f)
                      && ((borders.left > 0.0f) || (borders.top > 0.0f) || (borders.right > 0.0f) || (borders.bottom > 0.0f));
        },
        [&](EllipseOutlineShape const& ellipseOutlineShape) {
            shapeRect  = ellipseOutlineShape.rect;
            shapeValid = (ellipseOutlineShape.rect.width > 0.0f)
                      && (ellipseOutlineShape.rect.height > 0.0f)
                      && (ellipseOutlineShape.border.value_or(0.0f) > 0.0f);
        }
    );

    if (shapeValid == false) {
        return;
    }

    if (colorBrush != nullptr) {
        if (colorBrush->color.alpha == 0) {
            return;
        }
    } else if (imageBrush != nullptr) {
        if (imageBrush->image == nullptr) {
            return;
        }
    } else if (gradientBrush != nullptr) {
        if (
            (gradientBrush->startColor.has_value() == false) &&
            (gradientBrush->stopColor.has_value() == false) &&
            (gradientBrush->stops.size() == 0)
        ) {
            return;
        }
    } else {
        return;
    }

    __paint(shape, brush, options, shapeRect);
}

} /* namespace Rocket */
