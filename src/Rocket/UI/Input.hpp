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
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Paint/Shadow.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Color family of an input. */
enum class InputIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/** Size of an input. */
enum class InputSize {
    Medium,
    Small,
    Large
};

/**
 * Everything Input takes from the theme: a list of rules, resolved as those
 * of ButtonTheme are.
 */
struct InputTheme {
    /** State of the input a rule can match. */
    enum class State {
        Normal,
        Hover,
        /** The input has the keyboard focus; it wins over Hover. */
        Focus,
        Disabled
    };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Intent of the input. */
        std::optional<InputIntent> intent;
        /** Size of the input. */
        std::optional<InputSize> size;
        /** State of the input. */
        std::optional<State> state;
    };

    /**
     * What a rule sets; an unset field is left to other rules. An input
     * whose resolved style has a material kind is drawn with the theme's
     * material, otherwise with the plain colors.
     */
    struct Style {
        /** Kind of the material the input is drawn with. */
        std::optional<MaterialKind> materialKind;
        /** Intent of the material the input is drawn with (default Default). */
        std::optional<MaterialIntent> materialIntent;
        /** Brightness of the material the input is drawn with (default Normal). */
        std::optional<MaterialBrightness> materialBrightness;
        /** Corner roundness of the material the input is drawn with (default None). */
        std::optional<MaterialRoundness> materialRoundness;
        /** Fill color, without a material (default none). */
        std::optional<Vec4> background;
        /** Edge color, without a material (default none). */
        std::optional<Vec4> borderColor;
        /** Edge width in points, without a material (default 0). */
        std::optional<float> borderWidth;
        /** Corner radius in points, without a material (default 0). */
        std::optional<float> borderRadius;
        /** Shadow (default none). */
        std::optional<Shadow> shadow;
        /** Color of the text. */
        std::optional<Vec4> color;
        /** Color of the placeholder. */
        std::optional<Vec4> placeholderColor;
        /** Font size of the text and the placeholder. */
        std::optional<float> fontSize;
        /** Padding around the text as left, top, right, bottom (default 0). */
        std::optional<Vec4> padding;
        /** Minimum height in points. */
        std::optional<float> minHeight;
        /** Gap between the prefix, the text and the suffix (default 0). */
        std::optional<float> gap;
        /** Opacity of the whole input. */
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

/** Props for the Input component. */
struct InputProps {
    /** Reconciliation key. */
    std::string key;
    /** Text of the input (default empty). It is shown again whenever it changes; edits are passed to `onChange`. */
    std::optional<std::string> value;
    /** Text shown while the input is empty; unset shows none. */
    std::optional<std::string> placeholder;
    /** Color family (default Default). */
    std::optional<InputIntent> intent;
    /** Size (default Medium). */
    std::optional<InputSize> size;
    /** Fades the input and ignores the pointer and the keyboard (default false). */
    std::optional<bool> disabled;
    /** Shows bullets in place of the text, and does not let it be copied (default false). */
    std::optional<bool> password;
    /** Most characters typed or pasted text may bring the text to (default no limit). */
    std::optional<std::size_t> maxLength;
    /** Takes the keyboard focus when it is first shown (default false). */
    std::optional<bool> focus;
    /** Invoked with the whole new text after each edit. */
    std::function<void(std::string const&)> onChange;
    /** Invoked when Enter is pressed in the input. */
    std::function<void()> onEnter;
    /** Renders content before the text, inside the edge. */
    std::function<void()> prefix;
    /** Renders content after the text, inside the edge. */
    std::function<void()> suffix;
    /** Props forwarded to the editable Node that holds the text. */
    NodeProps fieldProps;
    /** Props forwarded to the text Node. */
    NodeProps textProps;
    /** Props forwarded to the placeholder's text Node. */
    NodeProps placeholderProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * Renders a one-line text field, with an optional prefix and suffix inside
 * its edge. The caret, the selection, the editing keys, the clipboard and
 * undo are those of an editable Node. Each edit calls `onChange` with the
 * whole text; Enter calls `onEnter`. A press anywhere on the input focuses
 * the text.
 *
 * @param props Desired input configuration for this render.
 */
void Input(InputProps const& props);

} /* namespace Rocket */
