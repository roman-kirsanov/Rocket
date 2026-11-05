/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/Stage.hpp>

using namespace Rocket;

namespace Storybook {

auto constexpr _dark8 = Vec4{ 31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f };     /* #1f1f1f */
auto constexpr _dark2 = Vec4{ 130.0f / 255.0f, 130.0f / 255.0f, 130.0f / 255.0f, 1.0f };  /* #828282 */
auto constexpr _gray2 = Vec4{ 233.0f / 255.0f, 236.0f / 255.0f, 239.0f / 255.0f, 1.0f };  /* #e9ecef */
auto constexpr _gray6 = Vec4{ 134.0f / 255.0f, 142.0f / 255.0f, 150.0f / 255.0f, 1.0f };  /* #868e96 */
auto constexpr _white = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f };

static auto const _padding = 24.0f;
static auto const _gap = 12.0f;
static auto const _borderWidth = 1.0f;
static auto const _borderRadius = 8.0f;

StageColors GetDarkStageColors() {
    return StageColors{ .background = _dark8, .border = _dark8, .text = _dark2 };
}

StageColors GetLightStageColors() {
    return StageColors{ .background = _white, .border = _gray2, .text = _gray6 };
}

StageColors UseStageColors() {
    PROFILE

    auto const colors = UseContextIf<StageColors const>();

    return (colors != nullptr ? *colors : GetDarkStageColors());
}

void Stage(StageProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto const colors = props.colors.value_or(UseStageColors());

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.wrap = node.wrap.value_or(NodeWrap::Wrap);
    node.padding = node.padding.value_or(_padding);
    node.gap = node.gap.value_or(_gap);
    node.borderWidth = node.borderWidth.value_or(_borderWidth);
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.background = node.background.value_or(ColorBrush{ colors.background });
    node.border = node.border.value_or(ColorBrush{ colors.border });

    Node(node, children);
}

} /* namespace Storybook */
