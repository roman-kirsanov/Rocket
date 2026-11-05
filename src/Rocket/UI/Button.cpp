/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <array>
#include <cstddef>
#include <optional>
#include <tuple>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Button.hpp>

namespace Rocket {

struct _ButtonLook {
    ButtonTheme::Style style;
    Brush background;
    std::optional<Brush> border;
    Vec4 borderWidths;
    std::array<float, 4> borderRadii;
};

struct _ButtonState {
    std::array<_ButtonLook, 4> looks;
    std::function<void()> onClick;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    Effect<
        Theme const*,
        std::optional<ButtonIntent>,
        std::optional<ButtonKind>,
        std::optional<ButtonSize>,
        std::optional<bool>,
        std::optional<bool>,
        std::optional<bool>,
        std::optional<bool>,
        std::optional<bool>,
        float
    > shouldUpdate;
};

static auto constexpr _states = std::array<ButtonTheme::State, 4>{
    ButtonTheme::State::Normal,
    ButtonTheme::State::Hover,
    ButtonTheme::State::Active,
    ButtonTheme::State::Disabled
};
static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static ButtonTheme::Style _ResolveStyle(ButtonTheme const& theme, ButtonKind kind, ButtonIntent intent, ButtonSize size, bool narrow, ButtonTheme::State state) {
    auto const rules = _GetCascade<ButtonTheme::Rule>(
        theme.rules,
        [&](ButtonTheme::Rule const& rule) {
            return (
                (rule.selector.kind.has_value() == false || rule.selector.kind == kind) &&
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.size.has_value() == false || rule.selector.size == size) &&
                (rule.selector.state.has_value() == false || rule.selector.state == state) &&
                (rule.selector.narrow.has_value() == false || rule.selector.narrow == narrow)
            );
        },
        [](ButtonTheme::Rule const& rule) {
            return (
                (rule.selector.kind.has_value() ? 1 : 0) +
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.size.has_value() ? 1 : 0) +
                (rule.selector.state.has_value() ? 1 : 0) +
                (rule.selector.narrow.has_value() ? 1 : 0)
            );
        }
    );

    auto style = ButtonTheme::Style();

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
        _Cascade(style.padding, rule->style.padding);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.minHeight, rule->style.minHeight);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.iconSize, rule->style.iconSize);
        _Cascade(style.opacity, rule->style.opacity);
    }

    return style;
}

void Button(ButtonProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_ButtonState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto const& theme = UseTheme();

    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());
    auto const isActive = (props.active.value_or(false) || (nodeRef != nullptr && nodeRef->isActive()));
    auto const disabled = props.disabled.value_or(false);

    state.onClick = props.onClick;
    state.nodeOnEvent = props.nodeProps.onEvent;

    if (state.shouldUpdate(&theme, props.intent, props.kind, props.size, props.narrow, props.noLeftBorder, props.noTopBorder, props.noRightBorder, props.noBottomBorder, document.getScale())) {
        auto const noLeftBorder = props.noLeftBorder.value_or(false);
        auto const noTopBorder = props.noTopBorder.value_or(false);
        auto const noRightBorder = props.noRightBorder.value_or(false);
        auto const noBottomBorder = props.noBottomBorder.value_or(false);

        for (auto index = 0llu; index < _states.size(); index++) {
            auto const style = _ResolveStyle(
                theme.button,
                props.kind.value_or(ButtonKind::Default),
                props.intent.value_or(ButtonIntent::Default),
                props.size.value_or(ButtonSize::Medium),
                props.narrow.value_or(false),
                _states[index]
            );
            auto& look = state.looks[index];
            look.style = style;

            if (style.materialKind.has_value() && theme.material != nullptr) {
                look.background = theme.material->getVariant({
                    .kind = style.materialKind,
                    .intent = style.materialIntent,
                    .brightness = style.materialBrightness,
                    .roundness = style.materialRoundness,
                    .scale = document.getScale(),
                    .noLeftBorder = noLeftBorder,
                    .noTopBorder = noTopBorder,
                    .noRightBorder = noRightBorder,
                    .noBottomBorder = noBottomBorder
                }).brush;
                look.border = std::nullopt;
                look.borderWidths = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
                look.borderRadii = { 0.0f, 0.0f, 0.0f, 0.0f };
            } else {
                auto const borderWidth = style.borderWidth.value_or(0.0f);
                auto const borderRadius = style.borderRadius.value_or(0.0f);

                look.background = ColorBrush{ style.background.value_or(_transparent) };
                look.border = ColorBrush{ style.borderColor.value_or(_transparent) };
                look.borderWidths = Vec4{
                    (noLeftBorder ? 0.0f : borderWidth),
                    (noTopBorder ? 0.0f : borderWidth),
                    (noRightBorder ? 0.0f : borderWidth),
                    (noBottomBorder ? 0.0f : borderWidth)
                };
                look.borderRadii = {
                    ((noTopBorder || noLeftBorder) ? 0.0f : borderRadius),
                    ((noTopBorder || noRightBorder) ? 0.0f : borderRadius),
                    ((noBottomBorder || noLeftBorder) ? 0.0f : borderRadius),
                    ((noBottomBorder || noRightBorder) ? 0.0f : borderRadius)
                };
            }
        }
    }

    auto const& look = state.looks[
        disabled ? 3llu :
        isActive ? 2llu :
        isHover ? 1llu :
                  0llu
    ];
    auto const& style = look.style;
    auto const padding = style.padding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    auto const gap = style.gap.value_or(Vec2{ 0.0f, 0.0f });

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.gapX = node.gapX.value_or(node.gap.value_or(gap.x));
    node.gapY = node.gapY.value_or(node.gap.value_or(gap.y));
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(padding.left));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(padding.top));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(padding.right));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(padding.bottom));
    node.borderLeftWidth = node.borderLeftWidth.value_or(node.borderWidth.value_or(look.borderWidths.left));
    node.borderTopWidth = node.borderTopWidth.value_or(node.borderWidth.value_or(look.borderWidths.top));
    node.borderRightWidth = node.borderRightWidth.value_or(node.borderWidth.value_or(look.borderWidths.right));
    node.borderBottomWidth = node.borderBottomWidth.value_or(node.borderWidth.value_or(look.borderWidths.bottom));
    node.borderTopLeftRadius = node.borderTopLeftRadius.value_or(node.borderRadius.value_or(look.borderRadii[0]));
    node.borderTopRightRadius = node.borderTopRightRadius.value_or(node.borderRadius.value_or(look.borderRadii[1]));
    node.borderBottomLeftRadius = node.borderBottomLeftRadius.value_or(node.borderRadius.value_or(look.borderRadii[2]));
    node.borderBottomRightRadius = node.borderBottomRightRadius.value_or(node.borderRadius.value_or(look.borderRadii[3]));
    node.background = node.background.value_or(look.background);
    node.onEvent = [&, disabled](NodeEvent const& event) {
        if (state.nodeOnEvent != nullptr) {
            state.nodeOnEvent(event);
        }

        if (event.is<MouseClickNodeEvent>()) {
            if (disabled == false) {
                if (state.onClick != nullptr) {
                    state.onClick();
                }

                event.stopPropagation();
            }
        }
    };

    if (style.minHeight.has_value()) {
        node.minHeight = node.minHeight.value_or(style.minHeight.value());
    }

    if (style.opacity.has_value()) {
        node.opacity = node.opacity.value_or(style.opacity.value());
    }

    if (look.border.has_value()) {
        node.border = node.border.value_or(look.border.value());
    }

    if (disabled == false) {
        node.cursor = node.cursor.value_or(Cursor::Pointer);
    }

    Node(node, [&]{
        if (props.icon.has_value()) {
            auto icon = props.iconProps;

            if (icon.data.has_value() == false) {
                icon.data.emplace(props.icon.value());
            }

            if (style.iconSize.has_value()) {
                icon.sizeValue = icon.sizeValue.value_or(style.iconSize.value());
            }

            if (icon.intent.has_value() == false && style.color.has_value()) {
                icon.colorValue = icon.colorValue.value_or(style.color.value());
            }

            Icon(icon);
        }

        if (props.label.has_value()) {
            auto text = props.textProps;
            text.display = NodeDisplay::Text;
            text.content = text.content.value_or(props.label.value());

            if (style.fontSize.has_value()) {
                text.fontSize = text.fontSize.value_or(style.fontSize.value());
            }

            if (style.color.has_value()) {
                text.textColor = text.textColor.value_or(style.color.value());
            }

            Node(text);
        }
    });
}

} /* namespace Rocket */
