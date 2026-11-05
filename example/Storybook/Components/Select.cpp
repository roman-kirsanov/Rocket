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
#include <Rocket/UI/Popup.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/Select.hpp>

using namespace Rocket;

namespace Storybook {

struct _SelectColors {
    Vec4 background;
    Vec4 border;
    Vec4 borderHover;
    Vec4 text;
    Vec4 chevron;
    Vec4 popup;
    Vec4 popupBorder;
    Vec4 optionHover;
    Vec4 optionSelected;
    Vec4 optionSelectedText;
};

struct _SelectOptionProps {
    std::string key;
    std::string label;
    bool selected = false;
    std::function<void()> onClick;
};

struct _SelectOptionState {
    std::function<void()> onClick;
};

struct _SelectState {
    bool open = false;
    std::unique_ptr<Image> chevron;
    std::function<void(std::size_t)> onChange;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    Effect<float, ColorScheme> shouldUpdateChevron;
};

auto constexpr _gray1 = Vec4{ 241.0f / 255.0f, 243.0f / 255.0f, 245.0f / 255.0f, 1.0f }; /* #f1f3f5 */
auto constexpr _gray3 = Vec4{ 222.0f / 255.0f, 226.0f / 255.0f, 230.0f / 255.0f, 1.0f }; /* #dee2e6 */
auto constexpr _gray4 = Vec4{ 206.0f / 255.0f, 212.0f / 255.0f, 218.0f / 255.0f, 1.0f }; /* #ced4da */
auto constexpr _gray6 = Vec4{ 134.0f / 255.0f, 142.0f / 255.0f, 150.0f / 255.0f, 1.0f }; /* #868e96 */
auto constexpr _gray9 = Vec4{ 33.0f / 255.0f, 37.0f / 255.0f, 41.0f / 255.0f, 1.0f };    /* #212529 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */
auto constexpr _dark2 = Vec4{ 130.0f / 255.0f, 130.0f / 255.0f, 130.0f / 255.0f, 1.0f }; /* #828282 */
auto constexpr _dark3 = Vec4{ 105.0f / 255.0f, 105.0f / 255.0f, 105.0f / 255.0f, 1.0f }; /* #696969 */
auto constexpr _dark4 = Vec4{ 66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.0f };    /* #424242 */
auto constexpr _dark5 = Vec4{ 59.0f / 255.0f, 59.0f / 255.0f, 59.0f / 255.0f, 1.0f };    /* #3b3b3b */
auto constexpr _dark6 = Vec4{ 46.0f / 255.0f, 46.0f / 255.0f, 46.0f / 255.0f, 1.0f };    /* #2e2e2e */
auto constexpr _dark7 = Vec4{ 36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f };    /* #242424 */
auto constexpr _blue0 = Vec4{ 231.0f / 255.0f, 245.0f / 255.0f, 255.0f / 255.0f, 1.0f }; /* #e7f5ff */
auto constexpr _blue4 = Vec4{ 77.0f / 255.0f, 171.0f / 255.0f, 247.0f / 255.0f, 1.0f };  /* #4dabf7 */
auto constexpr _blue6 = Vec4{ 34.0f / 255.0f, 139.0f / 255.0f, 230.0f / 255.0f, 1.0f };  /* #228be6 */
auto constexpr _blue6Light = Vec4{ 34.0f / 255.0f, 139.0f / 255.0f, 230.0f / 255.0f, 0.15f }; /* #228be6 at 15% */
auto constexpr _blue7 = Vec4{ 28.0f / 255.0f, 126.0f / 255.0f, 214.0f / 255.0f, 1.0f };  /* #1c7ed6 */
auto constexpr _white = COLOR_WHITE;

static auto const _colors = std::map<ColorScheme, _SelectColors>{
    { ColorScheme::Light, { _white, _gray4, _gray6, _gray9, _gray6, _white, _gray3, _gray1, _blue0, _blue7 } },
    { ColorScheme::Dark, { _dark6, _dark4, _dark3, _dark0, _dark2, _dark7, _dark4, _dark5, _blue6Light, _blue4 } }
};

static auto const _chevronSvg = std::string(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"m6 9 6 6 6-6\"/></svg>"
);
static auto const _chevronSize = 16.0f;
static auto const _height = 34.0f;
static auto const _paddingLeft = 12.0f;
static auto const _paddingRight = 8.0f;
static auto const _borderWidth = 1.0f;
static auto const _borderRadius = 4.0f;
static auto const _fontSize = 14.0f;
static auto const _popupOffset = Vec2{ 0.0f, 4.0f };
static auto const _popupPadding = 4.0f;
static auto const _optionPaddingX = 10.0f;
static auto const _optionPaddingY = 7.0f;
static auto const _optionBorderRadius = 4.0f;

static void _SelectOption(_SelectOptionProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SelectOptionState>();
    auto& nodeRef = UseRef<class Node>();

    state.onClick = props.onClick;

    auto const& colors = _colors.at(UseColorScheme());
    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());
    auto node = NodeProps{
        .ref = &nodeRef,
        .direction = NodeDirection::Horizontal,
        .alignment = NodeAlignment::Center,
        .paddingLeft = _optionPaddingX,
        .paddingTop = _optionPaddingY,
        .paddingRight = _optionPaddingX,
        .paddingBottom = _optionPaddingY,
        .borderRadius = _optionBorderRadius,
        .cursor = Cursor::Pointer,
        .fontWeight = (props.selected ? FontWeight::Medium : FontWeight::Normal),
        .textColor = (props.selected ? colors.optionSelectedText : colors.text),
        .onEvent = [&](NodeEvent const& event) {
            if (event.is<MouseClickNodeEvent>()) {
                if (state.onClick != nullptr) {
                    state.onClick();
                }

                event.stopPropagation();
            }
        }
    };

    if (props.selected || isHover) {
        node.background = ColorBrush{ props.selected ? colors.optionSelected : colors.optionHover };
    }

    Node(node, [&]{
        Node({
            .display = NodeDisplay::Text,
            .content = props.label
        });
    });
}

void Select(SelectProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SelectState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);

    state.onChange = props.onChange;
    state.nodeOnEvent = props.nodeProps.onEvent;

    auto const colorScheme = UseColorScheme();
    auto const& colors = _colors.at(colorScheme);

    if (state.shouldUpdateChevron(document.getScale(), colorScheme)) {
        auto const size = Vec2{ (_chevronSize * document.getScale()), (_chevronSize * document.getScale()) };
        auto pixels = std::vector<std::uint8_t>();

        ConvertSVGToBitmap(_chevronSvg, size, colors.chevron, pixels);
        state.chevron = std::make_unique<Image>(size, pixels.data());
    }

    auto const& options = props.options.value_or(std::vector<std::string>());
    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());
    auto const border = (
        state.open ? _blue6 :
        isHover ? colors.borderHover :
                  colors.border
    );

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.justify = node.justify.value_or(NodeJustify::SpaceBetween);
    node.height = node.height.value_or(_height);
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(_paddingLeft));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(_paddingRight));
    node.borderWidth = node.borderWidth.value_or(_borderWidth);
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.cursor = node.cursor.value_or(Cursor::Pointer);
    node.fontSize = node.fontSize.value_or(_fontSize);
    node.textColor = node.textColor.value_or(colors.text);
    node.background = node.background.value_or(ColorBrush{ colors.background });
    node.border = node.border.value_or(ColorBrush{ border });
    node.onEvent = [&](NodeEvent const& event) {
        if (state.nodeOnEvent != nullptr) {
            state.nodeOnEvent(event);
        }

        if (event.is<MouseClickNodeEvent>()) {
            state.open = !state.open;
            event.stopPropagation();
        }
    };

    Node(node, [&]{
        Node({
            .display = NodeDisplay::Text,
            .content = ((props.selected.has_value() && props.selected.value() < options.size()) ? options[props.selected.value()] : "")
        });

        Node({
            .width = _chevronSize,
            .height = _chevronSize,
            .background = ImageBrush{
                .image = state.chevron.get(),
                .positionX = ImagePosition::Stretch,
                .positionY = ImagePosition::Stretch
            }
        });

        if (state.open) {
            Popup({
                .anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left },
                .collision = PopupCollision::Flip,
                .offset = _popupOffset,
                .onClose = [&]{
                    state.open = false;
                },
                .contentProps = {
                    .alignment = NodeAlignment::Stretch
                },
                .nodeProps = {
                    .alignment = NodeAlignment::Stretch,
                    .width = NodeValue{ PercentValue{ 100.0f } },
                    .padding = _popupPadding,
                    .background = ColorBrush{ colors.popup },
                    .border = ColorBrush{ colors.popupBorder }
                }
            }, [&]{
                for (auto index = 0llu; index < options.size(); index++) {
                    _SelectOption({
                        .key = options[index],
                        .label = options[index],
                        .selected = (props.selected == index),
                        .onClick = [&, index]{
                            state.open = false;

                            if (state.onChange != nullptr) {
                                state.onChange(index);
                            }
                        }
                    });
                }
            });
        }
    });
}

} /* namespace Storybook */
