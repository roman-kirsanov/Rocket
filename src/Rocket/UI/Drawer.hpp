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
#include <Rocket/Paint/Shadow.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Edge of the window a drawer is placed against. */
enum class DrawerOrigin {
    Left,
    Top,
    Right,
    Bottom
};

/**
 * Everything Drawer takes from the theme: a list of rules, resolved as those
 * of ButtonTheme are.
 */
struct DrawerTheme {
    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Edge the drawer is placed against. */
        std::optional<DrawerOrigin> origin;
    };

    /**
     * What a rule sets; an unset field is left to other rules. A panel whose
     * resolved style has a material kind is drawn with the theme's material,
     * otherwise with the plain colors.
     */
    struct Style {
        /** Color of the backdrop laid over the window (default none). */
        std::optional<Vec4> backdrop;
        /** Kind of the material the panel is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material the panel is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material the panel is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Fill color of the panel, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color of the panel, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Width in points of the panel's one edge that faces the window, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Shadow of the panel (default none). */
        std::optional<Shadow> shadow;
        /** Color of the content. */
        std::optional<Vec4> color;
        /** Font size of the content. */
        std::optional<float> fontSize;
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

/** Props for the Drawer component. */
struct DrawerProps {
    /** Reconciliation key. */
    std::string key;
    /** Edge of the window the drawer is placed against (default Left). */
    std::optional<DrawerOrigin> origin;
    /** Shows the drawer (default false). */
    std::optional<bool> visible;
    /** Invoked when a press lands on the backdrop, outside the panel. */
    std::function<void()> onClose;
    /** Props forwarded to the backdrop Node laid over the window. */
    NodeProps backdropProps;
    /** Props forwarded to the panel Node that holds `children`; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders, while `visible`, a backdrop over the whole window, above the rest
 * of the document, and a panel against one edge of it that holds
 * `children`, as high or wide as the window. A press on the backdrop calls
 * `onClose`; the drawer keeps no visibility of its own. It appears and
 * disappears at once: it does not slide yet.
 *
 * @param props    Desired drawer configuration for this render.
 * @param children Rendered inside the panel; default no-op.
 */
void Drawer(DrawerProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
