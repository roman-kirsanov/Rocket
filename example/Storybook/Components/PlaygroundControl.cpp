/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/PlaygroundControl.hpp>

using namespace Rocket;

namespace Storybook {

struct _PlaygroundControlColors {
    Vec4 label;
};

auto constexpr _gray7 = Vec4{ 73.0f / 255.0f, 80.0f / 255.0f, 87.0f / 255.0f, 1.0f }; /* #495057 */
auto constexpr _dark1 = Vec4{ 184.0f / 255.0f, 184.0f / 255.0f, 184.0f / 255.0f, 1.0f }; /* #b8b8b8 */

static auto const _colors = std::map<ColorScheme, _PlaygroundControlColors>{
    { ColorScheme::Light, { _gray7 } },
    { ColorScheme::Dark, { _dark1 } }
};

static auto const _gap = 6.0f;
static auto const _fontSize = 13.0f;

void PlaygroundControl(PlaygroundControlProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto const& colors = _colors.at(UseColorScheme());

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    node.alignment = node.alignment.value_or(NodeAlignment::Stretch);
    node.gapY = node.gapY.value_or(node.gap.value_or(_gap));

    Node(node, [&]{
        Node({
            .display = NodeDisplay::Text,
            .fontWeight = FontWeight::Medium,
            .fontSize = _fontSize,
            .textColor = colors.label,
            .content = props.label.value_or("")
        });

        children();
    });
}

} /* namespace Storybook */
