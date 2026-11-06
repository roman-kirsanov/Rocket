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
static auto constexpr _infoIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\">"
    "<path fill=\"#000\" d=\"M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256s256-114.6 256-256S397.4 0 256 0zM232 152C232 138.8 242.8 128 256 128s24 10.75 24 24v128c0 13.25-10.75 24-24 24S232 293.3 232 280V152zM256 400c-17.36 0-31.44-14.08-31.44-31.44c0-17.36 14.07-31.44 31.44-31.44s31.44 14.08 31.44 31.44C287.4 385.9 273.4 400 256 400z\"/>"
    "</svg>"
);
static auto constexpr _successIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\">"
    "<path fill=\"#000\" d=\"M0 256C0 114.6 114.6 0 256 0C397.4 0 512 114.6 512 256C512 397.4 397.4 512 256 512C114.6 512 0 397.4 0 256zM371.8 211.8C382.7 200.9 382.7 183.1 371.8 172.2C360.9 161.3 343.1 161.3 332.2 172.2L224 280.4L179.8 236.2C168.9 225.3 151.1 225.3 140.2 236.2C129.3 247.1 129.3 264.9 140.2 275.8L204.2 339.8C215.1 350.7 232.9 350.7 243.8 339.8L371.8 211.8z\"/>"
    "</svg>"
);
static auto constexpr _warningIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\">"
    "<path fill=\"#000\" d=\"M506.3 417l-213.3-364c-16.33-28-57.54-28-73.98 0l-213.2 364C-10.59 444.9 9.849 480 42.74 480h426.6C502.1 480 522.6 445 506.3 417zM232 168c0-13.25 10.75-24 24-24S280 154.8 280 168v128c0 13.25-10.75 24-23.1 24S232 309.3 232 296V168zM256 416c-17.36 0-31.44-14.08-31.44-31.44c0-17.36 14.07-31.44 31.44-31.44s31.44 14.08 31.44 31.44C287.4 401.9 273.4 416 256 416z\"/>"
    "</svg>"
);
static auto constexpr _dangerIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\">"
    "<path fill=\"#000\" d=\"M0 256C0 114.6 114.6 0 256 0C397.4 0 512 114.6 512 256C512 397.4 397.4 512 256 512C114.6 512 0 397.4 0 256zM175 208.1L222.1 255.1L175 303C165.7 312.4 165.7 327.6 175 336.1C184.4 346.3 199.6 346.3 208.1 336.1L255.1 289.9L303 336.1C312.4 346.3 327.6 346.3 336.1 336.1C346.3 327.6 346.3 312.4 336.1 303L289.9 255.1L336.1 208.1C346.3 199.6 346.3 184.4 336.1 175C327.6 165.7 312.4 165.7 303 175L255.1 222.1L208.1 175C199.6 165.7 184.4 165.7 175 175C165.7 184.4 165.7 199.6 175 208.1V208.1z\"/>"
    "</svg>"
);
static auto constexpr _spinnerIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\">"
    "<path fill=\"#000\" d=\"M224 32c0-17.7 14.3-32 32-32C397.4 0 512 114.6 512 256c0 46.6-12.5 90.4-34.3 128c-8.8 15.3-28.4 20.5-43.7 11.7s-20.5-28.4-11.7-43.7c16.3-28.2 25.7-61 25.7-96c0-106-86-192-192-192c-17.7 0-32-14.3-32-32z\"/>"
    "</svg>"
);
static auto constexpr _questionIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"64 64 896 896\">"
    "<path fill=\"#000\" d=\"M512 64C264.6 64 64 264.6 64 512s200.6 448 448 448 448-200.6 448-448S759.4 64 512 64zm0 820c-205.4 0-372-166.6-372-372s166.6-372 372-372 372 166.6 372 372-166.6 372-372 372z\"/>"
    "<path fill=\"#000\" d=\"M464 336a48 48 0 1096 0 48 48 0 10-96 0zm72 112h-48c-4.4 0-8 3.6-8 8v272c0 4.4 3.6 8 8 8h48c4.4 0 8-3.6 8-8V456c0-4.4-3.6-8-8-8z\"/>"
    "</svg>"
);
static auto constexpr _caretIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 384 512\">"
    "<path fill=\"#000\" d=\"M360.5 217.5l-152 143.1C203.9 365.8 197.9 368 192 368s-11.88-2.188-16.5-6.562L23.5 217.5C13.87 208.3 13.47 193.1 22.56 183.5C31.69 173.8 46.94 173.5 56.5 182.6L192 310.9l135.5-128.4c9.562-9.094 24.75-8.75 33.94 .9375C370.5 193.1 370.1 208.3 360.5 217.5z\"/>"
    "</svg>"
);
static auto constexpr _searchIcon = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\">"
    "<path fill=\"#000\" d=\"M504.1 471l-134-134C399.1 301.5 415.1 256.8 415.1 208c0-114.9-93.13-208-208-208S-.0002 93.13-.0002 208S93.12 416 207.1 416c48.79 0 93.55-16.91 129-45.04l134 134C475.7 509.7 481.9 512 488 512s12.28-2.344 16.97-7.031C514.3 495.6 514.3 480.4 504.1 471zM48 208c0-88.22 71.78-160 160-160s160 71.78 160 160s-71.78 160-160 160S48 296.2 48 208z\"/>"
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
        } },
        .callout = { .rules = {
            { .style = { .materialKind = MaterialKind::FlatOutlined, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .materialRoundness = MaterialRoundness::Small, .color = palette.neutral200, .fontSize = 13.0f, .padding = Vec4{ 10.0f, 10.0f, 10.0f, 10.0f }, .gap = 8.0f, .icon = SvgIconData{ _infoIcon }, .iconSize = Vec2{ 16.0f, 16.0f } } },
            { .selector = { .intent = CalloutIntent::Primary }, .style = { .materialIntent = MaterialIntent::Primary, .color = palette.blue300 } },
            { .selector = { .intent = CalloutIntent::Success }, .style = { .materialIntent = MaterialIntent::Success, .color = palette.green300, .icon = SvgIconData{ _successIcon } } },
            { .selector = { .intent = CalloutIntent::Warning }, .style = { .materialIntent = MaterialIntent::Warning, .color = palette.yellow300, .icon = SvgIconData{ _warningIcon } } },
            { .selector = { .intent = CalloutIntent::Danger }, .style = { .materialIntent = MaterialIntent::Danger, .color = palette.red300, .icon = SvgIconData{ _dangerIcon } } }
        } },
        .spinner = { .rules = {
            { .style = { .icon = SvgIconData{ _spinnerIcon }, .color = palette.blue500, .size = Vec2{ 16.0f, 16.0f } } },
            { .selector = { .size = SpinnerSize::Small }, .style = { .size = Vec2{ 12.0f, 12.0f } } },
            { .selector = { .size = SpinnerSize::Large }, .style = { .size = Vec2{ 24.0f, 24.0f } } }
        } },
        .modal = { .rules = {
            { .style = { .backdrop = _WithAlpha(palette.black, 0.6f), .materialKind = MaterialKind::Raised, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .materialRoundness = MaterialRoundness::Large, .color = palette.neutral200, .fontSize = 13.0f } }
        } },
        .alert = { .rules = {
            { .style = { .padding = Vec4{ 14.0f, 14.0f, 14.0f, 14.0f }, .gap = 14.0f, .contentGap = 12.0f, .textGap = 8.0f, .buttonsGap = 6.0f, .maxWidth = 400.0f, .titleFontSize = 14.0f, .titleFontWeight = FontWeight::Bold, .icon = SvgIconData{ _infoIcon }, .iconColor = palette.neutral200, .iconSize = Vec2{ 20.0f, 20.0f } } },
            { .selector = { .intent = AlertIntent::Primary }, .style = { .iconColor = palette.blue500 } },
            { .selector = { .intent = AlertIntent::Success }, .style = { .icon = SvgIconData{ _successIcon }, .iconColor = palette.green500 } },
            { .selector = { .intent = AlertIntent::Warning }, .style = { .icon = SvgIconData{ _warningIcon }, .iconColor = palette.yellow500 } },
            { .selector = { .intent = AlertIntent::Danger }, .style = { .icon = SvgIconData{ _dangerIcon }, .iconColor = palette.red500 } }
        } },
        .drawer = { .rules = {
            { .style = { .backdrop = _WithAlpha(palette.black, 0.6f), .materialKind = MaterialKind::Raised, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .color = palette.neutral200, .fontSize = 13.0f } }
        } },
        .tooltip = { .rules = {
            { .style = { .materialKind = MaterialKind::Raised, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .materialRoundness = MaterialRoundness::Small, .color = palette.neutral200, .fontSize = 12.0f, .padding = Vec4{ 8.0f, 5.0f, 8.0f, 5.0f }, .gap = 2.0f } },
            { .selector = { .intent = TooltipIntent::Primary }, .style = { .materialIntent = MaterialIntent::Primary } },
            { .selector = { .intent = TooltipIntent::Success }, .style = { .materialIntent = MaterialIntent::Success } },
            { .selector = { .intent = TooltipIntent::Warning }, .style = { .materialIntent = MaterialIntent::Warning } },
            { .selector = { .intent = TooltipIntent::Danger }, .style = { .materialIntent = MaterialIntent::Danger } }
        } },
        .label = { .rules = {
            { .style = { .color = palette.neutral200, .fontSize = 12.0f, .fontWeight = FontWeight::Medium, .minHeight = 14.0f, .requiredMark = "*", .requiredColor = palette.red500, .requiredFontSize = 14.0f, .requiredGap = 3.0f, .icon = SvgIconData{ _questionIcon }, .iconColor = palette.neutral200, .iconSize = Vec2{ 14.0f, 14.0f }, .iconOpacity = 0.5f, .iconGap = 6.0f } },
            { .selector = { .state = LabelTheme::State::Hover }, .style = { .iconOpacity = 1.0f } }
        } },
        .input = { .rules = {
            { .style = { .materialKind = MaterialKind::Lowered, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .materialRoundness = MaterialRoundness::Small, .color = palette.neutral100, .placeholderColor = palette.neutral500, .fontSize = 13.0f, .padding = Vec4{ 8.0f, 0.0f, 8.0f, 0.0f }, .minHeight = 28.0f, .gap = 0.0f } },
            { .selector = { .size = InputSize::Small }, .style = { .fontSize = 11.0f, .padding = Vec4{ 6.0f, 0.0f, 6.0f, 0.0f }, .minHeight = 20.0f } },
            { .selector = { .size = InputSize::Large }, .style = { .fontSize = 15.0f, .padding = Vec4{ 10.0f, 0.0f, 10.0f, 0.0f }, .minHeight = 40.0f } },
            { .selector = { .state = InputTheme::State::Hover }, .style = { .materialBrightness = MaterialBrightness::Lighter } },
            { .selector = { .state = InputTheme::State::Focus }, .style = { .materialKind = MaterialKind::Outlined, .materialIntent = MaterialIntent::Primary } },
            { .selector = { .state = InputTheme::State::Disabled }, .style = { .opacity = 0.75f } },
            { .selector = { .intent = InputIntent::Primary }, .style = { .materialIntent = MaterialIntent::Primary } },
            { .selector = { .intent = InputIntent::Success }, .style = { .materialIntent = MaterialIntent::Success } },
            { .selector = { .intent = InputIntent::Warning }, .style = { .materialIntent = MaterialIntent::Warning } },
            { .selector = { .intent = InputIntent::Danger }, .style = { .materialIntent = MaterialIntent::Danger } }
        } },
        .select = { .rules = {
            { .style = { .materialKind = MaterialKind::Lowered, .materialIntent = MaterialIntent::Default, .materialBrightness = MaterialBrightness::Normal, .materialRoundness = MaterialRoundness::Small, .color = palette.neutral100, .placeholderColor = palette.neutral500, .fontSize = 13.0f, .padding = Vec4{ 8.0f, 0.0f, 8.0f, 0.0f }, .minHeight = 28.0f, .gap = 6.0f, .caret = SvgIconData{ _caretIcon }, .searchIcon = SvgIconData{ _searchIcon }, .caretColor = palette.neutral400, .caretSize = Vec2{ 12.0f, 12.0f }, .listMaterialKind = MaterialKind::Raised, .listMaterialIntent = MaterialIntent::Default, .listMaterialRoundness = MaterialRoundness::Small, .listPadding = Vec4{ 6.0f, 6.0f, 6.0f, 6.0f }, .listGap = 2.0f, .listMaxHeight = 240.0f, .optionPadding = Vec4{ 8.0f, 5.0f, 8.0f, 5.0f }, .optionColor = palette.neutral200, .optionHighlightBackground = _WithAlpha(palette.white, 0.1f), .optionSelectedBackground = _WithAlpha(palette.blue500, 0.35f), .optionSelectedColor = palette.white } },
            { .selector = { .size = SelectSize::Small }, .style = { .fontSize = 11.0f, .padding = Vec4{ 6.0f, 0.0f, 6.0f, 0.0f }, .minHeight = 20.0f, .caretSize = Vec2{ 10.0f, 10.0f } } },
            { .selector = { .size = SelectSize::Large }, .style = { .fontSize = 15.0f, .padding = Vec4{ 10.0f, 0.0f, 10.0f, 0.0f }, .minHeight = 40.0f, .caretSize = Vec2{ 14.0f, 14.0f } } },
            { .selector = { .state = SelectTheme::State::Hover }, .style = { .materialBrightness = MaterialBrightness::Lighter } },
            { .selector = { .state = SelectTheme::State::Focus }, .style = { .materialKind = MaterialKind::Outlined, .materialIntent = MaterialIntent::Primary } },
            { .selector = { .state = SelectTheme::State::Disabled }, .style = { .opacity = 0.75f } },
            { .selector = { .intent = SelectIntent::Primary }, .style = { .materialIntent = MaterialIntent::Primary } },
            { .selector = { .intent = SelectIntent::Success }, .style = { .materialIntent = MaterialIntent::Success } },
            { .selector = { .intent = SelectIntent::Warning }, .style = { .materialIntent = MaterialIntent::Warning } },
            { .selector = { .intent = SelectIntent::Danger }, .style = { .materialIntent = MaterialIntent::Danger } }
        } }
    };
}

Theme const& GetPixelDarkTheme() {
    PROFILE

    static auto const theme = _CreatePixelDarkTheme(GetDefaultPalette());

    return theme;
}

} /* namespace Rocket */
