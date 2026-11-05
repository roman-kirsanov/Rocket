/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/Title.hpp>

using namespace Rocket;

namespace Storybook {

struct _TitleColors {
    Vec4 text;
};

auto constexpr _gray9 = Vec4{ 33.0f / 255.0f, 37.0f / 255.0f, 41.0f / 255.0f, 1.0f }; /* #212529 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */

static auto const _colors = std::map<ColorScheme, _TitleColors>{
    { ColorScheme::Light, { _gray9 } },
    { ColorScheme::Dark, { _dark0 } }
};

static auto const _fontSizes = std::map<TitleOrder, float>{
    { TitleOrder::H1, 30.0f },
    { TitleOrder::H2, 22.0f },
    { TitleOrder::H3, 17.0f }
};

void Title(TitleProps const& props) {
    PROFILE
    COMPONENT

    auto const& colors = _colors.at(UseColorScheme());

    auto node = props.nodeProps;
    node.display = node.display.value_or(NodeDisplay::Text);
    node.fontSize = node.fontSize.value_or(_fontSizes.at(props.order.value_or(TitleOrder::H1)));
    node.fontWeight = node.fontWeight.value_or(FontWeight::Bold);
    node.textColor = node.textColor.value_or(colors.text);
    node.content = node.content.value_or(props.content.value_or(""));

    Node(node);
}

} /* namespace Storybook */
