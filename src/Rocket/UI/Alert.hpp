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
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Color family of an alert. */
enum class AlertIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/**
 * Everything Alert takes from the theme: a list of rules, resolved as those
 * of ButtonTheme are. The panel and the backdrop are those of Modal; a style
 * changes the panel only in the fields it sets.
 */
struct AlertTheme {
    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the alert. */
        std::optional<AlertIntent> intent;
    };

    /** What a rule sets; an unset field is left to other rules. */
    struct Style {
        /** Kind of the material the panel is drawn with, in place of that of Modal; unset keeps that of Modal. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material the panel is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material the panel is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Corner roundness of the material the panel is drawn with (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Edge color of the panel, without a material; unset keeps that of Modal. */
        std::optional<Vec4> borderColor;
        /** Edge width of the panel in points, without a material; unset keeps that of Modal. */
        std::optional<float> borderWidth;
        /** Padding of the panel as left, top, right, bottom (default 0). */
        std::optional<Vec4> padding;
        /** Gap between the content and the buttons (default 0). */
        std::optional<float> gap;
        /** Gap between the icon and the text (default 0). */
        std::optional<float> contentGap;
        /** Gap between the title and the body (default 0). */
        std::optional<float> textGap;
        /** Gap between the buttons (default 0). */
        std::optional<float> buttonsGap;
        /** Widest the content may be, in points (default none). */
        std::optional<float> maxWidth;
        /** Color of the title and the body; unset keeps that of Modal. */
        std::optional<Vec4> color;
        /** Font size of the body; unset keeps that of Modal. */
        std::optional<float> fontSize;
        /** Font size of the title. */
        std::optional<float> titleFontSize;
        /** Font weight of the title. */
        std::optional<FontWeight> titleFontWeight;
        /** Icon shown when the alert shows one and sets none of its own. */
        std::optional<SvgIconData> icon;
        /** Color of the icon. */
        std::optional<Vec4> iconColor;
        /** Size of the icon in points. */
        std::optional<Vec2> iconSize;
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

/** Props for the Alert component. */
struct AlertProps {
    /** Reconciliation key. */
    std::string key;
    /** Color family (default Default). */
    std::optional<AlertIntent> intent;
    /** Shows an icon before the text (default false). */
    std::optional<bool> showIcon;
    /** Icon shown when `showIcon` is set; unset shows the theme's icon for the intent. */
    std::optional<IconData> icon;
    /** Title, above the body; unset shows none. */
    std::optional<std::string> title;
    /** Body text; unset shows none. */
    std::optional<std::string> body;
    /** Renders the buttons, in a row under the text; unset shows no row. */
    std::function<void()> buttons;
    /** Props forwarded to the Icon. */
    IconProps iconProps;
    /** Props forwarded to the title text Node. */
    NodeProps titleProps;
    /** Props forwarded to the body text Node. */
    NodeProps bodyProps;
    /** Props forwarded to the Node that holds the buttons. */
    NodeProps buttonsProps;
    /** Props forwarded to the backdrop Node of the Modal. */
    NodeProps backdropProps;
    /** Props forwarded to the panel Node of the Modal; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a Modal that asks for the user's attention: an optional icon, a
 * title and a body, and a row of buttons under them. Closing it is up to
 * the buttons: the caller stops rendering the alert.
 *
 * @param props Desired alert configuration for this render.
 */
void Alert(AlertProps const& props);

} /* namespace Rocket */
