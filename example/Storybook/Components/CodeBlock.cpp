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
#include <Components/CodeBlock.hpp>

using namespace Rocket;

namespace Storybook {

struct _CodeBlockColors {
    Vec4 background;
    Vec4 border;
    Vec4 text;
};

auto constexpr _gray0 = Vec4{ 248.0f / 255.0f, 249.0f / 255.0f, 250.0f / 255.0f, 1.0f }; /* #f8f9fa */
auto constexpr _gray2 = Vec4{ 233.0f / 255.0f, 236.0f / 255.0f, 239.0f / 255.0f, 1.0f }; /* #e9ecef */
auto constexpr _gray9 = Vec4{ 33.0f / 255.0f, 37.0f / 255.0f, 41.0f / 255.0f, 1.0f }; /* #212529 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */
auto constexpr _dark4 = Vec4{ 66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.0f }; /* #424242 */
auto constexpr _dark8 = Vec4{ 31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f }; /* #1f1f1f */

static auto const _colors = std::map<ColorScheme, _CodeBlockColors>{
    { ColorScheme::Light, { _gray0, _gray2, _gray9 } },
    { ColorScheme::Dark, { _dark8, _dark4, _dark0 } }
};

static auto const _fontFamily = std::string("Mono");
static auto const _fontSize = 13.0f;
static auto const _lineHeight = 1.6f;
static auto const _padding = 16.0f;
static auto const _borderWidth = 1.0f;
static auto const _borderRadius = 8.0f;

void CodeBlock(CodeBlockProps const& props) {
    PROFILE
    COMPONENT

    auto const& colors = _colors.at(UseColorScheme());

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(_padding));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(_padding));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(_padding));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(_padding));
    node.borderWidth = node.borderWidth.value_or(_borderWidth);
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.border = node.border.value_or(ColorBrush{ colors.border });
    node.background = node.background.value_or(ColorBrush{ colors.background });
    node.fontFamily = node.fontFamily.value_or(_fontFamily);
    node.fontWeight = node.fontWeight.value_or(FontWeight::Normal);
    node.fontSize = node.fontSize.value_or(_fontSize);
    node.lineHeight = node.lineHeight.value_or(_lineHeight);
    node.textColor = node.textColor.value_or(colors.text);

    Node(node, [&]{
        auto text = props.textProps;
        text.display = NodeDisplay::Text;
        text.content = text.content.value_or(props.content.value_or(""));

        Node(text);
    });
}

} /* namespace Storybook */
