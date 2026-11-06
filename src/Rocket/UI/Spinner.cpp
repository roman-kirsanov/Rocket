/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <optional>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Spinner.hpp>

namespace Rocket {

struct _SpinnerState {
    SpinnerTheme::Style style;
    Effect<
        Theme const*,
        std::optional<SpinnerSize>
    > shouldUpdate;
};

static SpinnerTheme::Style _ResolveStyle(SpinnerTheme const& theme, SpinnerSize size) {
    auto const rules = _GetCascade<SpinnerTheme::Rule>(
        theme.rules,
        [&](SpinnerTheme::Rule const& rule) {
            return (rule.selector.size.has_value() == false || rule.selector.size == size);
        },
        [](SpinnerTheme::Rule const& rule) {
            return (rule.selector.size.has_value() ? 1 : 0);
        }
    );

    auto style = SpinnerTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.icon, rule->style.icon);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.size, rule->style.size);
    }

    return style;
}

void Spinner(SpinnerProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SpinnerState>();
    auto const& theme = UseTheme();

    if (state.shouldUpdate(&theme, props.size)) {
        state.style = _ResolveStyle(theme.spinner, props.size.value_or(SpinnerSize::Medium));
    }

    auto icon = IconProps();
    icon.nodeProps = props.nodeProps;
    icon.colorValue = (props.color.has_value() ? props.color : state.style.color);
    icon.sizeValue = state.style.size;

    if (state.style.icon.has_value()) {
        icon.data.emplace(state.style.icon.value());
    }

    Icon(icon);
}

} /* namespace Rocket */
