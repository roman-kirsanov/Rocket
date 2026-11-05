/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/SegmentedControl.hpp>

using namespace Rocket;

namespace Storybook {

struct _SegmentedControlColors {
    Vec4 background;
    Vec4 selected;
    Vec4 selectedBorder;
    Vec4 text;
    Vec4 activeText;
};

struct _SegmentedControlItemProps {
    std::string key;
    std::string label;
    bool selected = false;
    std::function<void()> onClick;
};

struct _SegmentedControlItemState {
    std::function<void()> onClick;
};

struct _SegmentedControlState {
    std::function<void(std::size_t)> onChange;
};

auto constexpr _gray1 = Vec4{ 241.0f / 255.0f, 243.0f / 255.0f, 245.0f / 255.0f, 1.0f }; /* #f1f3f5 */
auto constexpr _gray3 = Vec4{ 222.0f / 255.0f, 226.0f / 255.0f, 230.0f / 255.0f, 1.0f }; /* #dee2e6 */
auto constexpr _gray7 = Vec4{ 73.0f / 255.0f, 80.0f / 255.0f, 87.0f / 255.0f, 1.0f };    /* #495057 */
auto constexpr _gray9 = Vec4{ 33.0f / 255.0f, 37.0f / 255.0f, 41.0f / 255.0f, 1.0f };    /* #212529 */
auto constexpr _dark0 = Vec4{ 201.0f / 255.0f, 201.0f / 255.0f, 201.0f / 255.0f, 1.0f }; /* #c9c9c9 */
auto constexpr _dark2 = Vec4{ 130.0f / 255.0f, 130.0f / 255.0f, 130.0f / 255.0f, 1.0f }; /* #828282 */
auto constexpr _dark4 = Vec4{ 66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.0f };    /* #424242 */
auto constexpr _dark5 = Vec4{ 59.0f / 255.0f, 59.0f / 255.0f, 59.0f / 255.0f, 1.0f };    /* #3b3b3b */
auto constexpr _dark8 = Vec4{ 31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f };    /* #1f1f1f */
auto constexpr _white = COLOR_WHITE;

static auto const _colors = std::map<ColorScheme, _SegmentedControlColors>{
    { ColorScheme::Light, { _gray1, _white, _gray3, _gray7, _gray9 } },
    { ColorScheme::Dark, { _dark8, _dark5, _dark4, _dark2, _dark0 } }
};

static auto const _padding = 3.0f;
static auto const _borderRadius = 6.0f;
static auto const _itemPaddingX = 4.0f;
static auto const _itemPaddingY = 4.0f;
static auto const _itemBorderWidth = 1.0f;
static auto const _itemBorderRadius = 4.0f;
static auto const _fontSize = 13.0f;

static void _SegmentedControlItem(_SegmentedControlItemProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SegmentedControlItemState>();
    auto& nodeRef = UseRef<class Node>();

    state.onClick = props.onClick;

    auto const& colors = _colors.at(UseColorScheme());
    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());

    Node({
        .ref = &nodeRef,
        .direction = NodeDirection::Horizontal,
        .alignment = NodeAlignment::Center,
        .justify = NodeJustify::Center,
        .paddingLeft = _itemPaddingX,
        .paddingTop = _itemPaddingY,
        .paddingRight = _itemPaddingX,
        .paddingBottom = _itemPaddingY,
        .borderWidth = _itemBorderWidth,
        .borderRadius = _itemBorderRadius,
        .flex = true,
        .cursor = Cursor::Pointer,
        .fontWeight = FontWeight::Medium,
        .textColor = ((props.selected || isHover) ? colors.activeText : colors.text),
        .background = ColorBrush{ props.selected ? colors.selected : colors.background },
        .border = ColorBrush{ props.selected ? colors.selectedBorder : colors.background },
        .onEvent = [&](NodeEvent const& event) {
            if (event.is<MouseClickNodeEvent>()) {
                if (state.onClick != nullptr) {
                    state.onClick();
                }

                event.stopPropagation();
            }
        }
    }, [&]{
        Node({
            .display = NodeDisplay::Text,
            .content = props.label
        });
    });
}

void SegmentedControl(SegmentedControlProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SegmentedControlState>();

    state.onChange = props.onChange;

    auto const& colors = _colors.at(UseColorScheme());

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Stretch);
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(_padding));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(_padding));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(_padding));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(_padding));
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.background = node.background.value_or(ColorBrush{ colors.background });
    node.fontSize = node.fontSize.value_or(_fontSize);

    Node(node, [&]{
        auto const& options = props.options.value_or(std::vector<std::string>());

        for (auto index = 0llu; index < options.size(); index++) {
            _SegmentedControlItem({
                .key = options[index],
                .label = options[index],
                .selected = (props.selected == index),
                .onClick = [&, index]{
                    if (state.onChange != nullptr) {
                        state.onChange(index);
                    }
                }
            });
        }
    });
}

} /* namespace Storybook */
