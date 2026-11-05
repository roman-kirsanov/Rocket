/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/Text.hpp>

using namespace Rocket;

namespace Storybook {

struct _TextColors {
    Vec4 text;
    Vec4 dimmed;
};

auto constexpr _gray6 = Vec4{ 134.0f / 255.0f, 142.0f / 255.0f, 150.0f / 255.0f, 1.0f }; /* #868e96 */
auto constexpr _gray9 = Vec4{ 33.0f / 255.0f, 37.0f / 255.0f, 41.0f / 255.0f, 1.0f }; /* #212529 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */
auto constexpr _dark2 = Vec4{ 130.0f / 255.0f, 130.0f / 255.0f, 130.0f / 255.0f, 1.0f }; /* #828282 */

static auto const _colors = std::map<ColorScheme, _TextColors>{
    { ColorScheme::Light, { _gray9, _gray6 } },
    { ColorScheme::Dark, { _dark0, _dark2 } }
};

static auto const _fontSizes = std::map<TextSize, float>{
    { TextSize::Medium, 15.0f },
    { TextSize::Small, 13.0f }
};
static auto const _lineHeight = 1.55f;

void Text(TextProps const& props) {
    PROFILE
    COMPONENT

    auto const& colors = _colors.at(UseColorScheme());

    auto node = props.nodeProps;
    node.display = node.display.value_or(NodeDisplay::Text);
    node.fontSize = node.fontSize.value_or(_fontSizes.at(props.size.value_or(TextSize::Medium)));
    node.lineHeight = node.lineHeight.value_or(_lineHeight);
    node.textColor = node.textColor.value_or(props.dimmed.value_or(false) ? colors.dimmed : colors.text);
    node.content = node.content.value_or(props.content.value_or(""));

    Node(node);
}

} /* namespace Storybook */
