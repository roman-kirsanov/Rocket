/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <Rocket/Base/Enum.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Math/Mat3.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Shape.hpp>
#include <Rocket/Paint/Shadow.hpp>
#include <Rocket/Paint/Filter.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

class Window;

/**
 * A render target backed by an Image. The painter acquires and submits its
 * own command buffer; the texture format is the fixed 32-bit RGBA format
 * used for all offscreen Image textures.
 */
struct ImagePaintTarget {
    /** The image to render into. */
    Image& image;
    /** If set, the image is cleared to this straight (non-premultiplied) RGBA color at the start of the pass. */
    std::optional<Vec4> clearColor;
};

/**
 * A render target backed by a window. The painter acquires the window's
 * drawable and its own command buffer, and submits the command buffer
 * automatically on endPaint(). If the drawable cannot be acquired the
 * whole pass becomes a no-op.
 *
 * Note: the elaborated `class Window` spelling keeps the reference bound to
 * the window class even where the Window component function is in scope.
 */
struct WindowPaintTarget {
    /** The window to render into. */
    class Window& window;
    /** If set, the drawable is cleared to this straight (non-premultiplied) RGBA color at the start of the pass. */
    std::optional<Vec4> clearColor;
};

/** An image-backed or window-backed render target. */
using PaintTarget = Enum<ImagePaintTarget, WindowPaintTarget>;

/**
 * How a draw combines with what is already on the target. Fragments are
 * premultiplied, so every mode is a plain pair of blend factors.
 */
enum class Blend {
    /** Premultiplied source-over (the default): the source covers the target. */
    Over,
    /** Adds the source to the target: lights, flashes, glows. Clips at white. */
    Add,
    /** Scales the target by the source: lightmaps, shadows, tints. A transparent source leaves the target alone. */
    Multiply,
    /** 1 - (1 - target) * (1 - source): a softer add that never clips. */
    Screen
};

/** Per-draw overrides for transform, scissor rectangle, opacity, filtering, and blending. */
struct PaintOptions {
    /** Transform applied to the shape; identity when unset. */
    std::optional<Mat3> transform;
    /** Clip rectangle the draw is restricted to, clamped to the target bounds. */
    std::optional<Vec4> scissor;
    /** Opacity multiplier, 0 to 1; fully opaque when unset. */
    std::optional<float> opacity;
    /** Post-processing filter applied to the draw; nullopt disables filtering. */
    std::optional<Filter> filter;
    /** Blend mode the draw (or its filtered result) is composited with; Over when unset. */
    std::optional<Blend> blend;
};

/**
 * The main drawing interface.
 *
 * Non-copyable and non-movable. Each paint pass must be bracketed with
 * beginPaint() and endPaint(); paint() calls are only valid in between.
 * Passes may be nested: beginPaint() suspends the current pass, and
 * endPaint() resumes it. Nested passes share the outermost pass's command
 * buffer, so the GPU runs everything in the order it was painted and
 * nothing is submitted (or presented) until the outermost endPaint().
 * Destroying a Painter with passes still open
 * abandons them: nothing is submitted or presented, so every open target
 * keeps its previous content. Graphics pipelines are created lazily and
 * cached per texture format, so a single Painter instance can render to
 * targets with different formats without pipeline recreation.
 */
class Painter {
public:
    ~Painter();
    Painter();

    Painter(Painter &&) = delete;
    Painter(Painter const&) = delete;
    Painter& operator=(Painter &&) = delete;
    Painter& operator=(Painter const&) = delete;

    /**
     * Starts a paint pass on the given target. If a pass is already active it
     * is suspended (its render pass ended) and will be resumed by the matching
     * endPaint(). The pipeline set is chosen based on the target's texture
     * format.
     *
     * @param target The render target to draw into.
     */
    void beginPaint(PaintTarget const& target);

    /**
     * Finalizes the current paint pass and, if a pass was suspended by the
     * matching beginPaint(), resumes it. The pass's command buffer is
     * submitted automatically.
     */
    void endPaint();

    /**
     * Draws a shape filled with a brush and optional per-draw overrides.
     * Returns immediately (no-op) if no pass is active, the shape has zero
     * area (or, for outline shapes, no resolved border width above zero), or
     * the brush is degenerate (null image, zero alpha color, or gradient with
     * no start color, no stop color, and no stops).
     *
     * When a filter with effect is set (e.g. a BlurFilter with radius > 0),
     * the shape is rendered into an offscreen texture, filtered, and the
     * result is composited back onto the target through the draw's scissor
     * rectangle. A filter without effect draws the shape normally.
     *
     * A ShadowFilter renders a blurred, tinted, optionally offset and dilated
     * shadow of the shape's silhouette instead of the shape itself; callers
     * that want both must paint the shape in a separate call.
     *
     * Const because drawing never changes which target or pass is active, so
     * code handed a Painter const& can paint but cannot begin or end passes.
     *
     * @param shape   The shape to draw.
     * @param brush   The brush to fill the shape with.
     * @param options Transform, scissor, opacity, and filter overrides (default none).
     */
    void paint(Shape const& shape, Brush const& brush, PaintOptions const& options = {}) const;
private:
    struct _Painter;
    _Painter* _impl;

    void __init();
    void __done();
    void __beginPaint(PaintTarget const&);
    void __endPaint();
    void __paint(Shape const&, Brush const&, PaintOptions const&, Vec4 const& bounds) const;
};

} /* namespace Rocket */
