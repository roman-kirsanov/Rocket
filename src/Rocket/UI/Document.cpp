/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Document.hpp>
#include <Rocket/UI/Node_Private.hpp>

namespace Rocket {

void Document(DocumentProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    struct _State {
        class Document** ref = nullptr;
        class Document document;
        _NodeContext context;
        Effect<std::optional<float>> shouldUpdateScale;
        Effect<std::optional<NodeOverflow>> shouldUpdateOverflowX;
        Effect<std::optional<NodeOverflow>> shouldUpdateOverflowY;
        Effect<std::optional<NodeDirection>> shouldUpdateDirection;
        Effect<std::optional<NodeAlignment>> shouldUpdateAlignment;
        Effect<std::optional<NodeJustify>> shouldUpdateJustify;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingTop;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingLeft;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingRight;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingBottom;
        Effect<std::optional<NodeValue>> shouldUpdateGapX;
        Effect<std::optional<NodeValue>> shouldUpdateGapY;
        Effect<std::optional<float>> shouldUpdateOpacity;
        Effect<std::optional<Cursor>> shouldUpdateCursor;
        Effect<std::optional<std::string>> shouldUpdateFontFamily;
        Effect<std::optional<FontWeight>> shouldUpdateFontWeight;
        Effect<std::optional<FontStyle>> shouldUpdateFontStyle;
        Effect<std::optional<float>> shouldUpdateFontSize;
        Effect<std::optional<float>> shouldUpdateLineHeight;
        Effect<std::optional<Vec4>> shouldUpdateTextColor;
        Effect<std::optional<Brush>> shouldUpdateBackground;
        Effect<std::optional<Brush>> shouldUpdateForeground;
        Effect<std::optional<Brush>> shouldUpdateBorder;
        Sub<> onAfterUpdate;

        _State(Reconciler& reconciler, class Window& window)
            : document(window)
            , onAfterUpdate(reconciler.onAfterUpdate, [this]{ document.update(); }) {
            PROFILE
        }

        ~_State() {
            PROFILE

            if (ref != nullptr && *ref == &document) {
                *ref = nullptr;
            }
        }
    };

    auto& reconciler = UseContext<Reconciler>();
    auto& window = UseContext<class Window>();
    auto& state = UseState<_State>(reconciler, window);

    if (state.ref != props.ref) {
        if (state.ref != nullptr) {
            *state.ref = nullptr;
        }

        state.ref = props.ref;

        if (state.ref != nullptr) {
            *state.ref = &state.document;
        }
    }

    if (state.shouldUpdateScale(props.scale) && props.scale.has_value()) state.document.setScale(props.scale.value());
    if (state.shouldUpdateOverflowX(props.nodeProps.overflowX)) state.document.setOverflowX(props.nodeProps.overflowX);
    if (state.shouldUpdateOverflowY(props.nodeProps.overflowY)) state.document.setOverflowY(props.nodeProps.overflowY);
    if (state.shouldUpdateDirection(props.nodeProps.direction)) state.document.setDirection(props.nodeProps.direction);
    if (state.shouldUpdateAlignment(props.nodeProps.alignment)) state.document.setAlignment(props.nodeProps.alignment);
    if (state.shouldUpdateJustify(props.nodeProps.justify)) state.document.setJustify(props.nodeProps.justify);
    if (state.shouldUpdatePaddingTop(props.nodeProps.paddingTop)) state.document.setPaddingTop(props.nodeProps.paddingTop);
    if (state.shouldUpdatePaddingLeft(props.nodeProps.paddingLeft)) state.document.setPaddingLeft(props.nodeProps.paddingLeft);
    if (state.shouldUpdatePaddingRight(props.nodeProps.paddingRight)) state.document.setPaddingRight(props.nodeProps.paddingRight);
    if (state.shouldUpdatePaddingBottom(props.nodeProps.paddingBottom)) state.document.setPaddingBottom(props.nodeProps.paddingBottom);
    if (state.shouldUpdateGapX(props.nodeProps.gapX)) state.document.setGapX(props.nodeProps.gapX);
    if (state.shouldUpdateGapY(props.nodeProps.gapY)) state.document.setGapY(props.nodeProps.gapY);
    if (state.shouldUpdateOpacity(props.nodeProps.opacity)) state.document.setOpacity(props.nodeProps.opacity);
    if (state.shouldUpdateCursor(props.nodeProps.cursor)) state.document.setCursor(props.nodeProps.cursor);
    if (state.shouldUpdateFontFamily(props.nodeProps.fontFamily)) state.document.setFontFamily(props.nodeProps.fontFamily);
    if (state.shouldUpdateFontWeight(props.nodeProps.fontWeight)) state.document.setFontWeight(props.nodeProps.fontWeight);
    if (state.shouldUpdateFontStyle(props.nodeProps.fontStyle)) state.document.setFontStyle(props.nodeProps.fontStyle);
    if (state.shouldUpdateFontSize(props.nodeProps.fontSize)) state.document.setFontSize(props.nodeProps.fontSize);
    if (state.shouldUpdateLineHeight(props.nodeProps.lineHeight)) state.document.setLineHeight(props.nodeProps.lineHeight);
    if (state.shouldUpdateTextColor(props.nodeProps.textColor)) state.document.setTextColor(props.nodeProps.textColor);
    if (state.shouldUpdateBackground(props.nodeProps.background)) state.document.setBackground(props.nodeProps.background);
    if (state.shouldUpdateForeground(props.nodeProps.foreground)) state.document.setForeground(props.nodeProps.foreground);
    if (state.shouldUpdateBorder(props.nodeProps.border)) state.document.setBorder(props.nodeProps.border);

    state.context.childNextIndex = 0;

    SetContext<class Document>(state.document);
    SetContext<_NodeContext>(state.context);

    if (children) {
        children();
    }
}

} /* namespace Rocket */
