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

/** Color family of a callout. */
enum class CalloutIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/**
 * Everything Callout takes from the theme: a list of rules, resolved as
 * those of ButtonTheme are.
 */
struct CalloutTheme {
    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the callout. */
        std::optional<CalloutIntent> intent;
    };

    /**
     * What a rule sets; an unset field is left to other rules. A callout
     * whose resolved style has a material kind is drawn with the theme's
     * material, otherwise with the plain colors.
     */
    struct Style {
        /** Kind of the material the callout is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material the callout is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material the callout is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Corner roundness of the material the callout is drawn with (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Fill color, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Corner radius in points, without a material (default 0). */
        std::optional<float> borderRadius;
        /** Color of the content and the icon. */
        std::optional<Vec4> color;
        /** Font size of the content. */
        std::optional<float> fontSize;
        /** Padding as left, top, right, bottom, inside the edge (default 0). */
        std::optional<Vec4> padding;
        /** Gap between the icon and the content (default 0). */
        std::optional<float> gap;
        /** Icon shown when the callout shows one and sets none of its own. */
        std::optional<SvgIconData> icon;
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

/** Props for the Callout component. */
struct CalloutProps {
    /** Reconciliation key. */
    std::string key;
    /** Color family (default Default). */
    std::optional<CalloutIntent> intent;
    /** Shows an icon before the content (default false). */
    std::optional<bool> showIcon;
    /** Icon shown when `showIcon` is set; unset shows the theme's icon for the intent. */
    std::optional<IconData> icon;
    /** Props forwarded to the Icon. */
    IconProps iconProps;
    /** Props forwarded to the content Node that holds `children`. */
    NodeProps contentProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a box tinted by its intent that holds `children`, with an
 * optional icon before them, to point something out on a page.
 *
 * @param props    Desired callout configuration for this render.
 * @param children Rendered inside the callout's content Node; default no-op.
 */
void Callout(CalloutProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
