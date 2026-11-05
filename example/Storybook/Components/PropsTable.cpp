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
#include <Components/PropsTable.hpp>
#include <Components/Text.hpp>

using namespace Rocket;

namespace Storybook {

struct _PropsTableColors {
    Vec4 headerBorder;
    Vec4 rowBorder;
    Vec4 secondary;
};

auto constexpr _gray2 = Vec4{ 233.0f / 255.0f, 236.0f / 255.0f, 239.0f / 255.0f, 1.0f }; /* #e9ecef */
auto constexpr _gray3 = Vec4{ 222.0f / 255.0f, 226.0f / 255.0f, 230.0f / 255.0f, 1.0f }; /* #dee2e6 */
auto constexpr _gray7 = Vec4{ 73.0f / 255.0f, 80.0f / 255.0f, 87.0f / 255.0f, 1.0f }; /* #495057 */
auto constexpr _dark1 = Vec4{ 184.0f / 255.0f, 184.0f / 255.0f, 184.0f / 255.0f, 1.0f }; /* #b8b8b8 */
auto constexpr _dark4 = Vec4{ 66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.0f }; /* #424242 */
auto constexpr _dark5 = Vec4{ 59.0f / 255.0f, 59.0f / 255.0f, 59.0f / 255.0f, 1.0f }; /* #3b3b3b */

static auto const _colors = std::map<ColorScheme, _PropsTableColors>{
    { ColorScheme::Light, { _gray3, _gray2, _gray7 } },
    { ColorScheme::Dark, { _dark4, _dark5, _dark1 } }
};

static auto const _fontFamily = std::string("Mono");
static auto const _nameWidth = 200.0f;
static auto const _typeWidth = 240.0f;
static auto const _defaultWidth = 100.0f;
static auto const _cellGap = 16.0f;
static auto const _rowPaddingY = 10.0f;
static auto const _borderWidth = 1.0f;
static auto const _headerFontSize = 13.0f;
static auto const _typeFontSize = 12.0f;

void PropsTable(PropsTableProps const& props) {
    PROFILE
    COMPONENT

    auto const& colors = _colors.at(UseColorScheme());

    auto node = props.nodeProps;
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    node.alignment = node.alignment.value_or(NodeAlignment::Stretch);

    Node(node, [&]{
        Node({
            .direction = NodeDirection::Horizontal,
            .paddingBottom = _rowPaddingY,
            .gapX = _cellGap,
            .borderBottomWidth = _borderWidth,
            .fontWeight = FontWeight::SemiBold,
            .fontSize = _headerFontSize,
            .textColor = colors.secondary,
            .border = ColorBrush{ colors.headerBorder }
        }, [&]{
            Node({ .display = NodeDisplay::Text, .width = _nameWidth, .content = "Name" });
            Node({ .display = NodeDisplay::Text, .width = _typeWidth, .content = "Type" });
            Node({ .display = NodeDisplay::Text, .width = _defaultWidth, .content = "Default" });
            Node({ .display = NodeDisplay::Text, .flex = true, .content = "Description" });
        });

        for (auto const& row : props.rows.value_or(std::vector<PropsTableRow>())) {
            Node({
                .key = row.name,
                .direction = NodeDirection::Horizontal,
                .alignment = NodeAlignment::Start,
                .paddingTop = _rowPaddingY,
                .paddingBottom = _rowPaddingY,
                .gapX = _cellGap,
                .borderBottomWidth = _borderWidth,
                .border = ColorBrush{ colors.rowBorder }
            }, [&]{
                Node({ .width = _nameWidth }, [&]{
                    Code({ .content = row.name });
                });

                Node({
                    .display = NodeDisplay::Text,
                    .width = _typeWidth,
                    .fontFamily = _fontFamily,
                    .fontSize = _typeFontSize,
                    .textColor = colors.secondary,
                    .content = row.type
                });

                Node({
                    .display = NodeDisplay::Text,
                    .width = _defaultWidth,
                    .fontFamily = _fontFamily,
                    .fontSize = _typeFontSize,
                    .textColor = colors.secondary,
                    .content = (row.defaultValue.empty() ? "-" : row.defaultValue)
                });

                Text({
                    .content = row.description,
                    .size = TextSize::Small,
                    .nodeProps = { .flex = true }
                });
            });
        }
    });
}

} /* namespace Storybook */
