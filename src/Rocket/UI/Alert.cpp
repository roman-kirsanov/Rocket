/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <optional>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Modal.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Alert.hpp>

namespace Rocket {

struct _AlertState {
    AlertTheme::Style style;
    std::optional<Brush> background;
    Effect<
        Theme const*,
        std::optional<AlertIntent>,
        float
    > shouldUpdate;
};

static AlertTheme::Style _ResolveStyle(AlertTheme const& theme, AlertIntent intent) {
    auto const rules = _GetCascade<AlertTheme::Rule>(
        theme.rules,
        [&](AlertTheme::Rule const& rule) {
            return (rule.selector.intent.has_value() == false || rule.selector.intent == intent);
        },
        [](AlertTheme::Rule const& rule) {
            return (rule.selector.intent.has_value() ? 1 : 0);
        }
    );

    auto style = AlertTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.materialRoundness, rule->style.materialRoundness);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.padding, rule->style.padding);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.contentGap, rule->style.contentGap);
        _Cascade(style.textGap, rule->style.textGap);
        _Cascade(style.buttonsGap, rule->style.buttonsGap);
        _Cascade(style.maxWidth, rule->style.maxWidth);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.titleFontSize, rule->style.titleFontSize);
        _Cascade(style.titleFontWeight, rule->style.titleFontWeight);
        _Cascade(style.icon, rule->style.icon);
        _Cascade(style.iconColor, rule->style.iconColor);
        _Cascade(style.iconSize, rule->style.iconSize);
    }

    return style;
}

void Alert(AlertProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_AlertState>();
    auto& document = UseContext<class Document>();
    auto const& theme = UseTheme();

    if (state.shouldUpdate(&theme, props.intent, document.getScale())) {
        auto const style = _ResolveStyle(theme.alert, props.intent.value_or(AlertIntent::Default));
        state.style = style;
        state.background = std::nullopt;

        if (style.materialKind.has_value() && theme.material != nullptr) {
            state.background = theme.material->getVariant({
                .kind = style.materialKind,
                .intent = style.materialIntent,
                .brightness = style.materialBrightness,
                .roundness = style.materialRoundness,
                .scale = document.getScale()
            }).brush;
        }
    }

    auto const& style = state.style;
    auto const padding = style.padding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.gapY = node.gapY.value_or(node.gap.value_or(style.gap.value_or(0.0f)));
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(padding.left));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(padding.top));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(padding.right));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(padding.bottom));

    if (state.background.has_value()) {
        node.background = node.background.value_or(state.background.value());
    }

    if (style.borderColor.has_value()) {
        node.border = node.border.value_or(ColorBrush{ style.borderColor.value() });
    }

    if (style.borderWidth.has_value()) {
        node.borderWidth = node.borderWidth.value_or(style.borderWidth.value());
    }

    if (style.color.has_value()) {
        node.textColor = node.textColor.value_or(style.color.value());
    }

    if (style.fontSize.has_value()) {
        node.fontSize = node.fontSize.value_or(style.fontSize.value());
    }

    Modal({ .backdropProps = props.backdropProps, .nodeProps = node }, [&]{
        Node({
            .direction = NodeDirection::Horizontal,
            .alignment = NodeAlignment::Start,
            .maxWidth = (style.maxWidth.has_value() ? std::optional<NodeValue>(style.maxWidth.value()) : std::nullopt),
            .gapX = style.contentGap.value_or(0.0f)
        }, [&]{
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

                if (icon.intent.has_value() == false && style.iconColor.has_value()) {
                    icon.colorValue = icon.colorValue.value_or(style.iconColor.value());
                }

                Icon(icon);
            }

            Node({
                .direction = NodeDirection::Vertical,
                .gapY = style.textGap.value_or(0.0f),
                .flex = true
            }, [&]{
                if (props.title.has_value()) {
                    auto title = props.titleProps;
                    title.display = NodeDisplay::Text;
                    title.content = title.content.value_or(props.title.value());

                    if (style.titleFontSize.has_value()) {
                        title.fontSize = title.fontSize.value_or(style.titleFontSize.value());
                    }

                    if (style.titleFontWeight.has_value()) {
                        title.fontWeight = title.fontWeight.value_or(style.titleFontWeight.value());
                    }

                    Node(title);
                }

                if (props.body.has_value()) {
                    auto body = props.bodyProps;
                    body.display = NodeDisplay::Text;
                    body.content = body.content.value_or(props.body.value());

                    Node(body);
                }
            });
        });

        if (props.buttons != nullptr) {
            auto buttons = props.buttonsProps;
            buttons.direction = buttons.direction.value_or(NodeDirection::Horizontal);
            buttons.alignment = buttons.alignment.value_or(NodeAlignment::Center);
            buttons.justify = buttons.justify.value_or(NodeJustify::Center);
            buttons.gapX = buttons.gapX.value_or(buttons.gap.value_or(style.buttonsGap.value_or(0.0f)));

            Node(buttons, props.buttons);
        }
    });
}

} /* namespace Rocket */
