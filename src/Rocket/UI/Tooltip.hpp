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
#include <Rocket/Paint/Shadow.hpp>
#include <Rocket/UI/Icon.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Side of its anchor a tooltip is shown on. */
enum class TooltipDirection {
    Up,
    Down,
    Left,
    Right
};

/** How a tooltip lines up with its anchor along the anchor's side. */
enum class TooltipAlignment {
    /** Centered on the anchor. */
    Center,
    /** Lined up with the anchor's left edge, or its top edge on the left or right. */
    Start,
    /** Lined up with the anchor's right edge, or its bottom edge on the left or right. */
    End
};

/** Color family of a tooltip. */
enum class TooltipIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/**
 * Everything Tooltip takes from the theme: a list of rules, resolved as
 * those of ButtonTheme are.
 */
struct TooltipTheme {
    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the tooltip. */
        std::optional<TooltipIntent> intent;
        /** Side of the anchor the tooltip is shown on, once it is kept inside the document. */
        std::optional<TooltipDirection> direction;
    };

    /**
     * What a rule sets; an unset field is left to other rules. A tooltip
     * whose resolved style has a material kind is drawn with the theme's
     * material, its arrow too, otherwise with the plain colors and `arrow`.
     */
    struct Style {
        /** Kind of the material the tooltip is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material the tooltip and its arrow are drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material the tooltip and its arrow are drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Corner roundness of the material the tooltip is drawn with (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Fill color, and that of the arrow, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color, and that of the arrow's edge, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Corner radius in points, without a material (default 0). */
        std::optional<float> borderRadius;
        /** Shadow (default none). */
        std::optional<Shadow> shadow;
        /** Color of the text. */
        std::optional<Vec4> color;
        /** Font size of the text. */
        std::optional<float> fontSize;
        /** Padding as left, top, right, bottom (default 0). */
        std::optional<Vec4> padding;
        /** Gap in points between the anchor and the tip of the arrow (default 0). */
        std::optional<float> gap;
        /** Arrow pointing from the tooltip to its anchor, without a material; unset draws none. */
        std::optional<SvgIconData> arrow;
        /** Size of the arrow in points, as drawn: wide along the tooltip's edge, deep toward the anchor. */
        std::optional<Vec2> arrowSize;
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

/** Props for the Tooltip component. */
struct TooltipProps {
    /** Reconciliation key. */
    std::string key;
    /** Text of the tooltip; unset shows no tooltip. */
    std::optional<std::string> text;
    /** Side of the anchor the tooltip is shown on; mirrored when it would run past the document (default Up). */
    std::optional<TooltipDirection> direction;
    /** How the tooltip lines up with the anchor (default Center). */
    std::optional<TooltipAlignment> alignment;
    /** Color family (default Default). */
    std::optional<TooltipIntent> intent;
    /** Gap in points between the anchor and the tip of the arrow; overrides the theme. */
    std::optional<float> gap;
    /** Milliseconds the pointer rests on the anchor before the tooltip shows (default 750). */
    std::optional<double> delayShow;
    /** Milliseconds the pointer is away before the tooltip hides (default 150). */
    std::optional<double> delayHide;
    /** Props forwarded to the text Node. */
    NodeProps textProps;
    /** Props forwarded to the Popup's Node; any field set here overrides the computed style. */
    NodeProps popupProps;
    /** Props forwarded to the anchor Node that holds `children`. */
    NodeProps nodeProps;
};

/**
 * Renders `children` in an anchor Node and, once the pointer has rested on
 * it for `delayShow`, a small Popup beside it that shows `text`, with an
 * arrow that points at it. The tooltip stays while the pointer is on the
 * anchor or on the tooltip, and hides `delayHide` after it leaves, or on a
 * press outside the anchor. Like hover, the delays are seen on the next
 * render: the app renders on every update, as the example apps do.
 *
 * @param props    Desired tooltip configuration for this render.
 * @param children The anchor's content; default no-op.
 */
void Tooltip(TooltipProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
