/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <array>
#include <cstdint>
#include <optional>
#include <tuple>
#include <Rocket/Base/Time.hpp>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Popup.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Tooltip.hpp>

namespace Rocket {

struct _TooltipLook {
    TooltipTheme::Style style;
    Brush background;
    Brush border;
    float borderWidth;
    float borderRadius;
    std::optional<MaterialArrowVariant> arrow;
};

struct _TooltipState {
    std::array<_TooltipLook, 4> looks;
    TooltipDirection side = TooltipDirection::Up;
    bool hover = false;
    bool visible = false;
    std::int64_t changeTime = 0;
    Effect<
        Theme const*,
        std::optional<TooltipIntent>,
        float
    > shouldUpdate;
};

static auto constexpr _directions = std::array<TooltipDirection, 4>{
    TooltipDirection::Up,
    TooltipDirection::Down,
    TooltipDirection::Left,
    TooltipDirection::Right
};
static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
static auto const _delayShow = 750.0;
static auto const _delayHide = 150.0;

static TooltipTheme::Style _ResolveStyle(TooltipTheme const& theme, TooltipIntent intent, TooltipDirection direction) {
    auto const rules = _GetCascade<TooltipTheme::Rule>(
        theme.rules,
        [&](TooltipTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.direction.has_value() == false || rule.selector.direction == direction)
            );
        },
        [](TooltipTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.direction.has_value() ? 1 : 0)
            );
        }
    );

    auto style = TooltipTheme::Style();

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
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.padding, rule->style.padding);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.arrow, rule->style.arrow);
        _Cascade(style.arrowSize, rule->style.arrowSize);
    }

    return style;
}

static MaterialArrowDirection _GetArrowDirection(TooltipDirection side) {
    return (
        side == TooltipDirection::Up ? MaterialArrowDirection::Down :
        side == TooltipDirection::Down ? MaterialArrowDirection::Up :
        side == TooltipDirection::Left ? MaterialArrowDirection::Right :
                                         MaterialArrowDirection::Left
    );
}

static TooltipDirection _Mirror(TooltipDirection side) {
    return (
        side == TooltipDirection::Up ? TooltipDirection::Down :
        side == TooltipDirection::Down ? TooltipDirection::Up :
        side == TooltipDirection::Left ? TooltipDirection::Right :
                                         TooltipDirection::Left
    );
}

static bool _IsVertical(TooltipDirection side) {
    return (side == TooltipDirection::Up || side == TooltipDirection::Down);
}

static Vec2 _GetArrowSize(_TooltipLook const& look, TooltipDirection side) {
    if (look.arrow.has_value()) {
        return look.arrow->size;
    }

    if (look.style.arrow.has_value() == false) {
        return Vec2{ 0.0f, 0.0f };
    }

    auto const size = look.style.arrowSize.value_or(Vec2{ 0.0f, 0.0f });

    return (_IsVertical(side) ? size : Vec2{ size.y, size.x });
}

static float _GetArrowDepth(_TooltipLook const& look, TooltipDirection side) {
    auto const size = _GetArrowSize(look, side);
    auto const depth = (_IsVertical(side) ? size.y : size.x);

    return (look.arrow.has_value() ? depth - look.arrow->overlap : depth);
}

static PopupOrigin _GetAnchorOrigin(TooltipDirection side, TooltipAlignment alignment) {
    auto const horizontal = (
        alignment == TooltipAlignment::Start ? PopupHorizontalOrigin::Left :
        alignment == TooltipAlignment::End ? PopupHorizontalOrigin::Right :
                                             PopupHorizontalOrigin::Center
    );
    auto const vertical = (
        alignment == TooltipAlignment::Start ? PopupVerticalOrigin::Top :
        alignment == TooltipAlignment::End ? PopupVerticalOrigin::Bottom :
                                             PopupVerticalOrigin::Center
    );

    return (
        side == TooltipDirection::Up ? PopupOrigin{ PopupVerticalOrigin::Top, horizontal } :
        side == TooltipDirection::Down ? PopupOrigin{ PopupVerticalOrigin::Bottom, horizontal } :
        side == TooltipDirection::Left ? PopupOrigin{ vertical, PopupHorizontalOrigin::Left } :
                                         PopupOrigin{ vertical, PopupHorizontalOrigin::Right }
    );
}

static void _Arrow(_TooltipLook const& look, TooltipDirection side, TooltipAlignment alignment, Vec4 const& anchor) {
    auto const size = _GetArrowSize(look, side);

    if (size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }

    auto const vertical = _IsVertical(side);
    auto const inset = (look.arrow.has_value() ? look.arrow->overlap : 0.0f);
    auto const across = (
        vertical
            ? (anchor.width - size.x) * 0.5f
            : (anchor.height - size.y) * 0.5f
    );

    auto node = NodeProps();
    node.position = NodePosition::Absolute;
    node.width = size.x;
    node.height = size.y;

    if (side == TooltipDirection::Up) {
        node.top = NodeValue(PercentValue{ 100.0f });
        node.offset = Vec2{ 0.0f, look.borderWidth - inset };
    } else if (side == TooltipDirection::Down) {
        node.bottom = NodeValue(PercentValue{ 100.0f });
        node.offset = Vec2{ 0.0f, inset - look.borderWidth };
    } else if (side == TooltipDirection::Left) {
        node.left = NodeValue(PercentValue{ 100.0f });
        node.offset = Vec2{ look.borderWidth - inset, 0.0f };
    } else {
        node.right = NodeValue(PercentValue{ 100.0f });
        node.offset = Vec2{ inset - look.borderWidth, 0.0f };
    }

    if (alignment == TooltipAlignment::Center) {
        if (vertical) {
            node.left = NodeValue(PercentValue{ 50.0f });
            node.transform = NodeTransform{ .translateX = PercentValue{ -50.0f } };
        } else {
            node.top = NodeValue(PercentValue{ 50.0f });
            node.transform = NodeTransform{ .translateY = PercentValue{ -50.0f } };
        }
    } else if (alignment == TooltipAlignment::Start) {
        if (vertical) {
            node.left = across - look.borderWidth;
        } else {
            node.top = across - look.borderWidth;
        }
    } else {
        if (vertical) {
            node.right = across - look.borderWidth;
        } else {
            node.bottom = across - look.borderWidth;
        }
    }

    if (look.arrow.has_value()) {
        node.background = look.arrow->brush;

        Node(node);

        return;
    }

    Node(node, [&]{
        auto const shift = (
            side == TooltipDirection::Up ? Vec2{ 0.0f, -look.borderWidth } :
            side == TooltipDirection::Down ? Vec2{ 0.0f, look.borderWidth } :
            side == TooltipDirection::Left ? Vec2{ -look.borderWidth, 0.0f } :
                                             Vec2{ look.borderWidth, 0.0f }
        );

        for (auto const& [color, offset] : std::array<std::tuple<Vec4, Vec2>, 2>{
            std::tuple(look.style.borderColor.value_or(_transparent), Vec2{ 0.0f, 0.0f }),
            std::tuple(look.style.background.value_or(_transparent), shift)
        }) {
            auto icon = IconProps();
            icon.data.emplace(look.style.arrow.value());
            icon.colorValue = color;
            icon.sizeValue = size;
            icon.nodeProps.position = NodePosition::Absolute;
            icon.nodeProps.left = 0.0f;
            icon.nodeProps.top = 0.0f;
            icon.nodeProps.offset = offset;

            Icon(icon);
        }
    });
}

void Tooltip(TooltipProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto& state = UseState<_TooltipState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto const& theme = UseTheme();

    if (state.shouldUpdate(&theme, props.intent, document.getScale())) {
        for (auto index = 0llu; index < _directions.size(); index++) {
            auto const style = _ResolveStyle(theme.tooltip, props.intent.value_or(TooltipIntent::Default), _directions[index]);
            auto& look = state.looks[index];
            look.style = style;
            look.arrow = std::nullopt;

            if (style.materialKind.has_value() && theme.material != nullptr) {
                look.background = theme.material->getVariant({
                    .kind = style.materialKind,
                    .intent = style.materialIntent,
                    .brightness = style.materialBrightness,
                    .roundness = style.materialRoundness,
                    .scale = document.getScale()
                }).brush;
                look.border = ColorBrush{ _transparent };
                look.borderWidth = 0.0f;
                look.borderRadius = 0.0f;
                look.arrow = theme.material->getArrowVariant({
                    .direction = _GetArrowDirection(_directions[index]),
                    .intent = style.materialIntent.value_or(MaterialIntent::Default),
                    .brightness = style.materialBrightness,
                    .scale = document.getScale()
                });
            } else {
                look.background = ColorBrush{ style.background.value_or(_transparent) };
                look.border = ColorBrush{ style.borderColor.value_or(_transparent) };
                look.borderWidth = style.borderWidth.value_or(0.0f);
                look.borderRadius = style.borderRadius.value_or(0.0f);
            }
        }
    }

    auto const now = GetTime();
    auto const hover = (nodeRef != nullptr && nodeRef->isHover());
    auto const delay = (hover ? props.delayShow.value_or(_delayShow) : props.delayHide.value_or(_delayHide));

    if (hover != state.hover) {
        state.hover = hover;
        state.changeTime = now;
    }

    if (state.visible != hover && static_cast<double>(now - state.changeTime) >= delay) {
        state.visible = hover;
    }

    auto const direction = props.direction.value_or(TooltipDirection::Up);
    auto const alignment = props.alignment.value_or(TooltipAlignment::Center);

    if (state.visible == false) {
        state.side = direction;
    }

    auto node = props.nodeProps;
    node.ref = &nodeRef;

    Node(node, [&]{
        children();

        if (state.visible == false || props.text.has_value() == false) {
            return;
        }

        auto const& look = state.looks[static_cast<std::size_t>(state.side)];
        auto const& preferred = state.looks[static_cast<std::size_t>(direction)];
        auto const& style = look.style;
        auto const padding = style.padding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
        auto const distance = props.gap.value_or(preferred.style.gap.value_or(0.0f)) + _GetArrowDepth(preferred, direction);
        auto const anchor = nodeRef->getComputedBorderRect();

        auto popup = props.popupProps;
        popup.overflowX = popup.overflowX.value_or(NodeOverflow::Visible);
        popup.overflowY = popup.overflowY.value_or(NodeOverflow::Visible);
        popup.zIndex = popup.zIndex.value_or(TOOLTIP_ZINDEX);
        popup.background = popup.background.value_or(look.background);
        popup.border = popup.border.value_or(look.border);
        popup.borderWidth = popup.borderWidth.value_or(look.borderWidth);
        popup.borderRadius = popup.borderRadius.value_or(look.borderRadius);
        popup.shadow = popup.shadow.value_or(style.shadow.value_or(Shadow{ .color = _transparent }));

        if (style.color.has_value()) {
            popup.textColor = popup.textColor.value_or(style.color.value());
        }

        if (style.fontSize.has_value()) {
            popup.fontSize = popup.fontSize.value_or(style.fontSize.value());
        }

        Popup({
            .anchorOrigin = _GetAnchorOrigin(direction, alignment),
            .transformOrigin = _GetAnchorOrigin(_Mirror(direction), alignment),
            .collision = PopupCollision::Flip,
            .offset = (
                direction == TooltipDirection::Up ? Vec2{ 0.0f, -distance } :
                direction == TooltipDirection::Down ? Vec2{ 0.0f, distance } :
                direction == TooltipDirection::Left ? Vec2{ -distance, 0.0f } :
                                                      Vec2{ distance, 0.0f }
            ),
            .onFlip = [&, direction](PopupFlip const& flip) {
                auto const flipped = (_IsVertical(direction) ? flip.vertical : flip.horizontal);
                state.side = (flipped ? _Mirror(direction) : direction);
            },
            .onClose = [&]{
                state.visible = false;
            },
            .header = [&, alignment, anchor]{
                _Arrow(state.looks[static_cast<std::size_t>(state.side)], state.side, alignment, anchor);
            },
            .contentProps = {
                .paddingLeft = padding.left,
                .paddingTop = padding.top,
                .paddingRight = padding.right,
                .paddingBottom = padding.bottom
            },
            .nodeProps = popup
        }, [&]{
            auto text = props.textProps;
            text.display = NodeDisplay::Text;
            text.content = text.content.value_or(props.text.value());

            Node(text);
        });
    });
}

} /* namespace Rocket */
