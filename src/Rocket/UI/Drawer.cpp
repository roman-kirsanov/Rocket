/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <optional>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Drawer.hpp>

namespace Rocket {

struct _DrawerState {
    DrawerTheme::Style style;
    Brush background;
    std::optional<Brush> border;
    float borderWidth;
    std::function<void()> onClose;
    std::function<void(NodeEvent const&)> backdropOnEvent;
    Effect<
        Theme const*,
        std::optional<DrawerOrigin>,
        float
    > shouldUpdate;
};

static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static DrawerTheme::Style _ResolveStyle(DrawerTheme const& theme, DrawerOrigin origin) {
    auto const rules = _GetCascade<DrawerTheme::Rule>(
        theme.rules,
        [&](DrawerTheme::Rule const& rule) {
            return (rule.selector.origin.has_value() == false || rule.selector.origin == origin);
        },
        [](DrawerTheme::Rule const& rule) {
            return (rule.selector.origin.has_value() ? 1 : 0);
        }
    );

    auto style = DrawerTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.backdrop, rule->style.backdrop);
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.background, rule->style.background);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.shadow, rule->style.shadow);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
    }

    return style;
}

void Drawer(DrawerProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto& state = UseState<_DrawerState>();
    auto& document = UseContext<class Document>();
    auto& backdropRef = UseRef<class Node>(props.backdropProps.ref);
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto const& theme = UseTheme();

    auto const origin = props.origin.value_or(DrawerOrigin::Left);

    state.onClose = props.onClose;
    state.backdropOnEvent = props.backdropProps.onEvent;

    if (state.shouldUpdate(&theme, props.origin, document.getScale())) {
        auto const style = _ResolveStyle(theme.drawer, origin);
        state.style = style;

        if (style.materialKind.has_value() && theme.material != nullptr) {
            state.background = theme.material->getVariant({
                .kind = style.materialKind,
                .intent = style.materialIntent,
                .brightness = style.materialBrightness,
                .scale = document.getScale(),
                .noLeftBorder = (origin != DrawerOrigin::Right),
                .noTopBorder = (origin != DrawerOrigin::Bottom),
                .noRightBorder = (origin != DrawerOrigin::Left),
                .noBottomBorder = (origin != DrawerOrigin::Top)
            }).brush;
            state.border = std::nullopt;
            state.borderWidth = 0.0f;
        } else {
            state.background = ColorBrush{ style.background.value_or(_transparent) };
            state.border = ColorBrush{ style.borderColor.value_or(_transparent) };
            state.borderWidth = style.borderWidth.value_or(0.0f);
        }
    }

    if (props.visible.value_or(false) == false) {
        return;
    }

    auto const& style = state.style;
    auto const horizontal = (origin == DrawerOrigin::Left || origin == DrawerOrigin::Right);

    auto backdrop = props.backdropProps;
    backdrop.ref = &backdropRef;
    backdrop.root = backdrop.root.value_or(true);
    backdrop.position = backdrop.position.value_or(NodePosition::Absolute);
    backdrop.left = backdrop.left.value_or(0.0f);
    backdrop.top = backdrop.top.value_or(0.0f);
    backdrop.right = backdrop.right.value_or(0.0f);
    backdrop.bottom = backdrop.bottom.value_or(0.0f);
    backdrop.direction = backdrop.direction.value_or(horizontal ? NodeDirection::Horizontal : NodeDirection::Vertical);
    backdrop.alignment = backdrop.alignment.value_or(NodeAlignment::Stretch);
    backdrop.justify = backdrop.justify.value_or((origin == DrawerOrigin::Left || origin == DrawerOrigin::Top) ? NodeJustify::Start : NodeJustify::End);
    backdrop.zIndex = backdrop.zIndex.value_or(MODAL_ZINDEX);
    backdrop.background = backdrop.background.value_or(ColorBrush{ style.backdrop.value_or(_transparent) });
    backdrop.cursor = backdrop.cursor.value_or(Cursor::Default);
    backdrop.onEvent = [&](NodeEvent const& event) {
        if (state.backdropOnEvent != nullptr) {
            state.backdropOnEvent(event);
        }

        if (event.is<MouseDownNodeEvent>() && &event.getNode() == backdropRef) {
            if (state.onClose != nullptr) {
                state.onClose();
            }

            event.stopPropagation();
        }
    };

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    node.background = node.background.value_or(state.background);

    if (state.border.has_value()) {
        node.borderLeftWidth = node.borderLeftWidth.value_or(node.borderWidth.value_or(origin == DrawerOrigin::Right ? state.borderWidth : 0.0f));
        node.borderTopWidth = node.borderTopWidth.value_or(node.borderWidth.value_or(origin == DrawerOrigin::Bottom ? state.borderWidth : 0.0f));
        node.borderRightWidth = node.borderRightWidth.value_or(node.borderWidth.value_or(origin == DrawerOrigin::Left ? state.borderWidth : 0.0f));
        node.borderBottomWidth = node.borderBottomWidth.value_or(node.borderWidth.value_or(origin == DrawerOrigin::Top ? state.borderWidth : 0.0f));
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
