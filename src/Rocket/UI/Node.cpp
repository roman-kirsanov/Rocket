/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstdint>
#include <Rocket/Node/Document.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Node_Private.hpp>

namespace Rocket {

void Node(NodeProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    struct _State {
        class Node** ref = nullptr;
        class Node node;
        _NodeContext context;
        Sub<NodeEvent const&> eventSub;
        Sub<NodeEvent const&> captureEventSub;
        std::function<void(NodeEvent const&)> onEvent;
        std::function<void(NodeEvent const&)> onCaptureEvent;
        Effect<std::optional<NodeDisplay>> shouldUpdateDisplay;
        Effect<std::optional<NodeOverflow>> shouldUpdateOverflowX;
        Effect<std::optional<NodeOverflow>> shouldUpdateOverflowY;
        Effect<std::optional<NodePosition>> shouldUpdatePosition;
        Effect<std::optional<NodeDirection>> shouldUpdateDirection;
        Effect<std::optional<NodeAlignment>> shouldUpdateAlignment;
        Effect<std::optional<NodeAlignment>> shouldUpdateSelfAlignment;
        Effect<std::optional<NodeJustify>> shouldUpdateJustify;
        Effect<std::optional<NodeValue>> shouldUpdateWidth;
        Effect<std::optional<NodeValue>> shouldUpdateHeight;
        Effect<std::optional<NodeValue>> shouldUpdateMinWidth;
        Effect<std::optional<NodeValue>> shouldUpdateMaxWidth;
        Effect<std::optional<NodeValue>> shouldUpdateMinHeight;
        Effect<std::optional<NodeValue>> shouldUpdateMaxHeight;
        Effect<std::optional<NodeValue>> shouldUpdateTop;
        Effect<std::optional<NodeValue>> shouldUpdateLeft;
        Effect<std::optional<NodeValue>> shouldUpdateRight;
        Effect<std::optional<NodeValue>> shouldUpdateBottom;
        Effect<std::optional<NodeValue>> shouldUpdatePadding;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingTop;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingLeft;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingRight;
        Effect<std::optional<NodeValue>> shouldUpdatePaddingBottom;
        Effect<std::optional<NodeValue>> shouldUpdateMargin;
        Effect<std::optional<NodeValue>> shouldUpdateMarginTop;
        Effect<std::optional<NodeValue>> shouldUpdateMarginLeft;
        Effect<std::optional<NodeValue>> shouldUpdateMarginRight;
        Effect<std::optional<NodeValue>> shouldUpdateMarginBottom;
        Effect<std::optional<NodeValue>> shouldUpdateGap;
        Effect<std::optional<NodeValue>> shouldUpdateGapX;
        Effect<std::optional<NodeValue>> shouldUpdateGapY;
        Effect<std::optional<float>> shouldUpdateBorderWidth;
        Effect<std::optional<float>> shouldUpdateBorderTopWidth;
        Effect<std::optional<float>> shouldUpdateBorderLeftWidth;
        Effect<std::optional<float>> shouldUpdateBorderRightWidth;
        Effect<std::optional<float>> shouldUpdateBorderBottomWidth;
        Effect<std::optional<float>> shouldUpdateBorderRadius;
        Effect<std::optional<float>> shouldUpdateBorderTopLeftRadius;
        Effect<std::optional<float>> shouldUpdateBorderTopRightRadius;
        Effect<std::optional<float>> shouldUpdateBorderBottomLeftRadius;
        Effect<std::optional<float>> shouldUpdateBorderBottomRightRadius;
        Effect<std::optional<bool>> shouldUpdateVisible;
        Effect<std::optional<int>> shouldUpdateZIndex;
        Effect<std::optional<Vec2>> shouldUpdateOffset;
        Effect<std::optional<NodeTransform>> shouldUpdateTransform;
        Effect<std::optional<float>> shouldUpdateOpacity;
        Effect<std::optional<std::string>> shouldUpdateFontFamily;
        Effect<std::optional<FontWeight>> shouldUpdateFontWeight;
        Effect<std::optional<FontStyle>> shouldUpdateFontStyle;
        Effect<std::optional<float>> shouldUpdateFontSize;
        Effect<std::optional<float>> shouldUpdateLineHeight;
        Effect<std::optional<Vec4>> shouldUpdateTextColor;
        Effect<std::optional<Vec4>> shouldUpdateTextMarker;
        Effect<std::optional<std::string>> shouldUpdateContent;
        Effect<std::optional<Brush>> shouldUpdateBackground;
        Effect<std::optional<Brush>> shouldUpdateForeground;
        Effect<std::optional<Brush>> shouldUpdateBorder;
        Effect<std::optional<Shadow>> shouldUpdateShadow;
        Effect<std::optional<Cursor>> shouldUpdateCursor;
        Effect<std::optional<int>> shouldUpdateTabIndex;
        Effect<std::optional<bool>> shouldUpdateSkip;
        Effect<std::optional<bool>> shouldUpdateFlex;
        Effect<std::optional<bool>> shouldUpdateKeyEvents;
        Effect<std::optional<bool>> shouldUpdateMouseEvents;
        Effect<std::optional<bool>> shouldUpdateClipped;
        Effect<std::optional<bool>> shouldUpdateEditable;
        Effect<std::optional<bool>> shouldUpdateSecure;
        Effect<std::optional<bool>> shouldUpdateMultiLine;
        Effect<std::optional<bool>, Document*, class Node*, std::int64_t> shouldUpdateParent;

        _State() {
            PROFILE

            context.node = &node;

            eventSub.on(node.onEvent, [this](NodeEvent const& event) {
                if (onEvent) {
                    onEvent(event);
                }
            });

            captureEventSub.on(node.onCaptureEvent, [this](NodeEvent const& event) {
                if (onCaptureEvent) {
                    onCaptureEvent(event);
                }
            });
        }

        ~_State() {
            PROFILE

            /* Only clear a ref that still points at this node: a sibling
               that got re-matched to the same forwarded slot this pass has
               already written its own node there. */
            if (ref != nullptr && *ref == &node) {
                *ref = nullptr;
            }
        }
    };

    auto& document = UseContext<class Document>();
    auto* context = UseContextIf<_NodeContext>();
    auto& state = UseState<_State>();

    auto* parent = (context != nullptr) ? context->node : nullptr;
    auto index = (context != nullptr) ? context->childNextIndex++ : 0;

    state.context.childNextIndex = 0;
    state.onEvent = props.onEvent;
    state.onCaptureEvent = props.onCaptureEvent;

    if (state.ref != props.ref) {
        if (state.ref != nullptr) {
            *state.ref = nullptr;
        }

        state.ref = props.ref;

        if (state.ref != nullptr) {
            *state.ref = &state.node;
        }
    }

    if (state.shouldUpdateDisplay(props.display)) state.node.setDisplay(props.display);
    if (state.shouldUpdateOverflowX(props.overflowX)) state.node.setOverflowX(props.overflowX);
    if (state.shouldUpdateOverflowY(props.overflowY)) state.node.setOverflowY(props.overflowY);
    if (state.shouldUpdatePosition(props.position)) state.node.setPosition(props.position);
    if (state.shouldUpdateDirection(props.direction)) state.node.setDirection(props.direction);
    if (state.shouldUpdateAlignment(props.alignment)) state.node.setAlignment(props.alignment);
    if (state.shouldUpdateSelfAlignment(props.selfAlignment)) state.node.setSelfAlignment(props.selfAlignment);
    if (state.shouldUpdateJustify(props.justify)) state.node.setJustify(props.justify);
    if (state.shouldUpdateWidth(props.width)) state.node.setWidth(props.width);
    if (state.shouldUpdateHeight(props.height)) state.node.setHeight(props.height);
    if (state.shouldUpdateMinWidth(props.minWidth)) state.node.setMinWidth(props.minWidth);
    if (state.shouldUpdateMaxWidth(props.maxWidth)) state.node.setMaxWidth(props.maxWidth);
    if (state.shouldUpdateMinHeight(props.minHeight)) state.node.setMinHeight(props.minHeight);
    if (state.shouldUpdateMaxHeight(props.maxHeight)) state.node.setMaxHeight(props.maxHeight);
    if (state.shouldUpdateTop(props.top)) state.node.setTop(props.top);
    if (state.shouldUpdateLeft(props.left)) state.node.setLeft(props.left);
    if (state.shouldUpdateRight(props.right)) state.node.setRight(props.right);
    if (state.shouldUpdateBottom(props.bottom)) state.node.setBottom(props.bottom);
    if (state.shouldUpdatePadding(props.padding)) state.node.setPadding(props.padding);
    if (state.shouldUpdatePaddingTop(props.paddingTop)) state.node.setPaddingTop(props.paddingTop);
    if (state.shouldUpdatePaddingLeft(props.paddingLeft)) state.node.setPaddingLeft(props.paddingLeft);
    if (state.shouldUpdatePaddingRight(props.paddingRight)) state.node.setPaddingRight(props.paddingRight);
    if (state.shouldUpdatePaddingBottom(props.paddingBottom)) state.node.setPaddingBottom(props.paddingBottom);
    if (state.shouldUpdateMargin(props.margin)) state.node.setMargin(props.margin);
    if (state.shouldUpdateMarginTop(props.marginTop)) state.node.setMarginTop(props.marginTop);
    if (state.shouldUpdateMarginLeft(props.marginLeft)) state.node.setMarginLeft(props.marginLeft);
    if (state.shouldUpdateMarginRight(props.marginRight)) state.node.setMarginRight(props.marginRight);
    if (state.shouldUpdateMarginBottom(props.marginBottom)) state.node.setMarginBottom(props.marginBottom);
    if (state.shouldUpdateGap(props.gap)) state.node.setGap(props.gap);
    if (state.shouldUpdateGapX(props.gapX)) state.node.setGapX(props.gapX);
    if (state.shouldUpdateGapY(props.gapY)) state.node.setGapY(props.gapY);
    if (state.shouldUpdateBorderWidth(props.borderWidth)) state.node.setBorderWidth(props.borderWidth);
    if (state.shouldUpdateBorderTopWidth(props.borderTopWidth)) state.node.setBorderTopWidth(props.borderTopWidth);
    if (state.shouldUpdateBorderLeftWidth(props.borderLeftWidth)) state.node.setBorderLeftWidth(props.borderLeftWidth);
    if (state.shouldUpdateBorderRightWidth(props.borderRightWidth)) state.node.setBorderRightWidth(props.borderRightWidth);
    if (state.shouldUpdateBorderBottomWidth(props.borderBottomWidth)) state.node.setBorderBottomWidth(props.borderBottomWidth);
    if (state.shouldUpdateBorderRadius(props.borderRadius)) state.node.setBorderRadius(props.borderRadius);
    if (state.shouldUpdateBorderTopLeftRadius(props.borderTopLeftRadius)) state.node.setBorderTopLeftRadius(props.borderTopLeftRadius);
    if (state.shouldUpdateBorderTopRightRadius(props.borderTopRightRadius)) state.node.setBorderTopRightRadius(props.borderTopRightRadius);
    if (state.shouldUpdateBorderBottomLeftRadius(props.borderBottomLeftRadius)) state.node.setBorderBottomLeftRadius(props.borderBottomLeftRadius);
    if (state.shouldUpdateBorderBottomRightRadius(props.borderBottomRightRadius)) state.node.setBorderBottomRightRadius(props.borderBottomRightRadius);
    if (state.shouldUpdateVisible(props.visible)) state.node.setVisible(props.visible);
    if (state.shouldUpdateZIndex(props.zIndex)) state.node.setZIndex(props.zIndex);
    if (state.shouldUpdateOffset(props.offset)) state.node.setOffset(props.offset);
    if (state.shouldUpdateTransform(props.transform)) state.node.setTransform(props.transform);
    if (state.shouldUpdateOpacity(props.opacity)) state.node.setOpacity(props.opacity);
    if (state.shouldUpdateFontFamily(props.fontFamily)) state.node.setFontFamily(props.fontFamily);
    if (state.shouldUpdateFontWeight(props.fontWeight)) state.node.setFontWeight(props.fontWeight);
    if (state.shouldUpdateFontStyle(props.fontStyle)) state.node.setFontStyle(props.fontStyle);
    if (state.shouldUpdateFontSize(props.fontSize)) state.node.setFontSize(props.fontSize);
    if (state.shouldUpdateLineHeight(props.lineHeight)) state.node.setLineHeight(props.lineHeight);
    if (state.shouldUpdateTextColor(props.textColor)) state.node.setTextColor(props.textColor);
    if (state.shouldUpdateTextMarker(props.textMarker)) state.node.setTextMarker(props.textMarker);
    if (state.shouldUpdateContent(props.content)) state.node.setContent(props.content);
    if (state.shouldUpdateBackground(props.background)) state.node.setBackground(props.background);
    if (state.shouldUpdateForeground(props.foreground)) state.node.setForeground(props.foreground);
    if (state.shouldUpdateBorder(props.border)) state.node.setBorder(props.border);
    if (state.shouldUpdateShadow(props.shadow)) state.node.setShadow(props.shadow);
    if (state.shouldUpdateCursor(props.cursor)) state.node.setCursor(props.cursor);
    if (state.shouldUpdateTabIndex(props.tabIndex)) state.node.setTabIndex(props.tabIndex.value_or(0));
    if (state.shouldUpdateSkip(props.skip)) state.node.setSkip(props.skip.value_or(false));
    if (state.shouldUpdateFlex(props.flex)) state.node.setFlex(props.flex.value_or(false));
    if (state.shouldUpdateKeyEvents(props.keyEvents)) state.node.setKeyEvents(props.keyEvents.value_or(true));
    if (state.shouldUpdateMouseEvents(props.mouseEvents)) state.node.setMouseEvents(props.mouseEvents.value_or(true));
    if (state.shouldUpdateClipped(props.clipped)) state.node.setClipped(props.clipped.value_or(true));
    if (state.shouldUpdateEditable(props.editable)) state.node.setContentEditable(props.editable.value_or(false));
    if (state.shouldUpdateSecure(props.secure)) state.node.setContentSecure(props.secure.value_or(false));
    if (state.shouldUpdateMultiLine(props.multiLine)) state.node.setContentMultiLine(props.multiLine.value_or(false));
    if (state.shouldUpdateParent(props.root, &document, parent, index)) {
        if (props.root == true || context == nullptr) {
            document.appendChild(state.node);
        } else if (parent != nullptr) {
            parent->insertChild(state.node, index);
        } else {
            document.insertChild(state.node, index);
        }
    }

    SetContext<_NodeContext>(state.context);

    if (props.prefix) {
        props.prefix();
    }
    if (children) {
        children();
    }
    if (props.suffix) {
        props.suffix();
    }
}

} /* namespace Rocket */
