/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <Rocket/Base/Enum.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Color family of an icon. */
enum class IconIntent {
    Default,
    /** The default color, half transparent. */
    Secondary,
    Primary,
    Success,
    Warning,
    Danger
};

/** Size of an icon. */
enum class IconSize {
    Medium,
    Small,
    Large
};

/** An icon drawn from SVG text, colored by the icon's intent or color. */
struct SvgIconData {
    /** The SVG text; it must outlive the icon and not change, as an image does for ImageIconData. */
    std::string_view data;
};

/** An icon drawn from an image, as it is. */
struct ImageIconData {
    /** The image; it must outlive the icon. */
    Image const& image;
    /** Sampling filter the image is drawn with. */
    ImageFilter filter;
};

/** What an icon draws: SVG text or an image. */
using IconData = Enum<SvgIconData, ImageIconData>;

/**
 * Everything Icon takes from the theme: a list of rules, resolved as those of
 * ButtonTheme are.
 */
struct IconTheme {
    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the icon. */
        std::optional<IconIntent> intent;
        /** Size of the icon. */
        std::optional<IconSize> size;
        /** Whether the icon is disabled. */
        std::optional<bool> disabled;
    };

    /** What a rule sets; an unset field is left to other rules. */
    struct Style {
        /** Color of an SVG icon. */
        std::optional<Vec4> color;
        /** Size in points. */
        std::optional<Vec2> size;
        /** Alpha the color of an SVG icon is drawn with, replacing its own. */
        std::optional<float> alpha;
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

/** Props for the Icon component. */
struct IconProps {
    /** Reconciliation key. */
    std::string key;
    /** What the icon draws; unset draws nothing. */
    std::optional<IconData> data;
    /** Color family of an SVG icon (default Default). */
    std::optional<IconIntent> intent;
    /** Size of the icon (default Medium). */
    std::optional<IconSize> size;
    /** Draws an SVG icon faded (default false). */
    std::optional<bool> disabled;
    /** Color of an SVG icon; overrides the intent. */
    std::optional<Vec4> colorValue;
    /** Size in points; overrides `size`. */
    std::optional<Vec2> sizeValue;
    /** Props forwarded to the Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders an icon from SVG text or an image, stretched over a node of the
 * icon's size. SVG text is rasterized at the document's scale in the
 * icon's color, and kept for every icon drawn alike.
 *
 * @param props Desired icon configuration for this render.
 */
void Icon(IconProps const& props);

} /* namespace Rocket */
