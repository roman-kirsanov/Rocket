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
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Color family of a radiobox. */
enum class RadioboxIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/** Size of a radiobox. */
enum class RadioboxSize {
    Medium,
    Small,
    Large
};

/**
 * Everything Radiobox takes from the theme: a list of rules, resolved as
 * those of ButtonTheme are.
 */
struct RadioboxTheme {
    /** State of the radiobox a rule can match. */
    enum class State {
        Normal,
        Hover,
        Active,
        Disabled
    };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the radiobox. */
        std::optional<RadioboxIntent> intent;
        /** Size of the radiobox. */
        std::optional<RadioboxSize> size;
        /** State of the radiobox; hover and active are left for disabled. */
        std::optional<State> state;
        /** Whether the radiobox is checked. */
        std::optional<bool> checked;
    };

    /**
     * What a rule sets; an unset field is left to other rules. The round box
     * and the dot are each drawn with the theme's material when their
     * resolved material kind is set, otherwise with the plain colors.
     */
    struct Style {
        /** Kind of the material circle the box is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material circle the box is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material circle the box is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Fill color of the box, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color of the box, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width of the box in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Diameter of the box in points. */
        std::optional<float> boxSize;
        /** Kind of the material circle the dot is drawn with. */
        std::optional<MaterialKind> dotMaterialKind;
        /** Intent of the material circle the dot is drawn with (default Default). */
        std::optional<MaterialIntent> dotMaterialIntent;
        /** Brightness of the material circle the dot is drawn with (default Normal). */
        std::optional<MaterialBrightness> dotMaterialBrightness;
        /** Fill color of the dot, without a material (default none). */
        std::optional<Vec4> dotBackground;
        /** Diameter of the dot in the middle of the box in points; unset draws none. */
        std::optional<float> dotSize;
        /** Color of the label. */
        std::optional<Vec4> color;
        /** Font size of the label. */
        std::optional<float> fontSize;
        /** Gap between the box and the label. */
        std::optional<float> gap;
        /** Opacity of the whole radiobox. */
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

/** Props for the Radiobox component. */
struct RadioboxProps {
    /** Reconciliation key. */
    std::string key;
    /** Text after the box; unset shows none. */
    std::optional<std::string> label;
    /** Whether this option is the one chosen (default false). */
    std::optional<bool> checked;
    /** Color family (default Default). */
    std::optional<RadioboxIntent> intent;
    /** Size (default Medium). */
    std::optional<RadioboxSize> size;
    /** Fades the radiobox and ignores clicks and keys (default false). */
    std::optional<bool> disabled;
    /** Invoked with true on a click or Space while not checked, unless disabled. */
    std::function<void(bool)> onChange;
    /** Props forwarded to the box Node. */
    NodeProps boxProps;
    /** Props forwarded to the label text Node. */
    NodeProps textProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a round box, with a dot in it when checked, followed by an
 * optional label: one option of a group the caller keeps the choice of. A
 * click anywhere on it, or Space while it has focus, calls `onChange` with
 * true while it is not checked; a checked radiobox stays checked.
 *
 * @param props Desired radiobox configuration for this render.
 */
void Radiobox(RadioboxProps const& props);

} /* namespace Rocket */
