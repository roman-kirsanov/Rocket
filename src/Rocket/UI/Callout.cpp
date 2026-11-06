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
#include <Rocket/UI/Callout.hpp>

namespace Rocket {

struct _CalloutState {
    CalloutTheme::Style style;
    Brush background;
    std::optional<Brush> border;
    float borderWidth;
    float borderRadius;
    Effect<
        Theme const*,
        std::optional<CalloutIntent>,
        float
    > shouldUpdate;
};

static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static CalloutTheme::Style _ResolveStyle(CalloutTheme const& theme, CalloutIntent intent) {
    auto const rules = _GetCascade<CalloutTheme::Rule>(
        theme.rules,
        [&](CalloutTheme::Rule const& rule) {
            return (rule.selector.intent.has_value() == false || rule.selector.intent == intent);
        },
        [](CalloutTheme::Rule const& rule) {
            return (rule.selector.intent.has_value() ? 1 : 0);
        }
    );

    auto style = CalloutTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.materialRoundness, rule->style.materialRoundness);
        _Cascade(style.background, rule->style.background);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.borderRadius, rule->style.borderRadius);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.padding, rule->style.padding);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.icon, rule->style.icon);
        _Cascade(style.iconSize, rule->style.iconSize);
    }

    return style;
}

void Callout(CalloutProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto& state = UseState<_CalloutState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto const& theme = UseTheme();

    if (state.shouldUpdate(&theme, props.intent, document.getScale())) {
        auto const style = _ResolveStyle(theme.callout, props.intent.value_or(CalloutIntent::Default));
        state.style = style;

        if (style.materialKind.has_value() && theme.material != nullptr) {
            auto const material = theme.material->getVariant({
                .kind = style.materialKind,
                .intent = style.materialIntent,
                .brightness = style.materialBrightness,
                .roundness = style.materialRoundness,
                .scale = document.getScale()
            });
            state.background = material.brush;
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
    auto const padding = style.padding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Start);
    node.gapX = node.gapX.value_or(node.gap.value_or(style.gap.value_or(0.0f)));
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(padding.left));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(padding.top));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(padding.right));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(padding.bottom));
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

    if (style.color.has_value()) {
        node.textColor = node.textColor.value_or(style.color.value());
    }

    if (style.fontSize.has_value()) {
        node.fontSize = node.fontSize.value_or(style.fontSize.value());
    }

    Node(node, [&]{
        if (props.showIcon.value_or(false)) {
            auto icon = props.iconProps;

            if (icon.data.has_value() == false) {
                if (props.icon.has_value()) {
                    icon.data.emplace(props.icon.value());
                } else if (style.icon.has_value()) {
                    icon.data.emplace(style.icon.value());
                }
            }

            if (style.iconSize.has_value()) {
                icon.sizeValue = icon.sizeValue.value_or(style.iconSize.value());
            }

            if (icon.intent.has_value() == false && style.color.has_value()) {
                icon.colorValue = icon.colorValue.value_or(style.color.value());
            }

            Icon(icon);
        }

        auto content = props.contentProps;
        content.direction = content.direction.value_or(NodeDirection::Vertical);
        content.flex = content.flex.value_or(true);

        Node(content, children);
    });
}

} /* namespace Rocket */
