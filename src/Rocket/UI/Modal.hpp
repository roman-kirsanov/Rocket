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

/**
 * Everything Modal takes from the theme: a list of rules, resolved as those
 * of ButtonTheme are.
 */
struct ModalTheme {
    /** What a rule matches; a modal has nothing to tell one from another, so every rule matches. */
    struct Selector {};

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
        /** Corner roundness of the material the panel is drawn with (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Fill color of the panel, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color of the panel, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width of the panel in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Corner radius of the panel in points, without a material (default 0). */
        std::optional<float> borderRadius;
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
        /** Beats every rule that is not important. */
        bool important = false;
    };

    /** The rules, in order. */
    std::vector<Rule> rules;
};

/** Props for the Modal component. */
struct ModalProps {
    /** Reconciliation key. */
    std::string key;
    /** Props forwarded to the backdrop Node laid over the window. */
    NodeProps backdropProps;
    /** Props forwarded to the panel Node that holds `children`; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a backdrop over the whole window, above the rest of the document,
 * with a panel in its center that holds `children`. The backdrop takes the
 * presses meant for what is under it.
 *
 * @param props    Desired modal configuration for this render.
 * @param children Rendered inside the panel; default no-op.
 */
void Modal(ModalProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
