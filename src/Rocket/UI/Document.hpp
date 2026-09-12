/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <optional>
#include <functional>
#include <Rocket/Node/Document.hpp>

namespace Rocket {

/**
 * Declarative props for Document.
 *
 * Rendering Document owns a retained Document (bound to the Window context)
 * for as long as the call site stays mounted, and syncs it to these props on
 * every render. An unset scale leaves the document at the scale adopted from
 * the window.
 */
struct DocumentProps {
    /** When non-null, filled with a pointer to the underlying Document while mounted; reset to nullptr when a different ref is passed, and on unmount unless another Document component has since claimed the same slot. */
    class Document** ref = nullptr;

    /** Reconciliation key for this component instance (see COMPONENT). */
    std::string key;

    /** Scale factor used for layout and rendering; unset keeps the window's scale. */
    std::optional<float> scale;

    /**
     * Node props applied to the document's root node. Only props meaningful
     * on a root are exposed: sizing, margin, positioning, content, etc. are
     * driven by the window and omitted.
     */
    struct {
        /** Horizontal overflow behaviour (hidden / visible / scroll). */
        std::optional<NodeOverflow> overflowX;

        /** Vertical overflow behaviour (hidden / visible / scroll). */
        std::optional<NodeOverflow> overflowY;

        /** Main-axis direction for children (Horizontal / Vertical, or their Reverse variants). */
        std::optional<NodeDirection> direction;

        /** Cross-axis alignment of children. */
        std::optional<NodeAlignment> alignment;

        /** Main-axis distribution of children. */
        std::optional<NodeJustify> justify;

        /** Left padding. */
        std::optional<NodeValue> paddingLeft;

        /** Top padding. */
        std::optional<NodeValue> paddingTop;

        /** Right padding. */
        std::optional<NodeValue> paddingRight;

        /** Bottom padding. */
        std::optional<NodeValue> paddingBottom;

        /** Horizontal gap between children. */
        std::optional<NodeValue> gapX;

        /** Vertical gap between children. */
        std::optional<NodeValue> gapY;

        /** Opacity in [0, 1] applied to the document and its subtree. */
        std::optional<float> opacity;

        /** Mouse cursor shown while hovering the document. */
        std::optional<Cursor> cursor;

        /** Font family name; inherited by descendant text. */
        std::optional<std::string> fontFamily;

        /** Font weight; inherited by descendant text. */
        std::optional<FontWeight> fontWeight;

        /** Font style; inherited by descendant text. */
        std::optional<FontStyle> fontStyle;

        /** Font size in points; inherited by descendant text. */
        std::optional<float> fontSize;

        /** Line height as a unitless multiplier of the font size; inherited by descendant text. */
        std::optional<float> lineHeight;

        /** Text colour (RGBA); inherited by descendant text. */
        std::optional<Vec4> textColor;

        /** Background brush, painted behind the children. */
        std::optional<Brush> background;

        /** Foreground brush, painted over the children. */
        std::optional<Brush> foreground;

        /** Border brush, painted along the border widths. */
        std::optional<Brush> border;
    } nodeProps;
};

/**
 * Renders a document: mounts a retained Document bound to the nearest
 * Window context on first render, keeps it in sync with `props` on every
 * render, and unmounts (destroying it) when this call site stops rendering.
 *
 * Must be called during a render (see COMPONENT), inside a Window context
 * (see UseContext<Window>). Exposes the document as a context via
 * SetContext, so Node components inside `children` parent under it. The
 * document is laid out (Document::update) on the reconciler's onAfterUpdate
 * — once per completed reconciliation, after stale components have
 * unmounted — so computed layout is available by the time
 * Reconciler::update() returns. Painting happens later, on the window's next
 * PaintWindowEvent (display-link paced), not during update().
 *
 * @param props    Desired document configuration for this render.
 * @param children Rendered inside the document's context; default no-op.
 */
void Document(DocumentProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
