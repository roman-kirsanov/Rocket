/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Svg.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/ColorSchemeToggle.hpp>

using namespace Rocket;

namespace Storybook {

struct _ColorSchemeToggleColors {
    Vec4 background;
    Vec4 hover;
    Vec4 border;
    Vec4 icon;
};

struct _ColorSchemeToggleState {
    std::unique_ptr<Image> icon;
    std::function<void()> onClick;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    Effect<float, ColorScheme> shouldUpdateIcon;
};

auto constexpr _gray0 = Vec4{ 248.0f / 255.0f, 249.0f / 255.0f, 250.0f / 255.0f, 1.0f }; /* #f8f9fa */
auto constexpr _gray3 = Vec4{ 222.0f / 255.0f, 226.0f / 255.0f, 230.0f / 255.0f, 1.0f }; /* #dee2e6 */
auto constexpr _gray7 = Vec4{ 73.0f / 255.0f, 80.0f / 255.0f, 87.0f / 255.0f, 1.0f };    /* #495057 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */
auto constexpr _dark4 = Vec4{ 66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.0f };    /* #424242 */
auto constexpr _dark5 = Vec4{ 59.0f / 255.0f, 59.0f / 255.0f, 59.0f / 255.0f, 1.0f };    /* #3b3b3b */
auto constexpr _dark6 = Vec4{ 46.0f / 255.0f, 46.0f / 255.0f, 46.0f / 255.0f, 1.0f };    /* #2e2e2e */
auto constexpr _white = COLOR_WHITE;

static auto const _colors = std::map<ColorScheme, _ColorSchemeToggleColors>{
    { ColorScheme::Light, { _white, _gray0, _gray3, _gray7 } },
    { ColorScheme::Dark, { _dark6, _dark5, _dark4, _dark0 } }
};
static auto const _icons = std::map<ColorScheme, std::string>{
    {
        ColorScheme::Light,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
        "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z\"/></svg>"
    },
    {
        ColorScheme::Dark,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
        "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" r=\"4\"/><path d=\"M12 2v2M12 20v2M4.93 4.93l1.41 1.41"
        "M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41\"/></svg>"
    }
};
static auto const _size = 30.0f;
static auto const _iconSize = 16.0f;
static auto const _borderWidth = 1.0f;
static auto const _borderRadius = 6.0f;

void ColorSchemeToggle(ColorSchemeToggleProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_ColorSchemeToggleState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);

    state.onClick = props.onClick;
    state.nodeOnEvent = props.nodeProps.onEvent;

    auto const colorScheme = UseColorScheme();
    auto const& colors = _colors.at(colorScheme);

    if (state.shouldUpdateIcon(document.getScale(), colorScheme)) {
        auto const size = Vec2{ (_iconSize * document.getScale()), (_iconSize * document.getScale()) };
        auto pixels = std::vector<std::uint8_t>();

        ConvertSVGToBitmap(_icons.at(colorScheme), size, colors.icon, pixels);
        state.icon = std::make_unique<Image>(size, pixels.data());
    }

    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.justify = node.justify.value_or(NodeJustify::Center);
    node.width = node.width.value_or(_size);
    node.height = node.height.value_or(_size);
    node.borderWidth = node.borderWidth.value_or(_borderWidth);
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.cursor = node.cursor.value_or(Cursor::Pointer);
    node.background = node.background.value_or(ColorBrush{ isHover ? colors.hover : colors.background });
    node.border = node.border.value_or(ColorBrush{ colors.border });
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
            .width = _iconSize,
            .height = _iconSize,
            .background = ImageBrush{
                .image = state.icon.get(),
                .positionX = ImagePosition::Stretch,
                .positionY = ImagePosition::Stretch
            }
        });
    });
}

} /* namespace Storybook */
