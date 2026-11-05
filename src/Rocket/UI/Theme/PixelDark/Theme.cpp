/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string_view>
#include <Rocket/UI/Theme/Default/Palette.hpp>
#include <Rocket/UI/Theme/PixelDark/Material.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>

namespace Rocket {

auto constexpr _blueOutlinedActive = Vec4{ 10.0f / 255.0f, 32.0f / 255.0f, 118.0f / 255.0f, 1.0f };
auto constexpr _greenOutlinedActive = Vec4{ 10.0f / 255.0f, 63.0f / 255.0f, 25.0f / 255.0f, 1.0f };
auto constexpr _yellowOutlinedActive = Vec4{ 93.0f / 255.0f, 43.0f / 255.0f, 0.0f, 1.0f };
auto constexpr _redOutlinedActive = Vec4{ 107.0f / 255.0f, 9.0f / 255.0f, 9.0f / 255.0f, 1.0f };

static auto constexpr _checkMark = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\">"
    "<path d=\"M4.5 12.5l5 5L19.5 7\" fill=\"none\" stroke=\"#000\" stroke-width=\"4\" stroke-linecap=\"square\"/>"
    "</svg>"
);

static Vec4 _WithAlpha(Vec4 color, float alpha) {
    color.alpha = alpha;

    return color;
}

static Theme _CreatePixelDarkTheme(ColorPalette const& palette) {
    return Theme{
        .material = &GetPixelDarkMaterial(),
        .button = { .rules = {
            { .style = { .materialKind = MaterialKind::Raised, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .materialRoundness = MaterialRoundness::Large, .color = palette.neutral200, .padding = Vec4{ 10.0f, 6.0f, 10.0f, 6.0f }, .gap = Vec2{ 6.0f, 6.0f }, .minHeight = 28.0f, .fontSize = 13.0f, .iconSize = Vec2{ 16.0f, 16.0f } } },
            { .selector = { .state = ButtonTheme::State::Hover }, .style = { .materialBrightness = MaterialBrightness::Lighter } },
            { .selector = { .state = ButtonTheme::State::Active }, .style = { .materialKind = MaterialKind::Default, .materialBrightness = MaterialBrightness::Darker } },
            { .selector = { .state = ButtonTheme::State::Disabled }, .style = { .opacity = 0.75f } },
            { .selector = { .size = ButtonSize::Small }, .style = { .padding = Vec4{ 8.0f, 4.0f, 8.0f, 4.0f }, .gap = Vec2{ 4.0f, 4.0f }, .minHeight = 20.0f, .fontSize = 11.0f, .iconSize = Vec2{ 12.0f, 12.0f } } },
            { .selector = { .size = ButtonSize::Large }, .style = { .padding = Vec4{ 12.0f, 8.0f, 12.0f, 8.0f }, .gap = Vec2{ 8.0f, 8.0f }, .minHeight = 40.0f, .fontSize = 15.0f, .iconSize = Vec2{ 24.0f, 24.0f } } },
            { .selector = { .narrow = true }, .style = { .padding = Vec4{ 6.0f, 6.0f, 6.0f, 6.0f } } },
            { .selector = { .size = ButtonSize::Small, .narrow = true }, .style = { .padding = Vec4{ 4.0f, 4.0f, 4.0f, 4.0f } } },
            { .selector = { .size = ButtonSize::Large, .narrow = true }, .style = { .padding = Vec4{ 8.0f, 8.0f, 8.0f, 8.0f } } },
            { .selector = { .intent = ButtonIntent::Primary }, .style = { .materialIntent = MaterialIntent::Primary } },
            { .selector = { .intent = ButtonIntent::Success }, .style = { .materialIntent = MaterialIntent::Success } },
            { .selector = { .intent = ButtonIntent::Warning }, .style = { .materialIntent = MaterialIntent::Warning } },
            { .selector = { .intent = ButtonIntent::Danger }, .style = { .materialIntent = MaterialIntent::Danger } },
            { .selector = { .kind = ButtonKind::Flat }, .style = { .materialKind = MaterialKind::Default } },
            { .selector = { .kind = ButtonKind::Outlined }, .style = { .materialKind = MaterialKind::Outlined, .color = palette.neutral350 } },
            { .selector = { .kind = ButtonKind::Outlined, .state = ButtonTheme::State::Hover }, .style = { .color = palette.neutral200 } },
            { .selector = { .kind = ButtonKind::Outlined, .state = ButtonTheme::State::Active }, .style = { .materialKind = MaterialKind::Outlined, .color = palette.neutral450 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Primary }, .style = { .color = palette.blue750 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Hover }, .style = { .color = palette.blue700 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Active }, .style = { .color = _blueOutlinedActive } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Success }, .style = { .color = palette.green750 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Hover }, .style = { .color = palette.green700 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Active }, .style = { .color = _greenOutlinedActive } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Warning }, .style = { .color = palette.yellow750 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Hover }, .style = { .color = palette.yellow700 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Active }, .style = { .color = _yellowOutlinedActive } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger }, .style = { .color = palette.red750 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Hover }, .style = { .color = palette.red700 } },
            { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Active }, .style = { .color = _redOutlinedActive } },
            { .selector = { .kind = ButtonKind::Transparent }, .style = { .materialKind = MaterialKind::Transparent, .color = palette.neutral350 } },
            { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Primary }, .style = { .color = palette.blue750 } },
            { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Success }, .style = { .color = palette.green750 } },
            { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Warning }, .style = { .color = palette.yellow750 } },
            { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Danger }, .style = { .color = palette.red750 } },
            { .selector = { .kind = ButtonKind::Transparent, .state = ButtonTheme::State::Hover }, .style = { .color = palette.neutral200 } },
            { .selector = { .kind = ButtonKind::Transparent, .state = ButtonTheme::State::Active }, .style = { .materialKind = MaterialKind::Transparent, .color = palette.neutral200 } }
        } },
        .icon = { .rules = {
            { .style = { .color = palette.neutral200, .size = Vec2{ 16.0f, 16.0f } } },
            { .selector = { .intent = IconIntent::Secondary }, .style = { .color = _WithAlpha(palette.neutral200, 0.5f) } },
            { .selector = { .intent = IconIntent::Primary }, .style = { .color = palette.blue500 } },
            { .selector = { .intent = IconIntent::Success }, .style = { .color = palette.green500 } },
            { .selector = { .intent = IconIntent::Warning }, .style = { .color = palette.yellow500 } },
            { .selector = { .intent = IconIntent::Danger }, .style = { .color = palette.red500 } },
            { .selector = { .size = IconSize::Small }, .style = { .size = Vec2{ 12.0f, 12.0f } } },
            { .selector = { .size = IconSize::Large }, .style = { .size = Vec2{ 24.0f, 24.0f } } },
            { .selector = { .disabled = true }, .style = { .alpha = 0.5f } }
        } },
        .checkbox = { .rules = {
            { .style = { .materialKind = MaterialKind::Lowered, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .materialRoundness = MaterialRoundness::Small, .boxSize = 16.0f, .markColor = palette.neutral100, .markSize = Vec2{ 10.0f, 10.0f }, .color = palette.neutral200, .fontSize = 13.0f, .gap = 6.0f } },
            { .selector = { .state = CheckboxTheme::State::Hover }, .style = { .materialBrightness = MaterialBrightness::Lighter } },
            { .selector = { .state = CheckboxTheme::State::Active }, .style = { .materialBrightness = MaterialBrightness::Darker } },
            { .selector = { .state = CheckboxTheme::State::Disabled }, .style = { .opacity = 0.75f } },
            { .selector = { .size = CheckboxSize::Small }, .style = { .boxSize = 12.0f, .markSize = Vec2{ 8.0f, 8.0f }, .fontSize = 11.0f, .gap = 4.0f } },
            { .selector = { .size = CheckboxSize::Large }, .style = { .boxSize = 20.0f, .markSize = Vec2{ 14.0f, 14.0f }, .fontSize = 15.0f, .gap = 8.0f } },
            { .selector = { .checked = true }, .style = { .materialIntent = MaterialIntent::Primary, .mark = SvgIconData{ _checkMark } } },
            { .selector = { .intent = CheckboxIntent::Success, .checked = true }, .style = { .materialIntent = MaterialIntent::Success } },
            { .selector = { .intent = CheckboxIntent::Warning, .checked = true }, .style = { .materialIntent = MaterialIntent::Warning } },
            { .selector = { .intent = CheckboxIntent::Danger, .checked = true }, .style = { .materialIntent = MaterialIntent::Danger } }
        } },
        .radiobox = { .rules = {
            { .style = { .materialKind = MaterialKind::Lowered, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .boxSize = 16.0f, .dotMaterialKind = MaterialKind::Flat, .dotMaterialIntent = MaterialIntent::Primary, .dotMaterialBrightness = MaterialBrightness::Lighter, .color = palette.neutral200, .fontSize = 13.0f, .gap = 6.0f } },
            { .selector = { .state = RadioboxTheme::State::Hover }, .style = { .materialBrightness = MaterialBrightness::Lighter } },
            { .selector = { .state = RadioboxTheme::State::Active }, .style = { .materialBrightness = MaterialBrightness::Darker } },
            { .selector = { .state = RadioboxTheme::State::Disabled }, .style = { .opacity = 0.75f } },
            { .selector = { .size = RadioboxSize::Small }, .style = { .boxSize = 12.0f, .fontSize = 11.0f, .gap = 4.0f } },
            { .selector = { .size = RadioboxSize::Large }, .style = { .boxSize = 20.0f, .fontSize = 15.0f, .gap = 8.0f } },
            { .selector = { .checked = true }, .style = { .dotSize = 8.0f } },
            { .selector = { .size = RadioboxSize::Small, .checked = true }, .style = { .dotSize = 4.0f } },
            { .selector = { .size = RadioboxSize::Large, .checked = true }, .style = { .dotSize = 12.0f } },
            { .selector = { .intent = RadioboxIntent::Success }, .style = { .dotMaterialIntent = MaterialIntent::Success } },
            { .selector = { .intent = RadioboxIntent::Warning }, .style = { .dotMaterialIntent = MaterialIntent::Warning } },
            { .selector = { .intent = RadioboxIntent::Danger }, .style = { .dotMaterialIntent = MaterialIntent::Danger } }
        } },
        .switch_ = { .rules = {
            { .style = { .materialKind = MaterialKind::Lowered, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .trackSize = Vec2{ 32.0f, 16.0f }, .knobMaterialKind = MaterialKind::Raised, .knobMaterialIntent = MaterialIntent::Default, .knobMaterialBrightness = MaterialBrightness::Normal, .knobSize = 16.0f, .color = palette.neutral200, .fontSize = 13.0f, .gap = 6.0f } },
            { .selector = { .state = SwitchTheme::State::Hover }, .style = { .knobMaterialBrightness = MaterialBrightness::Lighter } },
            { .selector = { .state = SwitchTheme::State::Active }, .style = { .knobMaterialBrightness = MaterialBrightness::Darker } },
            { .selector = { .state = SwitchTheme::State::Disabled }, .style = { .opacity = 0.75f } },
            { .selector = { .size = SwitchSize::Small }, .style = { .trackSize = Vec2{ 24.0f, 12.0f }, .knobSize = 12.0f, .fontSize = 11.0f, .gap = 4.0f } },
            { .selector = { .size = SwitchSize::Large }, .style = { .trackSize = Vec2{ 40.0f, 20.0f }, .knobSize = 20.0f, .fontSize = 15.0f, .gap = 8.0f } },
            { .selector = { .checked = true }, .style = { .materialIntent = MaterialIntent::Primary } },
            { .selector = { .intent = SwitchIntent::Success, .checked = true }, .style = { .materialIntent = MaterialIntent::Success } },
            { .selector = { .intent = SwitchIntent::Warning, .checked = true }, .style = { .materialIntent = MaterialIntent::Warning } },
            { .selector = { .intent = SwitchIntent::Danger, .checked = true }, .style = { .materialIntent = MaterialIntent::Danger } }
        } }
    };
}

Theme const& GetPixelDarkTheme() {
    PROFILE

    static auto const theme = _CreatePixelDarkTheme(GetDefaultPalette());

    return theme;
}

} /* namespace Rocket */
