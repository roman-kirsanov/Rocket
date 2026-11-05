/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/CodeBlock.hpp>
#include <Components/Playground.hpp>

using namespace Rocket;

namespace Storybook {

struct _PlaygroundColors {
    Vec4 background;
    Vec4 border;
    Vec4 controls;
};

auto constexpr _gray0 = Vec4{ 248.0f / 255.0f, 249.0f / 255.0f, 250.0f / 255.0f, 1.0f }; /* #f8f9fa */
auto constexpr _gray3 = Vec4{ 222.0f / 255.0f, 226.0f / 255.0f, 230.0f / 255.0f, 1.0f }; /* #dee2e6 */
auto constexpr _dark4 = Vec4{ 66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.0f }; /* #424242 */
auto constexpr _dark7 = Vec4{ 36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f }; /* #242424 */
auto constexpr _dark8 = Vec4{ 31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f }; /* #1f1f1f */
auto constexpr _white = COLOR_WHITE;

static auto const _colors = std::map<ColorScheme, _PlaygroundColors>{
    { ColorScheme::Light, { _white, _gray3, _gray0 } },
    { ColorScheme::Dark, { _dark7, _dark4, _dark8 } }
};

static auto const _borderWidth = 1.0f;
static auto const _borderRadius = 8.0f;
static auto const _innerBorderRadius = (_borderRadius - _borderWidth);
static auto const _previewMinHeight = 320.0f;
static auto const _previewPadding = 24.0f;
static auto const _controlsWidth = 400.0f;
static auto const _controlsPadding = 16.0f;
static auto const _controlsGap = 14.0f;

void Playground(PlaygroundProps const& props) {
    PROFILE
    COMPONENT

    auto const colorScheme = UseColorScheme();
    auto const& colors = _colors.at(colorScheme);

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    node.alignment = node.alignment.value_or(NodeAlignment::Stretch);
    node.borderWidth = node.borderWidth.value_or(_borderWidth);
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.border = node.border.value_or(ColorBrush{ colors.border });
    node.background = node.background.value_or(ColorBrush{ colors.background });
    node.overflowX = node.overflowX.value_or(NodeOverflow::Hidden);
    node.overflowY = node.overflowY.value_or(NodeOverflow::Hidden);

    Node(node, [&]{
        Node({
            .direction = NodeDirection::Horizontal,
            .alignment = NodeAlignment::Stretch
        }, [&]{
            auto preview = props.previewProps;
            preview.direction = preview.direction.value_or(NodeDirection::Vertical);
            preview.alignment = preview.alignment.value_or(NodeAlignment::Center);
            preview.justify = preview.justify.value_or(NodeJustify::Center);
            preview.minHeight = preview.minHeight.value_or(_previewMinHeight);
            preview.padding = preview.padding.value_or(_previewPadding);
            preview.flex = preview.flex.value_or(true);
            preview.borderTopLeftRadius = preview.borderTopLeftRadius.value_or(_innerBorderRadius);
            preview.borderBottomLeftRadius = preview.borderBottomLeftRadius.value_or(props.code.has_value() ? 0.0f : _innerBorderRadius);

            Node(preview, [&]{
                if (props.preview != nullptr) {
                    props.preview();
                }
            });

            Node({
                .direction = NodeDirection::Vertical,
                .alignment = NodeAlignment::Stretch,
                .width = _controlsWidth,
                .padding = _controlsPadding,
                .gapY = _controlsGap,
                .borderLeftWidth = _borderWidth,
                .borderTopRightRadius = _innerBorderRadius,
                .borderBottomRightRadius = (props.code.has_value() ? 0.0f : _innerBorderRadius),
                .background = ColorBrush{ colors.controls },
                .border = ColorBrush{ colors.border }
            }, [&]{
                if (props.controls != nullptr) {
                    Context((colorScheme == ColorScheme::Light ? GetDefaultLightTheme() : GetDefaultDarkTheme()), [&]{
                        props.controls();
                    });
                }
            });
        });

        if (props.code.has_value()) {
            CodeBlock({
                .content = props.code.value(),
                .nodeProps = {
                    .borderWidth = 0.0f,
                    .borderTopWidth = _borderWidth,
                    .borderRadius = 0.0f,
                    .borderBottomLeftRadius = _innerBorderRadius,
                    .borderBottomRightRadius = _innerBorderRadius,
                    .border = ColorBrush{ colors.border }
                }
            });
        }
    });
}

} /* namespace Storybook */
