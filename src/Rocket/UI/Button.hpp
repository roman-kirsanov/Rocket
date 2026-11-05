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

/** Color family of a button. */
enum class ButtonIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/** Look of a button. */
enum class ButtonKind {
    Default,
    Outlined,
    /** Draws nothing until hovered or pressed. */
    Transparent,
    Flat
};

/** Size of a button. */
enum class ButtonSize {
    Medium,
    Small,
    Large
};

/**
 * Everything Button takes from the theme: a list of rules, as in a CSS
 * stylesheet. Every rule whose selector matches the button applies its style;
 * where two rules set the same field, an important rule beats one that is
 * not, then the rule whose selector sets more fields wins, then the later one.
 */
struct ButtonTheme {
    /** State of the button a rule can match. */
    enum class State {
        Normal,
        Hover,
        Active,
        Disabled
    };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Kind of the button. */
        std::optional<ButtonKind> kind;
        /** Intent of the button. */
        std::optional<ButtonIntent> intent;
        /** Size of the button. */
        std::optional<ButtonSize> size;
        /** State of the button; hover and active are left for disabled. */
        std::optional<State> state;
        /** Whether the button is narrow. */
        std::optional<bool> narrow;
    };

    /**
     * What a rule sets; an unset field is left to other rules. A button whose
     * resolved style has a material kind is drawn with the theme's material,
     * otherwise with the plain colors.
     */
    struct Style {
        /** Kind of the material drawn. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material drawn (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material drawn (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Corner roundness of the material drawn (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Fill color, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width in points, without a material; a left-out edge has none (default 0). */
        std::optional<float> borderWidth;
        /** Corner radius in points, without a material; the corners on a left-out edge are square (default 0). */
        std::optional<float> borderRadius;
        /** Color of the label and icon. */
        std::optional<Vec4> color;
        /** Padding as left, top, right, bottom. */
        std::optional<Vec4> padding;
        /** Gap between the icon and the label. */
        std::optional<Vec2> gap;
        /** Minimum height. */
        std::optional<float> minHeight;
        /** Font size of the label. */
        std::optional<float> fontSize;
        /** Size of the icon. */
        std::optional<Vec2> iconSize;
        /** Opacity of the whole button. */
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

/** Props for the Button component. */
struct ButtonProps {
    /** Reconciliation key. */
    std::string key;
    /** Text of the label; unset shows none. */
    std::optional<std::string> label;
    /** Icon shown before the label; unset shows none. */
    std::optional<IconData> icon;
    /** Color family (default Default). */
    std::optional<ButtonIntent> intent;
    /** Look (default Default). */
    std::optional<ButtonKind> kind;
    /** Size (default Medium). */
    std::optional<ButtonSize> size;
    /** Draws the button pressed, as for a toggle that is on (default false). */
    std::optional<bool> active;
    /** Pads the button as much on the sides as on the top and bottom, for an icon alone (default false). */
    std::optional<bool> narrow;
    /** Fades the button and ignores clicks (default false). */
    std::optional<bool> disabled;
    /** Leaves out the left edge, so the button joins a neighbor on that side (default false). */
    std::optional<bool> noLeftBorder;
    /** Leaves out the top edge, so the button joins a neighbor on that side (default false). */
    std::optional<bool> noTopBorder;
    /** Leaves out the right edge, so the button joins a neighbor on that side (default false). */
    std::optional<bool> noRightBorder;
    /** Leaves out the bottom edge, so the button joins a neighbor on that side (default false). */
    std::optional<bool> noBottomBorder;
    /** Invoked on click, unless disabled. */
    std::function<void()> onClick;
    /** Props forwarded to the label text Node. */
    NodeProps textProps;
    /** Props forwarded to the Icon; its data defaults to `icon`, and its color to the button's content color unless it sets an intent. */
    IconProps iconProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a button drawn with a material: an optional icon followed by an
 * optional label. It lightens when hovered, darkens while pressed or active,
 * and calls `onClick` when clicked unless disabled.
 *
 * @param props Desired button configuration for this render.
 */
void Button(ButtonProps const& props);

} /* namespace Rocket */
