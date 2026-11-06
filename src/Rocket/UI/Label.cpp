/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <array>
#include <optional>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Label.hpp>

namespace Rocket {

struct _LabelState {
    std::array<LabelTheme::Style, 2> styles;
    Effect<Theme const*> shouldUpdate;
};

static auto constexpr _states = std::array<LabelTheme::State, 2>{
    LabelTheme::State::Normal,
    LabelTheme::State::Hover
};

static LabelTheme::Style _ResolveStyle(LabelTheme const& theme, LabelTheme::State state) {
    auto const rules = _GetCascade<LabelTheme::Rule>(
        theme.rules,
        [&](LabelTheme::Rule const& rule) {
            return (rule.selector.state.has_value() == false || rule.selector.state == state);
        },
        [](LabelTheme::Rule const& rule) {
            return (rule.selector.state.has_value() ? 1 : 0);
        }
    );

    auto style = LabelTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.fontWeight, rule->style.fontWeight);
        _Cascade(style.minHeight, rule->style.minHeight);
        _Cascade(style.requiredMark, rule->style.requiredMark);
        _Cascade(style.requiredColor, rule->style.requiredColor);
        _Cascade(style.requiredFontSize, rule->style.requiredFontSize);
        _Cascade(style.requiredGap, rule->style.requiredGap);
        _Cascade(style.icon, rule->style.icon);
        _Cascade(style.iconColor, rule->style.iconColor);
        _Cascade(style.iconSize, rule->style.iconSize);
        _Cascade(style.iconOpacity, rule->style.iconOpacity);
        _Cascade(style.iconGap, rule->style.iconGap);
    }

    return style;
}

void Label(LabelProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto& state = UseState<_LabelState>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto& tooltipRef = UseRef<class Node>(props.tooltipProps.nodeProps.ref);
    auto const& theme = UseTheme();

    if (state.shouldUpdate(&theme)) {
        for (auto index = 0llu; index < _states.size(); index++) {
            state.styles[index] = _ResolveStyle(theme.label, _states[index]);
        }
    }

    auto const isHover = (tooltipRef != nullptr && tooltipRef->isHover());
    auto const& style = state.styles[isHover ? 1llu : 0llu];

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);

    if (style.minHeight.has_value()) {
        node.minHeight = node.minHeight.value_or(style.minHeight.value());
    }

    if (style.color.has_value()) {
        node.textColor = node.textColor.value_or(style.color.value());
    }

    if (style.fontSize.has_value()) {
        node.fontSize = node.fontSize.value_or(style.fontSize.value());
    }

    if (style.fontWeight.has_value()) {
        node.fontWeight = node.fontWeight.value_or(style.fontWeight.value());
    }

    Node(node, [&]{
        if (props.text.has_value()) {
            auto text = props.textProps;
            text.display = NodeDisplay::Text;
            text.content = text.content.value_or(props.text.value());

            Node(text);
        }

        children();

        if (props.required.value_or(false) && style.requiredMark.has_value()) {
            auto required = props.requiredProps;
            required.display = NodeDisplay::Text;
            required.content = required.content.value_or(style.requiredMark.value());
            required.marginLeft = required.marginLeft.value_or(required.margin.value_or(style.requiredGap.value_or(0.0f)));

            if (style.requiredColor.has_value()) {
                required.textColor = required.textColor.value_or(style.requiredColor.value());
            }

            if (style.requiredFontSize.has_value()) {
                required.fontSize = required.fontSize.value_or(style.requiredFontSize.value());
            }

            Node(required);
        }

        if (props.tooltip.has_value()) {
            auto tooltip = props.tooltipProps;
            tooltip.text = props.tooltip;
            tooltip.nodeProps.ref = &tooltipRef;
            tooltip.nodeProps.marginLeft = tooltip.nodeProps.marginLeft.value_or(tooltip.nodeProps.margin.value_or(style.iconGap.value_or(0.0f)));

            Tooltip(tooltip, [&]{
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

                if (icon.intent.has_value() == false && style.iconColor.has_value()) {
                    icon.colorValue = icon.colorValue.value_or(style.iconColor.value());
                }

                if (style.iconOpacity.has_value()) {
                    icon.nodeProps.opacity = icon.nodeProps.opacity.value_or(style.iconOpacity.value());
                }

                Icon(icon);
            });
        }
    });
}

} /* namespace Rocket */
