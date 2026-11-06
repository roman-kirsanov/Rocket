/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
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

/** Color family of a select. */
enum class SelectIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/** Size of a select. */
enum class SelectSize {
    Medium,
    Small,
    Large
};

/**
 * Everything Select takes from the theme: a list of rules, resolved as those
 * of ButtonTheme are. The search field is an Input, drawn without its own
 * edge and fill.
 */
struct SelectTheme {
    /** State of the select a rule can match. */
    enum class State {
        Normal,
        Hover,
        /** The select has the keyboard focus or its list is open; it wins over Hover. */
        Focus,
        Disabled
    };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the select. */
        std::optional<SelectIntent> intent;
        /** Size of the select. */
        std::optional<SelectSize> size;
        /** State of the select. */
        std::optional<State> state;
    };

    /**
     * What a rule sets; an unset field is left to other rules. The field,
     * and apart from it the list, is drawn with the theme's material when
     * its resolved style has a material kind, otherwise with the plain
     * colors.
     */
    struct Style {
        /** Kind of the material the field is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material the field is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material the field is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Corner roundness of the material the field is drawn with (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Fill color of the field, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color of the field, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width of the field in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Corner radius of the field in points, without a material (default 0). */
        std::optional<float> borderRadius;
        /** Shadow of the field (default none). */
        std::optional<Shadow> shadow;
        /** Color of the text. */
        std::optional<Vec4> color;
        /** Color of the placeholder. */
        std::optional<Vec4> placeholderColor;
        /** Font size of the field and the list. */
        std::optional<float> fontSize;
        /** Padding of the field as left, top, right, bottom (default 0). */
        std::optional<Vec4> padding;
        /** Minimum height of the field in points. */
        std::optional<float> minHeight;
        /** Gap between the prefix, the text, the caret and the suffix (default 0). */
        std::optional<float> gap;
        /** Opacity of the whole select. */
        std::optional<float> opacity;
        /** Mark at the end of the field that the list opens from; unset shows none. */
        std::optional<SvgIconData> caret;
        /** Mark shown in place of the caret while the list is searched; unset shows the caret. */
        std::optional<SvgIconData> searchIcon;
        /** Color of the caret and the search mark. */
        std::optional<Vec4> caretColor;
        /** Size of the caret and the search mark in points. */
        std::optional<Vec2> caretSize;
        /** Kind of the material the list is drawn with. */
        std::optional<MaterialKind> listMaterialKind;
        /** Intent of the material the list is drawn with (default Default). */
        std::optional<MaterialIntent> listMaterialIntent;
        /** Corner roundness of the material the list is drawn with (default None). */
        std::optional<MaterialRoundness> listMaterialRoundness;
        /** Fill color of the list, without a material (default none). */
        std::optional<Vec4> listBackground;
        /** Edge color of the list, without a material (default none). */
        std::optional<Vec4> listBorderColor;
        /** Edge width of the list in points, without a material (default 0). */
        std::optional<float> listBorderWidth;
        /** Corner radius of the list in points, without a material (default 0). */
        std::optional<float> listBorderRadius;
        /** Shadow of the list (default none). */
        std::optional<Shadow> listShadow;
        /** Padding of the list as left, top, right, bottom (default 0). */
        std::optional<Vec4> listPadding;
        /** Gap in points between the field and the list (default 0). */
        std::optional<float> listGap;
        /** Tallest the list may be, in points, before it scrolls (default none). */
        std::optional<float> listMaxHeight;
        /** Padding of an option as left, top, right, bottom (default 0). */
        std::optional<Vec4> optionPadding;
        /** Corner radius of an option in points (default 0). */
        std::optional<float> optionBorderRadius;
        /** Color of an option's text. */
        std::optional<Vec4> optionColor;
        /** Fill of the option under the pointer or the arrow keys (default none). */
        std::optional<Vec4> optionHighlightBackground;
        /** Text color of the option under the pointer or the arrow keys; unset keeps `optionColor`. */
        std::optional<Vec4> optionHighlightColor;
        /** Fill of the selected option (default none). */
        std::optional<Vec4> optionSelectedBackground;
        /** Text color of the selected option; unset keeps `optionColor`. */
        std::optional<Vec4> optionSelectedColor;
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

/** Props for the Select component. */
struct SelectProps {
    /** Reconciliation key. */
    std::string key;
    /** Labels of the options, top to bottom (default none). */
    std::optional<std::vector<std::string>> options;
    /** Index of the selected option; unset shows the placeholder. */
    std::optional<std::size_t> selected;
    /** Text shown while no option is selected; unset shows none. */
    std::optional<std::string> placeholder;
    /** Color family (default Default). */
    std::optional<SelectIntent> intent;
    /** Size (default Medium). */
    std::optional<SelectSize> size;
    /** Fades the select and ignores the pointer and the keyboard (default false). */
    std::optional<bool> disabled;
    /** While the list is open, the field becomes a search box that narrows the options to those whose label holds the search, in any case (default false). */
    std::optional<bool> searchable;
    /** Text shown in the empty search box; unset shows none. */
    std::optional<std::string> searchPlaceholder;
    /** Invoked with the index of an option, in `options`, when it is picked. */
    std::function<void(std::size_t)> onChange;
    /** Renders content before the text, inside the field's edge. */
    std::function<void()> prefix;
    /** Renders content after the caret, inside the field's edge. */
    std::function<void()> suffix;
    /** Renders content pinned above the options in the list. */
    std::function<void()> header;
    /** Renders content pinned below the options in the list. */
    std::function<void()> footer;
    /** Props forwarded to the text Node of the field. */
    NodeProps textProps;
    /** Props forwarded to the Popup's Node of the list; any field set here overrides the computed style. */
    NodeProps listProps;
    /** Props forwarded to the field Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a field that shows the selected option; a click on it, or Enter,
 * Space or Down while it has the focus, opens a list of the options under
 * it, as wide as the field. In the list, Up and Down move the highlight,
 * Enter picks the highlighted option and Escape closes it; a click picks an
 * option. Picking one calls `onChange` and closes the list, as does a press
 * outside the select. The select keeps no selection of its own.
 *
 * @param props Desired select configuration for this render.
 */
void Select(SelectProps const& props);

} /* namespace Rocket */
