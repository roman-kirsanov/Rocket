/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/NavLink.hpp>

using namespace Rocket;

namespace Storybook {

struct _NavLinkColors {
    Vec4 text;
    Vec4 hover;
    Vec4 active;
    Vec4 activeText;
};

struct _NavLinkState {
    std::function<void()> onClick;
    std::function<void(NodeEvent const&)> nodeOnEvent;
};

auto constexpr _gray1 = Vec4{ 241.0f / 255.0f, 243.0f / 255.0f, 245.0f / 255.0f, 1.0f }; /* #f1f3f5 */
auto constexpr _gray7 = Vec4{ 73.0f / 255.0f, 80.0f / 255.0f, 87.0f / 255.0f, 1.0f }; /* #495057 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */
auto constexpr _dark6 = Vec4{ 46.0f / 255.0f, 46.0f / 255.0f, 46.0f / 255.0f, 1.0f }; /* #2e2e2e */
auto constexpr _blue0 = Vec4{ 231.0f / 255.0f, 245.0f / 255.0f, 255.0f / 255.0f, 1.0f }; /* #e7f5ff */
auto constexpr _blue4 = Vec4{ 77.0f / 255.0f, 171.0f / 255.0f, 247.0f / 255.0f, 1.0f }; /* #4dabf7 */
auto constexpr _blue6Light = Vec4{ 34.0f / 255.0f, 139.0f / 255.0f, 230.0f / 255.0f, 0.15f }; /* #228be6 at 15% */
auto constexpr _blue7 = Vec4{ 28.0f / 255.0f, 126.0f / 255.0f, 214.0f / 255.0f, 1.0f }; /* #1c7ed6 */

static auto const _colors = std::map<ColorScheme, _NavLinkColors>{
    { ColorScheme::Light, { _gray7, _gray1, _blue0, _blue7 } },
    { ColorScheme::Dark, { _dark0, _dark6, _blue6Light, _blue4 } }
};

static auto const _paddingX = 12.0f;
static auto const _paddingY = 8.0f;
static auto const _borderRadius = 6.0f;
static auto const _fontSize = 14.0f;

void NavLink(NavLinkProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_NavLinkState>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);

    state.onClick = props.onClick;
    state.nodeOnEvent = props.nodeProps.onEvent;

    auto const& colors = _colors.at(UseColorScheme());
    auto const active = props.active.value_or(false);
    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(_paddingX));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(_paddingX));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(_paddingY));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(_paddingY));
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.cursor = node.cursor.value_or(Cursor::Pointer);
    node.fontSize = node.fontSize.value_or(_fontSize);
    node.fontWeight = node.fontWeight.value_or(active ? FontWeight::SemiBold : FontWeight::Medium);
    node.textColor = node.textColor.value_or(active ? colors.activeText : colors.text);

    if (active || isHover) {
        node.background = node.background.value_or(ColorBrush{ active ? colors.active : colors.hover });
    }

    node.onEvent = [&](NodeEvent const& event) {
        if (state.nodeOnEvent != nullptr) {
            state.nodeOnEvent(event);
        }

        if (event.is<MouseClickNodeEvent>()) {
            if (state.onClick != nullptr) {
                state.onClick();
            }

            event.stopPropagation();
        }
    };

    Node(node, [&]{
        Node({
            .display = NodeDisplay::Text,
            .content = props.label.value_or("")
        });
    });
}

} /* namespace Storybook */
