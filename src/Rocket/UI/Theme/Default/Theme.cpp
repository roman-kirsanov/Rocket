/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <string_view>
#include <tuple>
#include <vector>
#include <Rocket/UI/Theme/Default/Palette.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>

namespace Rocket {

struct _DefaultFamily {
    Vec4 filled;
    Vec4 filledHover;
    Vec4 filledActive;
    Vec4 filledBorder;
    Vec4 filledContent;
    Vec4 accent;
    Vec4 flatContent;
};

struct _DefaultToggleAccent {
    Vec4 on;
    Vec4 onHover;
    Vec4 onActive;
};

struct _DefaultToggleColors {
    Vec4 off;
    Vec4 offHover;
    Vec4 offActive;
    Vec4 content;
    Vec4 label;
    _DefaultToggleAccent neutral;
    _DefaultToggleAccent primary;
    _DefaultToggleAccent success;
    _DefaultToggleAccent warning;
    _DefaultToggleAccent danger;
};

struct _DefaultAlphas {
    float hover;
    float active;
    float flat;
    float flatHover;
    float flatActive;
};

static auto const _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
static auto const _darkAlphas = _DefaultAlphas{ .hover = 0.1f, .active = 0.15f, .flat = 0.15f, .flatHover = 0.2f, .flatActive = 0.25f };
static auto constexpr _checkMark = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\">"
    "<path d=\"M5 12.5l4.5 4.5L19 7.5\" fill=\"none\" stroke=\"#000\" stroke-width=\"3.5\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "</svg>"
);

static Vec4 _WithAlpha(Vec4 color, float alpha) {
    color.alpha = alpha;

    return color;
}

static std::vector<ButtonTheme::Rule> _CreateDarkButtonRules(std::map<ButtonIntent, _DefaultFamily> const& families, _DefaultAlphas const& alphas) {
    auto rules = std::vector<ButtonTheme::Rule>{
        { .style = { .borderWidth = 1.0f, .borderRadius = 4.0f, .padding = Vec4{ 18.0f, 4.0f, 18.0f, 4.0f }, .gap = Vec2{ 8.0f, 8.0f }, .minHeight = 36.0f, .fontSize = 14.0f, .iconSize = Vec2{ 16.0f, 16.0f } } },
        { .selector = { .state = ButtonTheme::State::Disabled }, .style = { .opacity = 0.6f } },
        { .selector = { .size = ButtonSize::Small }, .style = { .padding = Vec4{ 14.0f, 4.0f, 14.0f, 4.0f }, .gap = Vec2{ 6.0f, 6.0f }, .minHeight = 30.0f, .fontSize = 12.0f, .iconSize = Vec2{ 14.0f, 14.0f } } },
        { .selector = { .size = ButtonSize::Large }, .style = { .padding = Vec4{ 22.0f, 4.0f, 22.0f, 4.0f }, .gap = Vec2{ 10.0f, 10.0f }, .minHeight = 42.0f, .fontSize = 16.0f, .iconSize = Vec2{ 20.0f, 20.0f } } },
        { .selector = { .narrow = true }, .style = { .padding = Vec4{ 9.0f, 4.0f, 9.0f, 4.0f } } },
        { .selector = { .size = ButtonSize::Small, .narrow = true }, .style = { .padding = Vec4{ 7.0f, 4.0f, 7.0f, 4.0f } } },
        { .selector = { .size = ButtonSize::Large, .narrow = true }, .style = { .padding = Vec4{ 10.0f, 4.0f, 10.0f, 4.0f } } }
    };

    for (auto const& [intent, family] : families) {
        rules.push_back({ .selector = { .kind = ButtonKind::Default, .intent = intent }, .style = { .background = family.filled, .borderColor = family.filledBorder, .color = family.filledContent } });
        rules.push_back({ .selector = { .kind = ButtonKind::Default, .intent = intent, .state = ButtonTheme::State::Hover }, .style = { .background = family.filledHover } });
        rules.push_back({ .selector = { .kind = ButtonKind::Default, .intent = intent, .state = ButtonTheme::State::Active }, .style = { .background = family.filledActive } });
        rules.push_back({ .selector = { .kind = ButtonKind::Outlined, .intent = intent }, .style = { .background = _transparent, .borderColor = family.accent, .color = family.accent } });
        rules.push_back({ .selector = { .kind = ButtonKind::Outlined, .intent = intent, .state = ButtonTheme::State::Hover }, .style = { .background = _WithAlpha(family.accent, alphas.hover) } });
        rules.push_back({ .selector = { .kind = ButtonKind::Outlined, .intent = intent, .state = ButtonTheme::State::Active }, .style = { .background = _WithAlpha(family.accent, alphas.active) } });
        rules.push_back({ .selector = { .kind = ButtonKind::Transparent, .intent = intent }, .style = { .background = _transparent, .borderColor = _transparent, .color = family.accent } });
        rules.push_back({ .selector = { .kind = ButtonKind::Transparent, .intent = intent, .state = ButtonTheme::State::Hover }, .style = { .background = _WithAlpha(family.accent, alphas.hover) } });
        rules.push_back({ .selector = { .kind = ButtonKind::Transparent, .intent = intent, .state = ButtonTheme::State::Active }, .style = { .background = _WithAlpha(family.accent, alphas.active) } });
        rules.push_back({ .selector = { .kind = ButtonKind::Flat, .intent = intent }, .style = { .background = _WithAlpha(family.accent, alphas.flat), .borderColor = _transparent, .color = family.flatContent } });
        rules.push_back({ .selector = { .kind = ButtonKind::Flat, .intent = intent, .state = ButtonTheme::State::Hover }, .style = { .background = _WithAlpha(family.accent, alphas.flatHover) } });
        rules.push_back({ .selector = { .kind = ButtonKind::Flat, .intent = intent, .state = ButtonTheme::State::Active }, .style = { .background = _WithAlpha(family.accent, alphas.flatActive) } });
    }

    return rules;
}

static std::vector<ButtonTheme::Rule> _CreateLightButtonRules(ColorPalette const& palette) {
    return {
        { .style = { .background = palette.gray150, .borderColor = palette.gray200, .borderWidth = 1.0f, .borderRadius = 6.0f, .color = palette.gray800, .padding = Vec4{ 12.0f, 6.0f, 12.0f, 6.0f }, .gap = Vec2{ 8.0f, 8.0f }, .minHeight = 32.0f, .fontSize = 13.0f, .iconSize = Vec2{ 16.0f, 16.0f } } },
        { .selector = { .state = ButtonTheme::State::Hover }, .style = { .background = palette.gray250, .borderColor = palette.gray300 } },
        { .selector = { .state = ButtonTheme::State::Active }, .style = { .background = palette.gray300, .borderColor = palette.gray300 } },
        { .selector = { .state = ButtonTheme::State::Disabled }, .style = { .opacity = 0.45f } },
        { .selector = { .narrow = true }, .style = { .padding = Vec4{ 10.0f, 6.0f, 10.0f, 6.0f } } },
        { .selector = { .size = ButtonSize::Small }, .style = { .padding = Vec4{ 10.0f, 4.0f, 10.0f, 4.0f }, .minHeight = 28.0f, .iconSize = Vec2{ 14.0f, 14.0f } } },
        { .selector = { .size = ButtonSize::Small, .narrow = true }, .style = { .padding = Vec4{ 8.0f, 4.0f, 8.0f, 4.0f } } },
        { .selector = { .size = ButtonSize::Large }, .style = { .minHeight = 36.0f } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Primary }, .style = { .background = palette.blue500, .borderColor = palette.blue550, .color = palette.white } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Hover }, .style = { .background = palette.blue550, .borderColor = palette.blue600 } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Active }, .style = { .background = palette.blue600, .borderColor = palette.blue600 } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Success }, .style = { .background = palette.green500, .borderColor = palette.green550, .color = palette.white } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Hover }, .style = { .background = palette.green550, .borderColor = palette.green600 } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Active }, .style = { .background = palette.green600, .borderColor = palette.green600 } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Warning }, .style = { .background = palette.yellow500, .borderColor = palette.yellow550, .color = palette.white } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Hover }, .style = { .background = palette.yellow550, .borderColor = palette.yellow600 } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Active }, .style = { .background = palette.yellow600, .borderColor = palette.yellow600 } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Danger }, .style = { .background = palette.red500, .borderColor = palette.red550, .color = palette.white } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Hover }, .style = { .background = palette.red550, .borderColor = palette.red600 } },
        { .selector = { .kind = ButtonKind::Default, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Active }, .style = { .background = palette.red600, .borderColor = palette.red600 } },
        { .selector = { .kind = ButtonKind::Outlined }, .style = { .background = _transparent } },
        { .selector = { .kind = ButtonKind::Outlined, .state = ButtonTheme::State::Hover }, .style = { .background = _transparent, .borderColor = palette.gray300 } },
        { .selector = { .kind = ButtonKind::Outlined, .state = ButtonTheme::State::Active }, .style = { .background = _transparent, .borderColor = palette.gray350 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Primary }, .style = { .borderColor = palette.blue500, .color = palette.blue600 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Hover }, .style = { .borderColor = palette.blue550, .color = palette.blue650 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Active }, .style = { .borderColor = palette.blue650, .color = palette.blue750 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Success }, .style = { .borderColor = palette.green500, .color = palette.green600 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Hover }, .style = { .borderColor = palette.green550, .color = palette.green650 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Active }, .style = { .borderColor = palette.green650, .color = palette.green750 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Warning }, .style = { .borderColor = palette.yellow500, .color = palette.yellow600 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Hover }, .style = { .borderColor = palette.yellow550, .color = palette.yellow650 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Active }, .style = { .borderColor = palette.yellow650, .color = palette.yellow750 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger }, .style = { .borderColor = palette.red500, .color = palette.red600 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Hover }, .style = { .borderColor = palette.red550, .color = palette.red650 } },
        { .selector = { .kind = ButtonKind::Outlined, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Active }, .style = { .borderColor = palette.red650, .color = palette.red750 } },
        { .selector = { .kind = ButtonKind::Flat }, .style = { .background = _transparent, .borderColor = _transparent } },
        { .selector = { .kind = ButtonKind::Flat, .state = ButtonTheme::State::Hover }, .style = { .background = palette.gray150 } },
        { .selector = { .kind = ButtonKind::Flat, .state = ButtonTheme::State::Active }, .style = { .background = palette.gray250 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Primary }, .style = { .color = palette.blue600 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Hover }, .style = { .background = palette.blue150, .color = palette.blue650 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Active }, .style = { .background = palette.blue250, .color = palette.blue750 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Success }, .style = { .color = palette.green600 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Hover }, .style = { .background = palette.green150, .color = palette.green650 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Active }, .style = { .background = palette.green250, .color = palette.green750 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Warning }, .style = { .color = palette.yellow600 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Hover }, .style = { .background = palette.yellow150, .color = palette.yellow650 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Active }, .style = { .background = palette.yellow250, .color = palette.yellow750 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Danger }, .style = { .color = palette.red600 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Hover }, .style = { .background = palette.red150, .color = palette.red650 } },
        { .selector = { .kind = ButtonKind::Flat, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Active }, .style = { .background = palette.red250, .color = palette.red750 } },
        { .selector = { .kind = ButtonKind::Transparent }, .style = { .background = _transparent, .borderColor = _transparent } },
        { .selector = { .kind = ButtonKind::Transparent, .state = ButtonTheme::State::Hover }, .style = { .background = _transparent } },
        { .selector = { .kind = ButtonKind::Transparent, .state = ButtonTheme::State::Active }, .style = { .background = _transparent } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Primary }, .style = { .color = palette.blue600 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Hover }, .style = { .color = palette.blue650 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Primary, .state = ButtonTheme::State::Active }, .style = { .color = palette.blue750 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Success }, .style = { .color = palette.green600 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Hover }, .style = { .color = palette.green650 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Success, .state = ButtonTheme::State::Active }, .style = { .color = palette.green750 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Warning }, .style = { .color = palette.yellow600 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Hover }, .style = { .color = palette.yellow650 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Warning, .state = ButtonTheme::State::Active }, .style = { .color = palette.yellow750 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Danger }, .style = { .color = palette.red600 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Hover }, .style = { .color = palette.red650 } },
        { .selector = { .kind = ButtonKind::Transparent, .intent = ButtonIntent::Danger, .state = ButtonTheme::State::Active }, .style = { .color = palette.red750 } }
    };
}

static std::vector<CheckboxTheme::Rule> _CreateCheckboxRules(_DefaultToggleColors const& colors) {
    auto rules = std::vector<CheckboxTheme::Rule>{
        { .style = { .background = colors.off, .borderRadius = 4.0f, .boxSize = 18.0f, .markColor = colors.content, .markSize = Vec2{ 12.0f, 12.0f }, .color = colors.label, .fontSize = 14.0f, .gap = 8.0f } },
        { .selector = { .state = CheckboxTheme::State::Hover }, .style = { .background = colors.offHover } },
        { .selector = { .state = CheckboxTheme::State::Active }, .style = { .background = colors.offActive } },
        { .selector = { .state = CheckboxTheme::State::Disabled }, .style = { .opacity = 0.4f } },
        { .selector = { .size = CheckboxSize::Small }, .style = { .borderRadius = 3.0f, .boxSize = 14.0f, .markSize = Vec2{ 10.0f, 10.0f }, .fontSize = 12.0f, .gap = 6.0f } },
        { .selector = { .size = CheckboxSize::Large }, .style = { .borderRadius = 5.0f, .boxSize = 22.0f, .markSize = Vec2{ 16.0f, 16.0f }, .fontSize = 16.0f, .gap = 10.0f } },
        { .selector = { .checked = true }, .style = { .mark = SvgIconData{ _checkMark } } }
    };

    for (auto const& [intent, accent] : std::vector<std::tuple<CheckboxIntent, _DefaultToggleAccent>>{
        { CheckboxIntent::Default, colors.neutral },
        { CheckboxIntent::Primary, colors.primary },
        { CheckboxIntent::Success, colors.success },
        { CheckboxIntent::Warning, colors.warning },
        { CheckboxIntent::Danger, colors.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent, .checked = true }, .style = { .background = accent.on } });
        rules.push_back({ .selector = { .intent = intent, .state = CheckboxTheme::State::Hover, .checked = true }, .style = { .background = accent.onHover } });
        rules.push_back({ .selector = { .intent = intent, .state = CheckboxTheme::State::Active, .checked = true }, .style = { .background = accent.onActive } });
    }

    return rules;
}

static std::vector<RadioboxTheme::Rule> _CreateRadioboxRules(_DefaultToggleColors const& colors) {
    auto rules = std::vector<RadioboxTheme::Rule>{
        { .style = { .background = colors.off, .boxSize = 18.0f, .dotBackground = colors.content, .color = colors.label, .fontSize = 14.0f, .gap = 8.0f } },
        { .selector = { .state = RadioboxTheme::State::Hover }, .style = { .background = colors.offHover } },
        { .selector = { .state = RadioboxTheme::State::Active }, .style = { .background = colors.offActive } },
        { .selector = { .state = RadioboxTheme::State::Disabled }, .style = { .opacity = 0.4f } },
        { .selector = { .size = RadioboxSize::Small }, .style = { .boxSize = 14.0f, .fontSize = 12.0f, .gap = 6.0f } },
        { .selector = { .size = RadioboxSize::Large }, .style = { .boxSize = 22.0f, .fontSize = 16.0f, .gap = 10.0f } },
        { .selector = { .checked = true }, .style = { .dotSize = 8.0f } },
        { .selector = { .size = RadioboxSize::Small, .checked = true }, .style = { .dotSize = 6.0f } },
        { .selector = { .size = RadioboxSize::Large, .checked = true }, .style = { .dotSize = 10.0f } }
    };

    for (auto const& [intent, accent] : std::vector<std::tuple<RadioboxIntent, _DefaultToggleAccent>>{
        { RadioboxIntent::Default, colors.neutral },
        { RadioboxIntent::Primary, colors.primary },
        { RadioboxIntent::Success, colors.success },
        { RadioboxIntent::Warning, colors.warning },
        { RadioboxIntent::Danger, colors.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent, .checked = true }, .style = { .background = accent.on } });
        rules.push_back({ .selector = { .intent = intent, .state = RadioboxTheme::State::Hover, .checked = true }, .style = { .background = accent.onHover } });
        rules.push_back({ .selector = { .intent = intent, .state = RadioboxTheme::State::Active, .checked = true }, .style = { .background = accent.onActive } });
    }

    return rules;
}

static std::vector<SwitchTheme::Rule> _CreateSwitchRules(_DefaultToggleColors const& colors) {
    auto rules = std::vector<SwitchTheme::Rule>{
        { .style = { .background = colors.off, .trackSize = Vec2{ 34.0f, 20.0f }, .knobBackground = colors.content, .knobSize = 14.0f, .color = colors.label, .fontSize = 14.0f, .gap = 8.0f } },
        { .selector = { .state = SwitchTheme::State::Hover }, .style = { .background = colors.offHover } },
        { .selector = { .state = SwitchTheme::State::Active }, .style = { .background = colors.offActive } },
        { .selector = { .state = SwitchTheme::State::Disabled }, .style = { .opacity = 0.4f } },
        { .selector = { .size = SwitchSize::Small }, .style = { .trackSize = Vec2{ 28.0f, 16.0f }, .knobSize = 10.0f, .fontSize = 12.0f, .gap = 6.0f } },
        { .selector = { .size = SwitchSize::Large }, .style = { .trackSize = Vec2{ 42.0f, 24.0f }, .knobSize = 18.0f, .fontSize = 16.0f, .gap = 10.0f } }
    };

    for (auto const& [intent, accent] : std::vector<std::tuple<SwitchIntent, _DefaultToggleAccent>>{
        { SwitchIntent::Default, colors.neutral },
        { SwitchIntent::Primary, colors.primary },
        { SwitchIntent::Success, colors.success },
        { SwitchIntent::Warning, colors.warning },
        { SwitchIntent::Danger, colors.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent, .checked = true }, .style = { .background = accent.on } });
        rules.push_back({ .selector = { .intent = intent, .state = SwitchTheme::State::Hover, .checked = true }, .style = { .background = accent.onHover } });
        rules.push_back({ .selector = { .intent = intent, .state = SwitchTheme::State::Active, .checked = true }, .style = { .background = accent.onActive } });
    }

    return rules;
}

static std::vector<IconTheme::Rule> _CreateIconRules(Vec4 const& color, Vec4 const& primary, Vec4 const& success, Vec4 const& warning, Vec4 const& danger) {
    return {
        { .style = { .color = color, .size = Vec2{ 16.0f, 16.0f } } },
        { .selector = { .intent = IconIntent::Secondary }, .style = { .color = _WithAlpha(color, 0.5f) } },
        { .selector = { .intent = IconIntent::Primary }, .style = { .color = primary } },
        { .selector = { .intent = IconIntent::Success }, .style = { .color = success } },
        { .selector = { .intent = IconIntent::Warning }, .style = { .color = warning } },
        { .selector = { .intent = IconIntent::Danger }, .style = { .color = danger } },
        { .selector = { .size = IconSize::Small }, .style = { .size = Vec2{ 14.0f, 14.0f } } },
        { .selector = { .size = IconSize::Large }, .style = { .size = Vec2{ 20.0f, 20.0f } } },
        { .selector = { .disabled = true }, .style = { .alpha = 0.5f } }
    };
}

static Theme _CreateDarkTheme(ColorPalette const& palette) {
    auto const toggles = _DefaultToggleColors{
        .off = palette.neutral600,
        .offHover = palette.neutral550,
        .offActive = palette.neutral650,
        .content = palette.white,
        .label = palette.neutral200,
        .neutral = { palette.blue500, palette.blue450, palette.blue550 },
        .primary = { palette.blue500, palette.blue450, palette.blue550 },
        .success = { palette.green500, palette.green450, palette.green550 },
        .warning = { palette.amber500, palette.amber450, palette.amber550 },
        .danger = { palette.red500, palette.red450, palette.red550 }
    };

    return Theme{
        .material = nullptr,
        .button = { .rules = _CreateDarkButtonRules({
            { ButtonIntent::Default, { palette.neutral800, palette.neutral750, palette.neutral850, palette.neutral700, palette.neutral200, palette.neutral300, palette.neutral300 } },
            { ButtonIntent::Primary, { palette.blue600,    palette.blue650,    palette.blue700,    palette.blue600,    palette.white,      palette.blue400,    palette.blue400 } },
            { ButtonIntent::Success, { palette.green600,   palette.green650,   palette.green700,   palette.green600,   palette.white,      palette.green400,   palette.green400 } },
            { ButtonIntent::Warning, { palette.amber600,   palette.amber650,   palette.amber700,   palette.amber600,   palette.white,      palette.amber400,   palette.amber400 } },
            { ButtonIntent::Danger,  { palette.red600,     palette.red650,     palette.red700,     palette.red600,     palette.white,      palette.red400,     palette.red400 } }
        }, _darkAlphas) },
        .icon = { .rules = _CreateIconRules(palette.neutral300, palette.blue400, palette.green400, palette.amber400, palette.red400) },
        .checkbox = { .rules = _CreateCheckboxRules(toggles) },
        .radiobox = { .rules = _CreateRadioboxRules(toggles) },
        .switch_ = { .rules = _CreateSwitchRules(toggles) }
    };
}

static Theme _CreateLightTheme(ColorPalette const& palette) {
    auto const toggles = _DefaultToggleColors{
        .off = palette.gray300,
        .offHover = palette.gray350,
        .offActive = palette.gray400,
        .content = palette.white,
        .label = palette.gray800,
        .neutral = { palette.blue500, palette.blue550, palette.blue600 },
        .primary = { palette.blue500, palette.blue550, palette.blue600 },
        .success = { palette.green500, palette.green550, palette.green600 },
        .warning = { palette.amber500, palette.amber550, palette.amber600 },
        .danger = { palette.red500, palette.red550, palette.red600 }
    };

    return Theme{
        .material = nullptr,
        .button = { .rules = _CreateLightButtonRules(palette) },
        .icon = { .rules = _CreateIconRules(palette.neutral800, palette.blue600, palette.green600, palette.amber600, palette.red600) },
        .checkbox = { .rules = _CreateCheckboxRules(toggles) },
        .radiobox = { .rules = _CreateRadioboxRules(toggles) },
        .switch_ = { .rules = _CreateSwitchRules(toggles) }
    };
}

Theme const& GetDefaultDarkTheme() {
    PROFILE

    static auto const theme = _CreateDarkTheme(GetDefaultPalette());

    return theme;
}

Theme const& GetDefaultLightTheme() {
    PROFILE

    static auto const theme = _CreateLightTheme(GetDefaultPalette());

    return theme;
}

} /* namespace Rocket */
