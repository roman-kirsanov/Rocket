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
#include <Rocket/UI/Switch.hpp>

namespace Rocket {

struct _SwitchLook {
    SwitchTheme::Style style;
    Brush track;
    std::optional<Brush> border;
    float borderWidth;
    Vec2 trackSize;
    Brush knob;
    float knobSize;
};

struct _SwitchState {
    std::array<_SwitchLook, 8> looks;
    std::function<void(bool)> onChange;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    Effect<
        Theme const*,
        std::optional<SwitchIntent>,
        std::optional<SwitchSize>,
        float
    > shouldUpdate;
};

static auto constexpr _states = std::array<SwitchTheme::State, 4>{
    SwitchTheme::State::Normal,
    SwitchTheme::State::Hover,
    SwitchTheme::State::Active,
    SwitchTheme::State::Disabled
};
static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static SwitchTheme::Style _ResolveStyle(SwitchTheme const& theme, SwitchIntent intent, SwitchSize size, SwitchTheme::State state, bool checked) {
    auto const rules = _GetCascade<SwitchTheme::Rule>(
        theme.rules,
        [&](SwitchTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.size.has_value() == false || rule.selector.size == size) &&
                (rule.selector.state.has_value() == false || rule.selector.state == state) &&
                (rule.selector.checked.has_value() == false || rule.selector.checked == checked)
            );
        },
        [](SwitchTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.size.has_value() ? 1 : 0) +
                (rule.selector.state.has_value() ? 1 : 0) +
                (rule.selector.checked.has_value() ? 1 : 0)
            );
        }
    );

    auto style = SwitchTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.background, rule->style.background);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.trackSize, rule->style.trackSize);
        _Cascade(style.knobMaterialKind, rule->style.knobMaterialKind);
        _Cascade(style.knobMaterialIntent, rule->style.knobMaterialIntent);
        _Cascade(style.knobMaterialBrightness, rule->style.knobMaterialBrightness);
        _Cascade(style.knobBackground, rule->style.knobBackground);
        _Cascade(style.knobSize, rule->style.knobSize);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.opacity, rule->style.opacity);
    }

    return style;
}

void Switch(SwitchProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SwitchState>();
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
                theme.switch_,
                props.intent.value_or(SwitchIntent::Default),
                props.size.value_or(SwitchSize::Medium),
                _states[index % _states.size()],
                (index >= _states.size())
            );
            auto const trackSize = style.trackSize.value_or(Vec2{ 0.0f, 0.0f });
            auto& look = state.looks[index];
            look.style = style;

            if (style.materialKind.has_value() && theme.material != nullptr) {
                auto const capsule = theme.material->getCapsuleVariant({
                    .kind = style.materialKind,
                    .intent = style.materialIntent,
                    .brightness = style.materialBrightness,
                    .width = trackSize.width,
                    .height = trackSize.height,
                    .scale = document.getScale()
                });

                look.track = capsule.brush;
                look.border = std::nullopt;
                look.borderWidth = 0.0f;
                look.trackSize = capsule.size;
            } else {
                look.track = ColorBrush{ style.background.value_or(_transparent) };
                look.border = ColorBrush{ style.borderColor.value_or(_transparent) };
                look.borderWidth = style.borderWidth.value_or(0.0f);
                look.trackSize = trackSize;
            }

            if (style.knobMaterialKind.has_value() && theme.material != nullptr) {
                auto const circle = theme.material->getCircleVariant({
                    .kind = style.knobMaterialKind,
                    .intent = style.knobMaterialIntent,
                    .brightness = style.knobMaterialBrightness,
                    .width = style.knobSize.value_or(0.0f),
                    .scale = document.getScale()
                });

                look.knob = circle.brush;
                look.knobSize = circle.size.width;
            } else {
                look.knob = ColorBrush{ style.knobBackground.value_or(_transparent) };
                look.knobSize = style.knobSize.value_or(0.0f);
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
    auto const inset = ((look.trackSize.height - look.knobSize) / 2.0f);

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
        auto track = props.trackProps;
        track.direction = track.direction.value_or(NodeDirection::Horizontal);
        track.alignment = track.alignment.value_or(NodeAlignment::Center);
        track.justify = track.justify.value_or(checked ? NodeJustify::End : NodeJustify::Start);
        track.width = track.width.value_or(look.trackSize.width);
        track.height = track.height.value_or(look.trackSize.height);
        track.paddingLeft = track.paddingLeft.value_or(track.padding.value_or(inset));
        track.paddingRight = track.paddingRight.value_or(track.padding.value_or(inset));
        track.borderLeftWidth = track.borderLeftWidth.value_or(track.borderWidth.value_or(look.borderWidth));
        track.borderTopWidth = track.borderTopWidth.value_or(track.borderWidth.value_or(look.borderWidth));
        track.borderRightWidth = track.borderRightWidth.value_or(track.borderWidth.value_or(look.borderWidth));
        track.borderBottomWidth = track.borderBottomWidth.value_or(track.borderWidth.value_or(look.borderWidth));
        track.borderTopLeftRadius = track.borderTopLeftRadius.value_or(track.borderRadius.value_or(look.trackSize.height / 2.0f));
        track.borderTopRightRadius = track.borderTopRightRadius.value_or(track.borderRadius.value_or(look.trackSize.height / 2.0f));
        track.borderBottomLeftRadius = track.borderBottomLeftRadius.value_or(track.borderRadius.value_or(look.trackSize.height / 2.0f));
        track.borderBottomRightRadius = track.borderBottomRightRadius.value_or(track.borderRadius.value_or(look.trackSize.height / 2.0f));
        track.background = track.background.value_or(look.track);

        if (look.border.has_value()) {
            track.border = track.border.value_or(look.border.value());
        }

        Node(track, [&]{
            Node({
                .width = look.knobSize,
                .height = look.knobSize,
                .borderRadius = (look.knobSize / 2.0f),
                .background = look.knob
            });
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
