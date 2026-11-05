/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <array>
#include <optional>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/Window/Input.hpp>
#include <Rocket/UI/Radiobox.hpp>

namespace Rocket {

struct _RadioboxLook {
    RadioboxTheme::Style style;
    Brush box;
    std::optional<Brush> border;
    float borderWidth;
    float boxSize;
    std::optional<Brush> dot;
    float dotSize;
};

struct _RadioboxState {
    std::array<_RadioboxLook, 8> looks;
    std::function<void(bool)> onChange;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    Effect<
        Theme const*,
        std::optional<RadioboxIntent>,
        std::optional<RadioboxSize>,
        float
    > shouldUpdate;
};

static auto constexpr _states = std::array<RadioboxTheme::State, 4>{
    RadioboxTheme::State::Normal,
    RadioboxTheme::State::Hover,
    RadioboxTheme::State::Active,
    RadioboxTheme::State::Disabled
};
static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static RadioboxTheme::Style _ResolveStyle(RadioboxTheme const& theme, RadioboxIntent intent, RadioboxSize size, RadioboxTheme::State state, bool checked) {
    auto const rules = _GetCascade<RadioboxTheme::Rule>(
        theme.rules,
        [&](RadioboxTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.size.has_value() == false || rule.selector.size == size) &&
                (rule.selector.state.has_value() == false || rule.selector.state == state) &&
                (rule.selector.checked.has_value() == false || rule.selector.checked == checked)
            );
        },
        [](RadioboxTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.size.has_value() ? 1 : 0) +
                (rule.selector.state.has_value() ? 1 : 0) +
                (rule.selector.checked.has_value() ? 1 : 0)
            );
        }
    );

    auto style = RadioboxTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.background, rule->style.background);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.boxSize, rule->style.boxSize);
        _Cascade(style.dotMaterialKind, rule->style.dotMaterialKind);
        _Cascade(style.dotMaterialIntent, rule->style.dotMaterialIntent);
        _Cascade(style.dotMaterialBrightness, rule->style.dotMaterialBrightness);
        _Cascade(style.dotBackground, rule->style.dotBackground);
        _Cascade(style.dotSize, rule->style.dotSize);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.opacity, rule->style.opacity);
    }

    return style;
}

void Radiobox(RadioboxProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_RadioboxState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto const& theme = UseTheme();

    auto const checked = props.checked.value_or(false);
    auto const disabled = props.disabled.value_or(false);
    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());
    auto const isActive = (nodeRef != nullptr && nodeRef->isActive());

    state.onChange = props.onChange;
    state.nodeOnEvent = props.nodeProps.onEvent;

    if (state.shouldUpdate(&theme, props.intent, props.size, document.getScale())) {
        for (auto index = 0llu; index < state.looks.size(); index++) {
            auto const style = _ResolveStyle(
                theme.radiobox,
                props.intent.value_or(RadioboxIntent::Default),
                props.size.value_or(RadioboxSize::Medium),
                _states[index % _states.size()],
                (index >= _states.size())
            );
            auto& look = state.looks[index];
            look.style = style;

            if (style.materialKind.has_value() && theme.material != nullptr) {
                auto const circle = theme.material->getCircleVariant({
                    .kind = style.materialKind,
                    .intent = style.materialIntent,
                    .brightness = style.materialBrightness,
                    .width = style.boxSize.value_or(0.0f),
                    .scale = document.getScale()
                });

                look.box = circle.brush;
                look.border = std::nullopt;
                look.borderWidth = 0.0f;
                look.boxSize = circle.size.width;
            } else {
                look.box = ColorBrush{ style.background.value_or(_transparent) };
                look.border = ColorBrush{ style.borderColor.value_or(_transparent) };
                look.borderWidth = style.borderWidth.value_or(0.0f);
                look.boxSize = style.boxSize.value_or(0.0f);
            }

            if (style.dotSize.has_value() == false) {
                look.dot = std::nullopt;
                look.dotSize = 0.0f;
            } else if (style.dotMaterialKind.has_value() && theme.material != nullptr) {
                auto const circle = theme.material->getCircleVariant({
                    .kind = style.dotMaterialKind,
                    .intent = style.dotMaterialIntent,
                    .brightness = style.dotMaterialBrightness,
                    .width = style.dotSize.value(),
                    .scale = document.getScale()
                });

                look.dot = circle.brush;
                look.dotSize = circle.size.width;
            } else {
                look.dot = ColorBrush{ style.dotBackground.value_or(_transparent) };
                look.dotSize = style.dotSize.value();
            }
        }
    }

    auto const& look = state.looks[
        (checked ? _states.size() : 0llu) + (
            disabled ? 3llu :
            isActive ? 2llu :
            isHover ? 1llu :
                      0llu
        )
    ];
    auto const& style = look.style;

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.gapX = node.gapX.value_or(node.gap.value_or(style.gap.value_or(0.0f)));
    node.onEvent = [&, checked, disabled](NodeEvent const& event) {
        if (state.nodeOnEvent != nullptr) {
            state.nodeOnEvent(event);
        }

        auto const keyDownEvent = event.as<KeyDownNodeEvent>();

        if (event.is<MouseClickNodeEvent>() || (keyDownEvent != nullptr && keyDownEvent->getKeycode() == Keycode::Space)) {
            if (disabled == false) {
                if (checked == false && state.onChange != nullptr) {
                    state.onChange(true);
                }

                event.stopPropagation();
            }
        }
    };

    if (style.opacity.has_value()) {
        node.opacity = node.opacity.value_or(style.opacity.value());
    }

    if (disabled == false) {
        node.cursor = node.cursor.value_or(Cursor::Pointer);
        node.tabIndex = node.tabIndex.value_or(1);
    }

    Node(node, [&]{
        auto box = props.boxProps;
        box.alignment = box.alignment.value_or(NodeAlignment::Center);
        box.justify = box.justify.value_or(NodeJustify::Center);
        box.width = box.width.value_or(look.boxSize);
        box.height = box.height.value_or(look.boxSize);
        box.borderLeftWidth = box.borderLeftWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderTopWidth = box.borderTopWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderRightWidth = box.borderRightWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderBottomWidth = box.borderBottomWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderTopLeftRadius = box.borderTopLeftRadius.value_or(box.borderRadius.value_or(look.boxSize / 2.0f));
        box.borderTopRightRadius = box.borderTopRightRadius.value_or(box.borderRadius.value_or(look.boxSize / 2.0f));
        box.borderBottomLeftRadius = box.borderBottomLeftRadius.value_or(box.borderRadius.value_or(look.boxSize / 2.0f));
        box.borderBottomRightRadius = box.borderBottomRightRadius.value_or(box.borderRadius.value_or(look.boxSize / 2.0f));
        box.background = box.background.value_or(look.box);

        if (look.border.has_value()) {
            box.border = box.border.value_or(look.border.value());
        }

        Node(box, [&]{
            if (look.dot.has_value()) {
                Node({
                    .width = look.dotSize,
                    .height = look.dotSize,
                    .borderRadius = (look.dotSize / 2.0f),
                    .background = look.dot.value()
                });
            }
        });

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
