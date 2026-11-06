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

struct _DefaultNotice {
    Vec4 background;
    Vec4 border;
    Vec4 content;
    Vec4 icon;
    Vec4 accent;
};

struct _DefaultNotices {
    _DefaultNotice neutral;
    _DefaultNotice primary;
    _DefaultNotice success;
    _DefaultNotice warning;
    _DefaultNotice danger;
};

struct _DefaultSurface {
    Vec4 backdrop;
    Vec4 background;
    Vec4 border;
    Vec4 content;
    Vec4 onAccent;
    Shadow shadow;
};

struct _DefaultFields {
    Vec4 background;
    Vec4 border;
    Vec4 borderHover;
    Vec4 borderFocus;
    float borderWidth;
    Shadow shadow;
    Vec4 content;
    Vec4 placeholder;
    Vec4 disabledBackground;
    Vec4 disabledContent;
    _DefaultToggleAccent primary;
    _DefaultToggleAccent success;
    _DefaultToggleAccent warning;
    _DefaultToggleAccent danger;
};

struct _DefaultList {
    Vec4 caret;
    Vec4 background;
    Vec4 border;
    Shadow shadow;
    Vec4 option;
    Vec4 highlight;
    Vec4 selected;
    Vec4 selectedContent;
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
static auto constexpr _arrowDown = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 8\"><path fill=\"#000\" d=\"M0 0L16 0L8 8Z\"/></svg>"
);
static auto constexpr _arrowUp = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 8\"><path fill=\"#000\" d=\"M0 8L16 8L8 0Z\"/></svg>"
);
static auto constexpr _arrowLeft = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 8 16\"><path fill=\"#000\" d=\"M8 0L8 16L0 8Z\"/></svg>"
);
static auto constexpr _arrowRight = std::string_view(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 8 16\"><path fill=\"#000\" d=\"M0 0L0 16L8 8Z\"/></svg>"
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

static std::vector<CalloutTheme::Rule> _CreateCalloutRules(_DefaultNotices const& notices) {
    auto rules = std::vector<CalloutTheme::Rule>{
        { .style = { .borderWidth = 1.0f, .borderRadius = 6.0f, .fontSize = 13.0f, .padding = Vec4{ 12.0f, 12.0f, 12.0f, 12.0f }, .gap = 10.0f, .icon = SvgIconData{ _infoIcon }, .iconSize = Vec2{ 16.0f, 16.0f } } },
        { .selector = { .intent = CalloutIntent::Success }, .style = { .icon = SvgIconData{ _successIcon } } },
        { .selector = { .intent = CalloutIntent::Warning }, .style = { .icon = SvgIconData{ _warningIcon } } },
        { .selector = { .intent = CalloutIntent::Danger }, .style = { .icon = SvgIconData{ _dangerIcon } } }
    };

    for (auto const& [intent, notice] : std::vector<std::tuple<CalloutIntent, _DefaultNotice>>{
        { CalloutIntent::Default, notices.neutral },
        { CalloutIntent::Primary, notices.primary },
        { CalloutIntent::Success, notices.success },
        { CalloutIntent::Warning, notices.warning },
        { CalloutIntent::Danger, notices.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent }, .style = { .background = notice.background, .borderColor = notice.border, .color = notice.content } });
    }

    return rules;
}

static std::vector<SpinnerTheme::Rule> _CreateSpinnerRules(Vec4 const& color) {
    return {
        { .style = { .icon = SvgIconData{ _spinnerIcon }, .color = color, .size = Vec2{ 16.0f, 16.0f } } },
        { .selector = { .size = SpinnerSize::Small }, .style = { .size = Vec2{ 12.0f, 12.0f } } },
        { .selector = { .size = SpinnerSize::Large }, .style = { .size = Vec2{ 20.0f, 20.0f } } }
    };
}

static std::vector<ModalTheme::Rule> _CreateModalRules(_DefaultSurface const& surface) {
    return {
        { .style = { .backdrop = surface.backdrop, .background = surface.background, .borderColor = surface.border, .borderWidth = 1.0f, .borderRadius = 6.0f, .shadow = surface.shadow, .color = surface.content, .fontSize = 14.0f } }
    };
}

static std::vector<AlertTheme::Rule> _CreateAlertRules(_DefaultNotices const& notices) {
    auto rules = std::vector<AlertTheme::Rule>{
        { .style = { .padding = Vec4{ 16.0f, 16.0f, 16.0f, 16.0f }, .gap = 16.0f, .contentGap = 16.0f, .textGap = 10.0f, .buttonsGap = 8.0f, .maxWidth = 400.0f, .titleFontSize = 15.0f, .titleFontWeight = FontWeight::Bold, .icon = SvgIconData{ _infoIcon }, .iconSize = Vec2{ 20.0f, 20.0f } } },
        { .selector = { .intent = AlertIntent::Success }, .style = { .icon = SvgIconData{ _successIcon } } },
        { .selector = { .intent = AlertIntent::Warning }, .style = { .icon = SvgIconData{ _warningIcon } } },
        { .selector = { .intent = AlertIntent::Danger }, .style = { .icon = SvgIconData{ _dangerIcon } } },
        { .selector = { .intent = AlertIntent::Default }, .style = { .iconColor = notices.neutral.icon } }
    };

    for (auto const& [intent, notice] : std::vector<std::tuple<AlertIntent, _DefaultNotice>>{
        { AlertIntent::Primary, notices.primary },
        { AlertIntent::Success, notices.success },
        { AlertIntent::Warning, notices.warning },
        { AlertIntent::Danger, notices.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent }, .style = { .borderColor = notice.accent, .iconColor = notice.icon } });
    }

    return rules;
}

static std::vector<DrawerTheme::Rule> _CreateDrawerRules(_DefaultSurface const& surface) {
    return {
        { .style = { .backdrop = surface.backdrop, .background = surface.background, .borderColor = surface.border, .borderWidth = 1.0f, .shadow = surface.shadow, .color = surface.content, .fontSize = 14.0f } }
    };
}

static std::vector<TooltipTheme::Rule> _CreateTooltipRules(_DefaultSurface const& surface, _DefaultNotices const& notices) {
    auto rules = std::vector<TooltipTheme::Rule>{
        { .style = { .background = surface.background, .borderColor = surface.border, .borderWidth = 1.0f, .borderRadius = 5.0f, .shadow = surface.shadow, .color = surface.content, .fontSize = 12.0f, .padding = Vec4{ 8.0f, 4.0f, 8.0f, 4.0f }, .gap = 4.0f, .arrowSize = Vec2{ 16.0f, 8.0f } } },
        { .selector = { .direction = TooltipDirection::Up }, .style = { .arrow = SvgIconData{ _arrowDown } } },
        { .selector = { .direction = TooltipDirection::Down }, .style = { .arrow = SvgIconData{ _arrowUp } } },
        { .selector = { .direction = TooltipDirection::Left }, .style = { .arrow = SvgIconData{ _arrowRight } } },
        { .selector = { .direction = TooltipDirection::Right }, .style = { .arrow = SvgIconData{ _arrowLeft } } }
    };

    for (auto const& [intent, notice] : std::vector<std::tuple<TooltipIntent, _DefaultNotice>>{
        { TooltipIntent::Primary, notices.primary },
        { TooltipIntent::Success, notices.success },
        { TooltipIntent::Warning, notices.warning },
        { TooltipIntent::Danger, notices.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent }, .style = { .background = notice.accent, .borderColor = notice.accent, .color = surface.onAccent } });
    }

    return rules;
}

static std::vector<LabelTheme::Rule> _CreateLabelRules(Vec4 const& color, Vec4 const& required) {
    return {
        { .style = { .color = color, .fontSize = 12.0f, .fontWeight = FontWeight::Medium, .minHeight = 14.0f, .requiredMark = "*", .requiredColor = required, .requiredFontSize = 14.0f, .requiredGap = 3.0f, .icon = SvgIconData{ _questionIcon }, .iconColor = color, .iconSize = Vec2{ 14.0f, 14.0f }, .iconOpacity = 0.5f, .iconGap = 6.0f } },
        { .selector = { .state = LabelTheme::State::Hover }, .style = { .iconOpacity = 1.0f } }
    };
}

static std::vector<InputTheme::Rule> _CreateInputRules(_DefaultFields const& fields) {
    auto rules = std::vector<InputTheme::Rule>{
        { .style = { .background = fields.background, .borderColor = fields.border, .borderWidth = fields.borderWidth, .borderRadius = 6.0f, .shadow = fields.shadow, .color = fields.content, .placeholderColor = fields.placeholder, .fontSize = 14.0f, .padding = Vec4{ 8.0f, 0.0f, 8.0f, 0.0f }, .minHeight = 32.0f, .gap = 0.0f } },
        { .selector = { .size = InputSize::Small }, .style = { .fontSize = 12.0f, .padding = Vec4{ 6.0f, 0.0f, 6.0f, 0.0f }, .minHeight = 28.0f } },
        { .selector = { .size = InputSize::Large }, .style = { .fontSize = 16.0f, .padding = Vec4{ 10.0f, 0.0f, 10.0f, 0.0f }, .minHeight = 36.0f } },
        { .selector = { .state = InputTheme::State::Hover }, .style = { .borderColor = fields.borderHover } },
        { .selector = { .state = InputTheme::State::Focus }, .style = { .borderColor = fields.borderFocus } },
        { .selector = { .state = InputTheme::State::Disabled }, .style = { .background = fields.disabledBackground, .color = fields.disabledContent } }
    };

    for (auto const& [intent, accent] : std::vector<std::tuple<InputIntent, _DefaultToggleAccent>>{
        { InputIntent::Primary, fields.primary },
        { InputIntent::Success, fields.success },
        { InputIntent::Warning, fields.warning },
        { InputIntent::Danger, fields.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent }, .style = { .borderColor = accent.on } });
        rules.push_back({ .selector = { .intent = intent, .state = InputTheme::State::Hover }, .style = { .borderColor = accent.onHover } });
        rules.push_back({ .selector = { .intent = intent, .state = InputTheme::State::Focus }, .style = { .borderColor = accent.onActive } });
    }

    return rules;
}

static std::vector<SelectTheme::Rule> _CreateSelectRules(_DefaultFields const& fields, _DefaultList const& list) {
    auto rules = std::vector<SelectTheme::Rule>{
        { .style = { .background = fields.background, .borderColor = fields.border, .borderWidth = fields.borderWidth, .borderRadius = 6.0f, .shadow = fields.shadow, .color = fields.content, .placeholderColor = fields.placeholder, .fontSize = 14.0f, .padding = Vec4{ 8.0f, 0.0f, 8.0f, 0.0f }, .minHeight = 32.0f, .gap = 8.0f, .caret = SvgIconData{ _caretIcon }, .searchIcon = SvgIconData{ _searchIcon }, .caretColor = list.caret, .caretSize = Vec2{ 12.0f, 12.0f }, .listBackground = list.background, .listBorderColor = list.border, .listBorderWidth = 1.0f, .listBorderRadius = 5.0f, .listShadow = list.shadow, .listPadding = Vec4{ 6.0f, 4.0f, 6.0f, 4.0f }, .listGap = 4.0f, .listMaxHeight = 240.0f, .optionPadding = Vec4{ 8.0f, 6.0f, 8.0f, 6.0f }, .optionBorderRadius = 5.0f, .optionColor = list.option, .optionHighlightBackground = list.highlight, .optionSelectedBackground = list.selected, .optionSelectedColor = list.selectedContent } },
        { .selector = { .size = SelectSize::Small }, .style = { .fontSize = 12.0f, .padding = Vec4{ 6.0f, 0.0f, 6.0f, 0.0f }, .minHeight = 28.0f, .caretSize = Vec2{ 10.0f, 10.0f } } },
        { .selector = { .size = SelectSize::Large }, .style = { .fontSize = 16.0f, .padding = Vec4{ 10.0f, 0.0f, 10.0f, 0.0f }, .minHeight = 36.0f, .caretSize = Vec2{ 14.0f, 14.0f } } },
        { .selector = { .state = SelectTheme::State::Hover }, .style = { .borderColor = fields.borderHover } },
        { .selector = { .state = SelectTheme::State::Focus }, .style = { .borderColor = fields.borderFocus } },
        { .selector = { .state = SelectTheme::State::Disabled }, .style = { .background = fields.disabledBackground, .color = fields.disabledContent } }
    };

    for (auto const& [intent, accent] : std::vector<std::tuple<SelectIntent, _DefaultToggleAccent>>{
        { SelectIntent::Primary, fields.primary },
        { SelectIntent::Success, fields.success },
        { SelectIntent::Warning, fields.warning },
        { SelectIntent::Danger, fields.danger }
    }) {
        rules.push_back({ .selector = { .intent = intent }, .style = { .borderColor = accent.on } });
        rules.push_back({ .selector = { .intent = intent, .state = SelectTheme::State::Hover }, .style = { .borderColor = accent.onHover } });
        rules.push_back({ .selector = { .intent = intent, .state = SelectTheme::State::Focus }, .style = { .borderColor = accent.onActive } });
    }

    return rules;
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

    auto const notices = _DefaultNotices{
        .neutral = { palette.neutral800, palette.neutral700, palette.neutral200, palette.neutral300, palette.neutral500 },
        .primary = { _WithAlpha(palette.blue500, 0.12f), _WithAlpha(palette.blue500, 0.35f), palette.blue300, palette.blue400, palette.blue500 },
        .success = { _WithAlpha(palette.green500, 0.12f), _WithAlpha(palette.green500, 0.35f), palette.green300, palette.green400, palette.green500 },
        .warning = { _WithAlpha(palette.amber500, 0.12f), _WithAlpha(palette.amber500, 0.35f), palette.amber300, palette.amber400, palette.amber500 },
        .danger = { _WithAlpha(palette.red500, 0.12f), _WithAlpha(palette.red500, 0.35f), palette.red300, palette.red400, palette.red500 }
    };
    auto const surface = _DefaultSurface{
        .backdrop = _WithAlpha(palette.black, 0.6f),
        .background = palette.neutral800,
        .border = palette.neutral700,
        .content = palette.neutral200,
        .onAccent = palette.white,
        .shadow = { .color = _WithAlpha(palette.black, 0.4f), .offset = Vec2{ 0.0f, 4.0f }, .blur = 12.0f }
    };

    auto const fields = _DefaultFields{
        .background = palette.neutral900,
        .border = palette.neutral700,
        .borderHover = palette.neutral600,
        .borderFocus = palette.blue500,
        .borderWidth = 1.0f,
        .shadow = { .color = _transparent },
        .content = palette.neutral100,
        .placeholder = palette.neutral500,
        .disabledBackground = palette.neutral850,
        .disabledContent = palette.neutral500,
        .primary = { palette.blue600, palette.blue500, palette.blue400 },
        .success = { palette.green600, palette.green500, palette.green400 },
        .warning = { palette.amber600, palette.amber500, palette.amber400 },
        .danger = { palette.red600, palette.red500, palette.red400 }
    };

    auto const list = _DefaultList{
        .caret = palette.neutral400,
        .background = palette.neutral800,
        .border = palette.neutral700,
        .shadow = surface.shadow,
        .option = palette.neutral100,
        .highlight = palette.neutral700,
        .selected = _WithAlpha(palette.blue500, 0.25f),
        .selectedContent = palette.blue200
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
        .switch_ = { .rules = _CreateSwitchRules(toggles) },
        .callout = { .rules = _CreateCalloutRules(notices) },
        .spinner = { .rules = _CreateSpinnerRules(palette.blue400) },
        .modal = { .rules = _CreateModalRules(surface) },
        .alert = { .rules = _CreateAlertRules(notices) },
        .drawer = { .rules = _CreateDrawerRules(surface) },
        .tooltip = { .rules = _CreateTooltipRules(surface, notices) },
        .label = { .rules = _CreateLabelRules(palette.neutral200, palette.red400) },
        .input = { .rules = _CreateInputRules(fields) },
        .select = { .rules = _CreateSelectRules(fields, list) }
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

    auto const notices = _DefaultNotices{
        .neutral = { palette.gray100, palette.gray150, palette.gray900, palette.gray600, palette.gray300 },
        .primary = { palette.blue100, palette.blue150, palette.blue600, palette.blue500, palette.blue500 },
        .success = { palette.green100, palette.green150, palette.green600, palette.green500, palette.green500 },
        .warning = { palette.yellow100, palette.yellow150, palette.yellow600, palette.yellow500, palette.yellow500 },
        .danger = { palette.red100, palette.red150, palette.red600, palette.red500, palette.red500 }
    };
    auto const surface = _DefaultSurface{
        .backdrop = _WithAlpha(palette.black, 0.5f),
        .background = palette.white,
        .border = palette.gray300,
        .content = palette.gray900,
        .onAccent = palette.white,
        .shadow = { .color = _WithAlpha(palette.black, 0.1f), .offset = Vec2{ 0.0f, 4.0f }, .blur = 6.0f }
    };

    auto const fields = _DefaultFields{
        .background = palette.white,
        .border = palette.gray300,
        .borderHover = palette.gray350,
        .borderFocus = palette.blue400,
        .borderWidth = 2.0f,
        .shadow = { .color = _WithAlpha(palette.black, 0.06f), .offset = Vec2{ 0.0f, 2.0f }, .blur = 3.0f, .inset = true },
        .content = palette.gray900,
        .placeholder = palette.gray400,
        .disabledBackground = palette.gray100,
        .disabledContent = palette.gray400,
        .primary = { palette.blue450, palette.blue400, palette.blue400 },
        .success = { palette.green450, palette.green400, palette.green400 },
        .warning = { palette.yellow450, palette.yellow400, palette.yellow400 },
        .danger = { palette.red450, palette.red400, palette.red400 }
    };

    auto const list = _DefaultList{
        .caret = palette.gray400,
        .background = palette.white,
        .border = palette.gray300,
        .shadow = surface.shadow,
        .option = palette.gray900,
        .highlight = palette.gray100,
        .selected = palette.blue100,
        .selectedContent = palette.blue700
    };

    return Theme{
        .material = nullptr,
        .button = { .rules = _CreateLightButtonRules(palette) },
        .icon = { .rules = _CreateIconRules(palette.neutral800, palette.blue600, palette.green600, palette.amber600, palette.red600) },
        .checkbox = { .rules = _CreateCheckboxRules(toggles) },
        .radiobox = { .rules = _CreateRadioboxRules(toggles) },
        .switch_ = { .rules = _CreateSwitchRules(toggles) },
        .callout = { .rules = _CreateCalloutRules(notices) },
        .spinner = { .rules = _CreateSpinnerRules(palette.blue500) },
        .modal = { .rules = _CreateModalRules(surface) },
        .alert = { .rules = _CreateAlertRules(notices) },
        .drawer = { .rules = _CreateDrawerRules(surface) },
        .tooltip = { .rules = _CreateTooltipRules(surface, notices) },
        .label = { .rules = _CreateLabelRules(palette.gray900, palette.red600) },
        .input = { .rules = _CreateInputRules(fields) },
        .select = { .rules = _CreateSelectRules(fields, list) }
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
