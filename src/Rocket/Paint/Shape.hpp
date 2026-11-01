/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <Rocket/Base/Enum.hpp>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/**
 * A filled rectangle shape with optional rounded corners.
 *
 * borderRadius sets a uniform corner radius in pixels for all corners.
 * Per-corner overrides (borderTopLeftRadius, borderTopRightRadius,
 * borderBottomLeftRadius, borderBottomRightRadius) take precedence when
 * set. Unset (or all-zero) radii mean sharp corners.
 */
struct QuadShape {
    /** Bounding rectangle (x, y, width, height). */
    Vec4 rect;
    /** Uniform corner radius in pixels applied to all corners. */
    std::optional<float> borderRadius;
    /** Top-left corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderTopLeftRadius;
    /** Top-right corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderTopRightRadius;
    /** Bottom-left corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderBottomLeftRadius;
    /** Bottom-right corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderBottomRightRadius;
    /**
     * Returns true when every field equals the other shape's.
     *
     * @param quadShape The shape to compare with.
     */
    bool operator==(QuadShape const& quadShape) const;
    /**
     * Returns true when any field differs from the other shape's.
     *
     * @param quadShape The shape to compare with.
     */
    bool operator!=(QuadShape const& quadShape) const;
};

/** A filled ellipse shape inscribed into its bounding rectangle. */
struct EllipseShape {
    /** Bounding rectangle (x, y, width, height) the ellipse is inscribed into. */
    Vec4 rect;
    /**
     * Returns true when every field equals the other shape's.
     *
     * @param ellipseShape The shape to compare with.
     */
    bool operator==(EllipseShape const& ellipseShape) const;
    /**
     * Returns true when any field differs from the other shape's.
     *
     * @param ellipseShape The shape to compare with.
     */
    bool operator!=(EllipseShape const& ellipseShape) const;
};

/**
 * A rectangle outline shape with configurable per-edge border widths.
 *
 * border sets a uniform width for all edges. Per-edge overrides
 * (leftBorder, topBorder, rightBorder, bottomBorder) take precedence
 * when set.
 *
 * borderRadius sets a uniform outer corner radius in pixels for all
 * corners. Per-corner overrides (borderTopLeftRadius, borderTopRightRadius,
 * borderBottomLeftRadius, borderBottomRightRadius) take precedence when
 * set. Unset (or all-zero) radii mean sharp corners.
 */
struct QuadOutlineShape {
    /** Bounding rectangle (x, y, width, height). */
    Vec4 rect;
    /** Uniform outer corner radius in pixels applied to all corners. */
    std::optional<float> borderRadius;
    /** Top-left outer corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderTopLeftRadius;
    /** Top-right outer corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderTopRightRadius;
    /** Bottom-left outer corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderBottomLeftRadius;
    /** Bottom-right outer corner radius in pixels; overrides borderRadius when set. */
    std::optional<float> borderBottomRightRadius;
    /** Uniform border width applied to all edges. */
    std::optional<float> border;
    /** Left edge border width; overrides border when set. */
    std::optional<float> leftBorder;
    /** Top edge border width; overrides border when set. */
    std::optional<float> topBorder;
    /** Right edge border width; overrides border when set. */
    std::optional<float> rightBorder;
    /** Bottom edge border width; overrides border when set. */
    std::optional<float> bottomBorder;
    /**
     * Returns true when every field equals the other shape's.
     *
     * @param frameShape The shape to compare with.
     */
    bool operator==(QuadOutlineShape const& frameShape) const;
    /**
     * Returns true when any field differs from the other shape's.
     *
     * @param frameShape The shape to compare with.
     */
    bool operator!=(QuadOutlineShape const& frameShape) const;
};

/** An ellipse outline shape (a ring) inscribed into its bounding rectangle. */
struct EllipseOutlineShape {
    /** Bounding rectangle (x, y, width, height) the ellipse is inscribed into. */
    Vec4 rect;
    /** Ring thickness in pixels. */
    std::optional<float> border;
    /**
     * Returns true when every field equals the other shape's.
     *
     * @param ellipseOutlineShape The shape to compare with.
     */
    bool operator==(EllipseOutlineShape const& ellipseOutlineShape) const;
    /**
     * Returns true when any field differs from the other shape's.
     *
     * @param ellipseOutlineShape The shape to compare with.
     */
    bool operator!=(EllipseOutlineShape const& ellipseOutlineShape) const;
};

/** A filled or outlined rectangle or ellipse shape. */
using Shape = Enum<QuadShape, EllipseShape, QuadOutlineShape, EllipseOutlineShape>;

} /* namespace Rocket */
