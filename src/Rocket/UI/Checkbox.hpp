/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/UI/Icon.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Color family of a checkbox. */
enum class CheckboxIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/** Size of a checkbox. */
enum class CheckboxSize {
    Medium,
    Small,
    Large
};

/**
 * Everything Checkbox takes from the theme: a list of rules, resolved as
 * those of ButtonTheme are.
 */
struct CheckboxTheme {
    /** State of the checkbox a rule can match. */
    enum class State {
        Normal,
        Hover,
        Active,
        Disabled
    };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the checkbox. */
        std::optional<CheckboxIntent> intent;
        /** Size of the checkbox. */
        std::optional<CheckboxSize> size;
        /** State of the checkbox; hover and active are left for disabled. */
        std::optional<State> state;
        /** Whether the checkbox is checked. */
        std::optional<bool> checked;
    };

    /**
     * What a rule sets; an unset field is left to other rules. A box whose
     * resolved style has a material kind is drawn with the theme's material,
     * otherwise with the plain colors.
     */
    struct Style {
        /** Kind of the material the box is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material the box is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material the box is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Corner roundness of the material the box is drawn with (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Fill color of the box, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color of the box, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width of the box in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Corner radius of the box in points, without a material (default 0). */
        std::optional<float> borderRadius;
        /** Width and height of the box in points. */
        std::optional<float> boxSize;
        /** Check mark drawn in the box; unset draws none. */
        std::optional<SvgIconData> mark;
        /** Color of the check mark. */
        std::optional<Vec4> markColor;
        /** Size of the check mark in points. */
        std::optional<Vec2> markSize;
        /** Color of the label. */
        std::optional<Vec4> color;
        /** Font size of the label. */
        std::optional<float> fontSize;
        /** Gap between the box and the label. */
        std::optional<float> gap;
        /** Opacity of the whole checkbox. */
        std::optional<float> opacity;
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

/** Props for the Checkbox component. */
struct CheckboxProps {
    /** Reconciliation key. */
    std::string key;
    /** Text after the box; unset shows none. */
    std::optional<std::string> label;
    /** Whether the box is checked (default false). */
    std::optional<bool> checked;
    /** Color family (default Default). */
    std::optional<CheckboxIntent> intent;
    /** Size (default Medium). */
    std::optional<CheckboxSize> size;
    /** Fades the checkbox and ignores clicks and keys (default false). */
    std::optional<bool> disabled;
    /** Invoked on a click or Space, unless disabled, with the value the checkbox is to take. */
    std::function<void(bool)> onChange;
    /** Props forwarded to the box Node. */
    NodeProps boxProps;
    /** Props forwarded to the label text Node. */
    NodeProps textProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a box, checked or not, followed by an optional label. A click
 * anywhere on it, or Space while it has focus, calls `onChange` with the
 * opposite of `checked`; the checkbox keeps no value of its own.
 *
 * @param props Desired checkbox configuration for this render.
 */
void Checkbox(CheckboxProps const& props);

} /* namespace Rocket */
