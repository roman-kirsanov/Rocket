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
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Color family of a switch. */
enum class SwitchIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/** Size of a switch. */
enum class SwitchSize {
    Medium,
    Small,
    Large
};

/**
 * Everything Switch takes from the theme: a list of rules, resolved as those
 * of ButtonTheme are.
 */
struct SwitchTheme {
    /** State of the switch a rule can match. */
    enum class State {
        Normal,
        Hover,
        Active,
        Disabled
    };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the switch. */
        std::optional<SwitchIntent> intent;
        /** Size of the switch. */
        std::optional<SwitchSize> size;
        /** State of the switch; hover and active are left for disabled. */
        std::optional<State> state;
        /** Whether the switch is on. */
        std::optional<bool> checked;
    };

    /**
     * What a rule sets; an unset field is left to other rules. The track, a
     * capsule, and the knob, a circle, are each drawn with the theme's
     * material when their resolved material kind is set, otherwise with the
     * plain colors.
     */
    struct Style {
        /** Kind of the material capsule the track is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material capsule the track is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material capsule the track is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Fill color of the track, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color of the track, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width of the track in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Width and height of the track in points. */
        std::optional<Vec2> trackSize;
        /** Kind of the material circle the knob is drawn with. */
        std::optional<MaterialKind> knobMaterialKind;
        /** Intent of the material circle the knob is drawn with (default Default). */
        std::optional<MaterialIntent> knobMaterialIntent;
        /** Brightness of the material circle the knob is drawn with (default Normal). */
        std::optional<MaterialBrightness> knobMaterialBrightness;
        /** Fill color of the knob, without a material (default none). */
        std::optional<Vec4> knobBackground;
        /** Diameter of the knob in points; it sits as far from the ends of the track as from its sides. */
        std::optional<float> knobSize;
        /** Color of the label. */
        std::optional<Vec4> color;
        /** Font size of the label. */
        std::optional<float> fontSize;
        /** Gap between the track and the label. */
        std::optional<float> gap;
        /** Opacity of the whole switch. */
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

/** Props for the Switch component. */
struct SwitchProps {
    /** Reconciliation key. */
    std::string key;
    /** Text after the track; unset shows none. */
    std::optional<std::string> label;
    /** Whether the switch is on (default false). */
    std::optional<bool> checked;
    /** Color family (default Default). */
    std::optional<SwitchIntent> intent;
    /** Size (default Medium). */
    std::optional<SwitchSize> size;
    /** Fades the switch and ignores clicks and keys (default false). */
    std::optional<bool> disabled;
    /** Invoked on a click or Space, unless disabled, with the value the switch is to take. */
    std::function<void(bool)> onChange;
    /** Props forwarded to the track Node. */
    NodeProps trackProps;
    /** Props forwarded to the label text Node. */
    NodeProps textProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a track with a knob at its start when off and at its end when on,
 * followed by an optional label. A click anywhere on it, or Space while it
 * has focus, calls `onChange` with the opposite of `checked`; the switch
 * keeps no value of its own.
 *
 * @param props Desired switch configuration for this render.
 */
void Switch(SwitchProps const& props);

} /* namespace Rocket */
