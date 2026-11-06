/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <optional>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Modal.hpp>

namespace Rocket {

struct _ModalState {
    ModalTheme::Style style;
    Brush background;
    std::optional<Brush> border;
    float borderWidth;
    float borderRadius;
    Effect<
        Theme const*,
        float
    > shouldUpdate;
};

static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static ModalTheme::Style _ResolveStyle(ModalTheme const& theme) {
    auto const rules = _GetCascade<ModalTheme::Rule>(
        theme.rules,
        [](ModalTheme::Rule const&) {
            return true;
        },
        [](ModalTheme::Rule const&) {
            return 0;
        }
    );

    auto style = ModalTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.backdrop, rule->style.backdrop);
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.materialRoundness, rule->style.materialRoundness);
        _Cascade(style.background, rule->style.background);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.borderRadius, rule->style.borderRadius);
        _Cascade(style.shadow, rule->style.shadow);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
    }

    return style;
}

void Modal(ModalProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto& state = UseState<_ModalState>();
    auto& document = UseContext<class Document>();
    auto& backdropRef = UseRef<class Node>(props.backdropProps.ref);
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto const& theme = UseTheme();

    if (state.shouldUpdate(&theme, document.getScale())) {
        auto const style = _ResolveStyle(theme.modal);
        state.style = style;

        if (style.materialKind.has_value() && theme.material != nullptr) {
            state.background = theme.material->getVariant({
                .kind = style.materialKind,
                .intent = style.materialIntent,
                .brightness = style.materialBrightness,
                .roundness = style.materialRoundness,
                .scale = document.getScale()
            }).brush;
            state.border = std::nullopt;
            state.borderWidth = 0.0f;
            state.borderRadius = 0.0f;
        } else {
            state.background = ColorBrush{ style.background.value_or(_transparent) };
            state.border = ColorBrush{ style.borderColor.value_or(_transparent) };
            state.borderWidth = style.borderWidth.value_or(0.0f);
            state.borderRadius = style.borderRadius.value_or(0.0f);
        }
    }

    auto const& style = state.style;

    auto backdrop = props.backdropProps;
    backdrop.ref = &backdropRef;
    backdrop.root = backdrop.root.value_or(true);
    backdrop.position = backdrop.position.value_or(NodePosition::Absolute);
    backdrop.left = backdrop.left.value_or(0.0f);
    backdrop.top = backdrop.top.value_or(0.0f);
    backdrop.right = backdrop.right.value_or(0.0f);
    backdrop.bottom = backdrop.bottom.value_or(0.0f);
    backdrop.alignment = backdrop.alignment.value_or(NodeAlignment::Center);
    backdrop.justify = backdrop.justify.value_or(NodeJustify::Center);
    backdrop.zIndex = backdrop.zIndex.value_or(MODAL_ZINDEX);
    backdrop.background = backdrop.background.value_or(ColorBrush{ style.backdrop.value_or(_transparent) });
    backdrop.cursor = backdrop.cursor.value_or(Cursor::Default);

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    node.borderLeftWidth = node.borderLeftWidth.value_or(node.borderWidth.value_or(state.borderWidth));
    node.borderTopWidth = node.borderTopWidth.value_or(node.borderWidth.value_or(state.borderWidth));
    node.borderRightWidth = node.borderRightWidth.value_or(node.borderWidth.value_or(state.borderWidth));
    node.borderBottomWidth = node.borderBottomWidth.value_or(node.borderWidth.value_or(state.borderWidth));
    node.borderTopLeftRadius = node.borderTopLeftRadius.value_or(node.borderRadius.value_or(state.borderRadius));
    node.borderTopRightRadius = node.borderTopRightRadius.value_or(node.borderRadius.value_or(state.borderRadius));
    node.borderBottomLeftRadius = node.borderBottomLeftRadius.value_or(node.borderRadius.value_or(state.borderRadius));
    node.borderBottomRightRadius = node.borderBottomRightRadius.value_or(node.borderRadius.value_or(state.borderRadius));
    node.background = node.background.value_or(state.background);

    if (state.border.has_value()) {
        node.border = node.border.value_or(state.border.value());
    }

    if (style.shadow.has_value()) {
        node.shadow = node.shadow.value_or(style.shadow.value());
    }

    if (style.color.has_value()) {
        node.textColor = node.textColor.value_or(style.color.value());
    }

    if (style.fontSize.has_value()) {
        node.fontSize = node.fontSize.value_or(style.fontSize.value());
    }

    Node(backdrop, [&]{
        Node(node, children);
    });
}

} /* namespace Rocket */
