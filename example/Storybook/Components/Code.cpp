/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/Code.hpp>

using namespace Rocket;

namespace Storybook {

struct _CodeColors {
    Vec4 background;
    Vec4 text;
};

auto constexpr _gray1 = Vec4{ 241.0f / 255.0f, 243.0f / 255.0f, 245.0f / 255.0f, 1.0f }; /* #f1f3f5 */
auto constexpr _gray9 = Vec4{ 33.0f / 255.0f, 37.0f / 255.0f, 41.0f / 255.0f, 1.0f }; /* #212529 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */
auto constexpr _dark5 = Vec4{ 59.0f / 255.0f, 59.0f / 255.0f, 59.0f / 255.0f, 1.0f }; /* #3b3b3b */

static auto const _colors = std::map<ColorScheme, _CodeColors>{
    { ColorScheme::Light, { _gray1, _gray9 } },
    { ColorScheme::Dark, { _dark5, _dark0 } }
};

static auto const _fontFamily = std::string("Mono");
static auto const _fontSize = 13.0f;
static auto const _paddingX = 5.0f;
static auto const _paddingY = 2.0f;
static auto const _borderRadius = 4.0f;

void Code(CodeProps const& props) {
    PROFILE
    COMPONENT

    auto const& colors = _colors.at(UseColorScheme());

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.selfAlignment = node.selfAlignment.value_or(NodeAlignment::Start);
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(_paddingX));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(_paddingX));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(_paddingY));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(_paddingY));
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.background = node.background.value_or(ColorBrush{ colors.background });
    node.fontFamily = node.fontFamily.value_or(_fontFamily);
    node.fontWeight = node.fontWeight.value_or(FontWeight::Normal);
    node.fontSize = node.fontSize.value_or(_fontSize);
    node.textColor = node.textColor.value_or(colors.text);

    Node(node, [&]{
        auto text = props.textProps;
        text.display = NodeDisplay::Text;
        text.content = text.content.value_or(props.content.value_or(""));

        Node(text);
    });
}

} /* namespace Storybook */
