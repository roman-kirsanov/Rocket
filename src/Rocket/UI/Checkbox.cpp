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
#include <Rocket/UI/Checkbox.hpp>

namespace Rocket {

struct _CheckboxLook {
    CheckboxTheme::Style style;
    Brush background;
    std::optional<Brush> border;
    float borderWidth;
    float borderRadius;
};

struct _CheckboxState {
    std::array<_CheckboxLook, 8> looks;
    std::function<void(bool)> onChange;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    Effect<
        Theme const*,
        std::optional<CheckboxIntent>,
        std::optional<CheckboxSize>,
        float
    > shouldUpdate;
};

static auto constexpr _states = std::array<CheckboxTheme::State, 4>{
    CheckboxTheme::State::Normal,
    CheckboxTheme::State::Hover,
    CheckboxTheme::State::Active,
    CheckboxTheme::State::Disabled
};
static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static CheckboxTheme::Style _ResolveStyle(CheckboxTheme const& theme, CheckboxIntent intent, CheckboxSize size, CheckboxTheme::State state, bool checked) {
    auto const rules = _GetCascade<CheckboxTheme::Rule>(
        theme.rules,
        [&](CheckboxTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.size.has_value() == false || rule.selector.size == size) &&
                (rule.selector.state.has_value() == false || rule.selector.state == state) &&
                (rule.selector.checked.has_value() == false || rule.selector.checked == checked)
            );
        },
        [](CheckboxTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.size.has_value() ? 1 : 0) +
                (rule.selector.state.has_value() ? 1 : 0) +
                (rule.selector.checked.has_value() ? 1 : 0)
            );
        }
    );

    auto style = CheckboxTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.materialRoundness, rule->style.materialRoundness);
        _Cascade(style.background, rule->style.background);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.borderRadius, rule->style.borderRadius);
        _Cascade(style.boxSize, rule->style.boxSize);
        _Cascade(style.mark, rule->style.mark);
        _Cascade(style.markColor, rule->style.markColor);
        _Cascade(style.markSize, rule->style.markSize);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.opacity, rule->style.opacity);
    }

    return style;
}

void Checkbox(CheckboxProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_CheckboxState>();
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
                theme.checkbox,
                props.intent.value_or(CheckboxIntent::Default),
                props.size.value_or(CheckboxSize::Medium),
                _states[index % _states.size()],
                (index >= _states.size())
            );
            auto& look = state.looks[index];
            look.style = style;

            if (style.materialKind.has_value() && theme.material != nullptr) {
                look.background = theme.material->getVariant({
                    .kind = style.materialKind,
                    .intent = style.materialIntent,
                    .brightness = style.materialBrightness,
                    .roundness = style.materialRoundness,
                    .scale = document.getScale()
                }).brush;
                look.border = std::nullopt;
                look.borderWidth = 0.0f;
                look.borderRadius = 0.0f;
            } else {
                look.background = ColorBrush{ style.background.value_or(_transparent) };
                look.border = ColorBrush{ style.borderColor.value_or(_transparent) };
                look.borderWidth = style.borderWidth.value_or(0.0f);
                look.borderRadius = style.borderRadius.value_or(0.0f);
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
                if (state.onChange != nullptr) {
                    state.onChange(checked == false);
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
        auto const boxSize = style.boxSize.value_or(0.0f);
        auto box = props.boxProps;
        box.alignment = box.alignment.value_or(NodeAlignment::Center);
        box.justify = box.justify.value_or(NodeJustify::Center);
        box.width = box.width.value_or(boxSize);
        box.height = box.height.value_or(boxSize);
        box.borderLeftWidth = box.borderLeftWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderTopWidth = box.borderTopWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderRightWidth = box.borderRightWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderBottomWidth = box.borderBottomWidth.value_or(box.borderWidth.value_or(look.borderWidth));
        box.borderTopLeftRadius = box.borderTopLeftRadius.value_or(box.borderRadius.value_or(look.borderRadius));
        box.borderTopRightRadius = box.borderTopRightRadius.value_or(box.borderRadius.value_or(look.borderRadius));
        box.borderBottomLeftRadius = box.borderBottomLeftRadius.value_or(box.borderRadius.value_or(look.borderRadius));
        box.borderBottomRightRadius = box.borderBottomRightRadius.value_or(box.borderRadius.value_or(look.borderRadius));
        box.background = box.background.value_or(look.background);

        if (look.border.has_value()) {
            box.border = box.border.value_or(look.border.value());
        }

        Node(box, [&]{
            if (style.mark.has_value()) {
                auto mark = IconProps();
                mark.data.emplace(style.mark.value());
                mark.colorValue = style.markColor;
                mark.sizeValue = style.markSize;

                Icon(mark);
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
