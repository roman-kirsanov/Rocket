/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/UI/Icon.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Size of a spinner. */
enum class SpinnerSize {
    Medium,
    Small,
    Large
};

/**
 * Everything Spinner takes from the theme: a list of rules, resolved as
 * those of ButtonTheme are.
 */
struct SpinnerTheme {
    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Size of the spinner. */
        std::optional<SpinnerSize> size;
    };

    /** What a rule sets; an unset field is left to other rules. */
    struct Style {
        /** Drawing of the spinner; unset draws nothing. */
        std::optional<SvgIconData> icon;
        /** Color of the drawing. */
        std::optional<Vec4> color;
        /** Size in points. */
        std::optional<Vec2> size;
    };

    /** A selector and the style it applies. */
    struct Rule {
        /** What the rule matches. */
        Selector selector;
        /** What the rule sets. */
        Style style;
        /** Beats every rule that is not important, whatever its selector. */
        bool important = false;
    };

    /** The rules, in order. */
    std::vector<Rule> rules;
};

/** Props for the Spinner component. */
struct SpinnerProps {
    /** Reconciliation key. */
    std::string key;
    /** Size (default Medium). */
    std::optional<SpinnerSize> size;
    /** Color of the drawing; overrides the theme. */
    std::optional<Vec4> color;
    /** Props forwarded to the Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders the theme's drawing of a spinner, to show that something is
 * loading. It does not turn yet: nodes cannot be rotated.
 *
 * @param props Desired spinner configuration for this render.
 */
void Spinner(SpinnerProps const& props);

} /* namespace Rocket */
