/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Palette.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Material.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static ButtonTheme::Style const* _FindStyle(ButtonTheme const& theme, ButtonTheme::Selector const& selector) {
    auto const it = std::ranges::find_if(theme.rules, [&](ButtonTheme::Rule const& rule) {
        return (
            rule.selector.kind == selector.kind &&
            rule.selector.intent == selector.intent &&
            rule.selector.size == selector.size &&
            rule.selector.state == selector.state &&
            rule.selector.narrow == selector.narrow
        );
    });

    return (it != theme.rules.end() ? &it->style : nullptr);
}

TEST(Theme, DrawsTheDefaultThemesWithPlainColorsAndNoMaterial) {
    EXPECT_EQ(GetDefaultDarkTheme().material, nullptr);
    EXPECT_EQ(GetDefaultLightTheme().material, nullptr);

    for (auto const& rule : GetDefaultDarkTheme().button.rules) {
        EXPECT_FALSE(rule.style.materialKind.has_value());
    }
}

TEST(Theme, BuildsTheDefaultThemesOnTheDefaultPalette) {
    auto const dark = _FindStyle(GetDefaultDarkTheme().button, { .kind = ButtonKind::Default, .intent = ButtonIntent::Primary });
    auto const light = _FindStyle(GetDefaultLightTheme().button, {});

    ASSERT_NE(dark, nullptr);
    ASSERT_NE(light, nullptr);
    EXPECT_EQ(dark->background, GetDefaultPalette().blue600);
    EXPECT_EQ(light->background, GetDefaultPalette().gray150);
}

TEST(Theme, DrawsThePixelDarkThemeWithThePixelDarkMaterial) {
    auto const base = _FindStyle(GetPixelDarkTheme().button, {});

    EXPECT_EQ(GetPixelDarkTheme().material, &GetPixelDarkMaterial());
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->materialKind, MaterialKind::Raised);
}

TEST(Theme, GivesTheDefaultDarkThemeWhenNoneIsProvided) {
    Theme const* used = nullptr;
    auto reconciler = Reconciler();
    reconciler.setUpdateFn([&]{
        used = &UseTheme();
    });

    reconciler.update();

    EXPECT_EQ(used, &GetDefaultDarkTheme());
}

TEST(Theme, GivesAThemeProvidedConst) {
    Theme const* used = nullptr;
    auto reconciler = Reconciler();
    reconciler.setUpdateFn([&]{
        Context(GetPixelDarkTheme(), [&]{
            used = &UseTheme();
        });
    });

    reconciler.update();

    EXPECT_EQ(used, &GetPixelDarkTheme());
}

TEST(Theme, GivesTheNearestProvidedTheme) {
    Theme const* used = nullptr;
    auto outer = GetDefaultLightTheme();
    auto inner = GetPixelDarkTheme();
    auto reconciler = Reconciler();
    reconciler.setUpdateFn([&]{
        Context(outer, [&]{
            Context(inner, [&]{
                used = &UseTheme();
            });
        });
    });

    reconciler.update();

    EXPECT_EQ(used, &inner);
}
