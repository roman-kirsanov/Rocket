/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/UI/Alert.hpp>
#include <Rocket/UI/Button.hpp>
#include <Rocket/UI/Callout.hpp>
#include <Rocket/UI/Checkbox.hpp>
#include <Rocket/UI/Drawer.hpp>
#include <Rocket/UI/Icon.hpp>
#include <Rocket/UI/Input.hpp>
#include <Rocket/UI/Label.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Modal.hpp>
#include <Rocket/UI/Radiobox.hpp>
#include <Rocket/UI/Select.hpp>
#include <Rocket/UI/Spinner.hpp>
#include <Rocket/UI/Switch.hpp>
#include <Rocket/UI/Tooltip.hpp>

namespace Rocket {

/**
 * The look of every component: the material the material appearances are
 * drawn with, and one member per component. The library's own themes are in
 * src/Rocket/UI/Theme/<Name>.
 */
struct Theme {
    /** Material the material appearances are drawn with; it must outlive the theme. Null in a theme that has no material appearance. */
    Material const* material;
    /** Theme of Button. */
    ButtonTheme button;
    /** Theme of Icon. */
    IconTheme icon;
    /** Theme of Checkbox. */
    CheckboxTheme checkbox;
    /** Theme of Radiobox. */
    RadioboxTheme radiobox;
    /** Theme of Switch; `switch` is a keyword of C++. */
    SwitchTheme switch_;
    /** Theme of Callout. */
    CalloutTheme callout;
    /** Theme of Spinner. */
    SpinnerTheme spinner;
    /** Theme of Modal. */
    ModalTheme modal;
    /** Theme of Alert. */
    AlertTheme alert;
    /** Theme of Drawer. */
    DrawerTheme drawer;
    /** Theme of Tooltip. */
    TooltipTheme tooltip;
    /** Theme of Label. */
    LabelTheme label;
    /** Theme of Input. */
    InputTheme input;
    /** Theme of Select. */
    SelectTheme select;
};

/**
 * Returns the theme provided by the nearest Theme context, or the default
 * dark theme (see GetDefaultDarkTheme) when there is none. Must be called during a
 * render.
 *
 * A theme is provided with Context or SetContext, const or not, on a Theme
 * that outlives the render and is never changed while provided; to switch
 * themes, provide another one.
 */
Theme const& UseTheme();

} /* namespace Rocket */
