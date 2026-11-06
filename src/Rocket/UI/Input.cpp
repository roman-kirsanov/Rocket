/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/Window/Input.hpp>
#include <Rocket/UI/Input.hpp>

namespace Rocket {

struct _InputLook {
    InputTheme::Style style;
    Brush background;
    std::optional<Brush> border;
    float borderWidth;
    float borderRadius;
};

struct _InputState {
    std::array<_InputLook, 4> looks;
    std::function<void(std::string const&)> onChange;
    std::function<void()> onEnter;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    std::function<void(NodeEvent const&)> fieldOnEvent;
    std::optional<std::size_t> maxLength;
    bool focused = false;
    Effect<
        Theme const*,
        std::optional<InputIntent>,
        std::optional<InputSize>,
        float
    > shouldUpdate;
};

static auto constexpr _states = std::array<InputTheme::State, 4>{
    InputTheme::State::Normal,
    InputTheme::State::Hover,
    InputTheme::State::Focus,
    InputTheme::State::Disabled
};
static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static InputTheme::Style _ResolveStyle(InputTheme const& theme, InputIntent intent, InputSize size, InputTheme::State state) {
    auto const rules = _GetCascade<InputTheme::Rule>(
        theme.rules,
        [&](InputTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.size.has_value() == false || rule.selector.size == size) &&
                (rule.selector.state.has_value() == false || rule.selector.state == state)
            );
        },
        [](InputTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.size.has_value() ? 1 : 0) +
                (rule.selector.state.has_value() ? 1 : 0)
            );
        }
    );

    auto style = InputTheme::Style();

    for (auto const rule : rules) {
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
        _Cascade(style.placeholderColor, rule->style.placeholderColor);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.padding, rule->style.padding);
        _Cascade(style.minHeight, rule->style.minHeight);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.opacity, rule->style.opacity);
    }

    return style;
}

static std::size_t _GetLength(std::string const& text) {
    auto length = 0llu;

    for (auto const character : text) {
        if ((static_cast<unsigned char>(character) & 0xC0) != 0x80) {
            length++;
        }
    }

    return length;
}

void Input(InputProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_InputState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto& fieldRef = UseRef<class Node>(props.fieldProps.ref);
    auto& textRef = UseRef<class Node>(props.textProps.ref);
    auto const& theme = UseTheme();

    auto const disabled = props.disabled.value_or(false);
    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());
    auto const isFocus = (fieldRef != nullptr && fieldRef->isFocused());

    state.onChange = props.onChange;
    state.onEnter = props.onEnter;
    state.nodeOnEvent = props.nodeProps.onEvent;
    state.fieldOnEvent = props.fieldProps.onEvent;
    state.maxLength = props.maxLength;

    if (state.shouldUpdate(&theme, props.intent, props.size, document.getScale())) {
        for (auto index = 0llu; index < state.looks.size(); index++) {
            auto const style = _ResolveStyle(
                theme.input,
                props.intent.value_or(InputIntent::Default),
                props.size.value_or(InputSize::Medium),
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

    if (props.focus.value_or(false) && state.focused == false && fieldRef != nullptr && disabled == false) {
        document.focusNode(fieldRef);
        state.focused = true;
    }

    auto const& look = state.looks[
        disabled ? 3llu :
        isFocus ? 2llu :
        isHover ? 1llu :
                  0llu
    ];
    auto const& style = look.style;
    auto const padding = style.padding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    auto const value = props.value.value_or("");
    auto const empty = (textRef != nullptr ? textRef->getContent().value_or("").empty() : value.empty());

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.gapX = node.gapX.value_or(node.gap.value_or(style.gap.value_or(0.0f)));
    node.borderLeftWidth = node.borderLeftWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderTopWidth = node.borderTopWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderRightWidth = node.borderRightWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderBottomWidth = node.borderBottomWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderRadius = node.borderRadius.value_or(look.borderRadius);
    node.background = node.background.value_or(look.background);
    node.cursor = node.cursor.value_or(Cursor::Text);
    node.onEvent = [&](NodeEvent const& event) {
        if (state.nodeOnEvent != nullptr) {
            state.nodeOnEvent(event);
        }

        if (event.is<MouseDownNodeEvent>() && fieldRef != nullptr && fieldRef->containsNode(event.getNode()) == false) {
            document.focusNode(fieldRef);
            event.preventDefault();
        }
    };

    if (look.border.has_value()) {
        node.border = node.border.value_or(look.border.value());
    }

    if (style.shadow.has_value()) {
        node.shadow = node.shadow.value_or(style.shadow.value());
    }

    if (style.minHeight.has_value()) {
        node.minHeight = node.minHeight.value_or(style.minHeight.value());
    }

    if (style.color.has_value()) {
        node.textColor = node.textColor.value_or(style.color.value());
    }

    if (style.fontSize.has_value()) {
        node.fontSize = node.fontSize.value_or(style.fontSize.value());
    }

    if (style.opacity.has_value()) {
        node.opacity = node.opacity.value_or(style.opacity.value());
    }

    if (disabled) {
        node.mouseEvents = node.mouseEvents.value_or(false);
    }

    auto field = props.fieldProps;
    field.ref = &fieldRef;
    field.direction = field.direction.value_or(NodeDirection::Horizontal);
    field.alignment = field.alignment.value_or(NodeAlignment::Center);
    field.paddingLeft = field.paddingLeft.value_or(field.padding.value_or(padding.left));
    field.paddingTop = field.paddingTop.value_or(field.padding.value_or(padding.top));
    field.paddingRight = field.paddingRight.value_or(field.padding.value_or(padding.right));
    field.paddingBottom = field.paddingBottom.value_or(field.padding.value_or(padding.bottom));
    field.flex = field.flex.value_or(true);
    field.selfAlignment = field.selfAlignment.value_or(NodeAlignment::Stretch);
    field.editable = field.editable.value_or(disabled == false);
    field.secure = field.secure.value_or(props.password.value_or(false));
    field.onEvent = [&](NodeEvent const& event) {
        if (state.fieldOnEvent != nullptr) {
            state.fieldOnEvent(event);
        }

        if (auto const input = event.as<InputNodeEvent>()) {
            if (state.onChange != nullptr) {
                state.onChange(input->getContent());
            }
        } else if (auto const beforeInput = event.as<BeforeInputNodeEvent>()) {
            auto const content = (textRef != nullptr ? textRef->getContent().value_or("") : std::string());

            if (state.maxLength.has_value() && _GetLength(content) + _GetLength(beforeInput->getText()) > state.maxLength.value()) {
                event.preventDefault();
            }
        } else if (auto const keyDown = event.as<KeyDownNodeEvent>()) {
            if (keyDown->getKeycode() == Keycode::Enter && state.onEnter != nullptr) {
                state.onEnter();
            }
        }
    };

    Node(node, [&]{
        if (props.prefix != nullptr) {
            props.prefix();
        }

        Node({
            .direction = NodeDirection::Horizontal,
            .selfAlignment = NodeAlignment::Stretch,
            .flex = true
        }, [&]{
            Node(field, [&]{
                auto text = props.textProps;
                text.ref = &textRef;
                text.display = NodeDisplay::Text;
                text.content = text.content.value_or(value);

                Node(text);
            });

            if (empty && props.placeholder.has_value()) {
                auto placeholder = props.placeholderProps;
                placeholder.display = NodeDisplay::Text;
                placeholder.content = placeholder.content.value_or(props.placeholder.value());

                if (style.placeholderColor.has_value()) {
                    placeholder.textColor = placeholder.textColor.value_or(style.placeholderColor.value());
                }

                Node({
                    .position = NodePosition::Absolute,
                    .direction = NodeDirection::Horizontal,
                    .alignment = NodeAlignment::Center,
                    .top = 0.0f,
                    .left = 0.0f,
                    .right = 0.0f,
                    .bottom = 0.0f,
                    .paddingLeft = padding.left,
                    .paddingRight = padding.right,
                    .mouseEvents = false
                }, [&]{
                    Node(placeholder);
                });
            }
        });

        if (props.suffix != nullptr) {
            props.suffix();
        }
    });
}

} /* namespace Rocket */
