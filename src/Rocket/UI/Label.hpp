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
#include <Rocket/Paint/Font.hpp>
#include <Rocket/UI/Icon.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Tooltip.hpp>

namespace Rocket {

/**
 * Everything Label takes from the theme: a list of rules, resolved as those
 * of ButtonTheme are.
 */
struct LabelTheme {
    /** State of the label's icon a rule can match. */
    enum class State {
        Normal,
        /** The pointer is on the icon. */
        Hover
    };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** State of the icon. */
        std::optional<State> state;
    };

    /** What a rule sets; an unset field is left to other rules. */
    struct Style {
        /** Color of the text. */
        std::optional<Vec4> color;
        /** Font size of the text. */
        std::optional<float> fontSize;
        /** Font weight of the text. */
        std::optional<FontWeight> fontWeight;
        /** Minimum height in points. */
        std::optional<float> minHeight;
        /** Mark shown after the text of a required field; unset shows none. */
        std::optional<std::string> requiredMark;
        /** Color of the mark. */
        std::optional<Vec4> requiredColor;
        /** Font size of the mark. */
        std::optional<float> requiredFontSize;
        /** Gap between the text and the mark (default 0). */
        std::optional<float> requiredGap;
        /** Icon that shows the tooltip, when the label sets none of its own. */
        std::optional<SvgIconData> icon;
        /** Color of the icon. */
        std::optional<Vec4> iconColor;
        /** Size of the icon in points. */
        std::optional<Vec2> iconSize;
        /** Opacity of the icon. */
        std::optional<float> iconOpacity;
        /** Gap between the text, or the mark, and the icon (default 0). */
        std::optional<float> iconGap;
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

/** Props for the Label component. */
struct LabelProps {
    /** Reconciliation key. */
    std::string key;
    /** Text of the label; unset shows none. */
    std::optional<std::string> text;
    /** Shows the theme's mark of a required field after the text (default false). */
    std::optional<bool> required;
    /** Text of a tooltip shown from an icon after the label; unset shows no icon. */
    std::optional<std::string> tooltip;
    /** Icon that shows the tooltip; unset shows the theme's icon. */
    std::optional<IconData> icon;
    /** Props forwarded to the text Node. */
    NodeProps textProps;
    /** Props forwarded to the mark's text Node. */
    NodeProps requiredProps;
    /** Props forwarded to the Icon. */
    IconProps iconProps;
    /** Props forwarded to the Tooltip; its text is `tooltip`. */
    TooltipProps tooltipProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders the label of a form field: its text, then `children`, then the
 * mark of a required field and an icon that shows `tooltip` in a Tooltip.
 *
 * @param props    Desired label configuration for this render.
 * @param children Rendered after the text; default no-op.
 */
void Label(LabelProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
