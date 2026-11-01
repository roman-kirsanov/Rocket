/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace Rocket {

/**
 * Mouse cursor shape shown by a window.
 *
 * The values follow the CSS cursor keywords. Platforms without a native
 * equivalent for a value substitute the closest available system cursor (on
 * macOS 15 and later, Help, Progress and Wait fall back to the plain arrow,
 * Cell to the crosshair, and Move and AllScroll to the open hand; every
 * resize and zoom value has a native cursor; on SDL3 and Win32 the
 * substitutes are listed in Window_SDL3.cpp and Window_Win32.cpp). None hides
 * the cursor entirely.
 */
enum class Cursor {
    /** The platform default cursor, usually an arrow. */
    Default,

    /** A context menu is available. */
    ContextMenu,

    /** Help is available. */
    Help,

    /** A link: a pointing hand. */
    Pointer,

    /** The program is busy, but the user can still interact. */
    Progress,

    /** The program is busy and the user cannot interact. */
    Wait,

    /** A table cell can be selected. */
    Cell,

    /** A crosshair, for precise selection. */
    Crosshair,

    /** Text can be selected: an I-beam. */
    Text,

    /** Vertical text can be selected. */
    VerticalText,

    /** An alias or shortcut is to be created. */
    Alias,

    /** Something is to be copied. */
    Copy,

    /** Something is to be moved. */
    Move,

    /** The item cannot be dropped here. */
    NoDrop,

    /** The action is not allowed. */
    NotAllowed,

    /** Something can be grabbed: an open hand. */
    Grab,

    /** Something is being grabbed: a closed hand. */
    Grabbing,

    /** The east edge is to be resized. */
    EResize,

    /** The north edge is to be resized. */
    NResize,

    /** The north-east corner is to be resized. */
    NEResize,

    /** The north-west corner is to be resized. */
    NWResize,

    /** The south edge is to be resized. */
    SResize,

    /** The south-east corner is to be resized. */
    SEResize,

    /** The south-west corner is to be resized. */
    SWResize,

    /** The west edge is to be resized. */
    WResize,

    /** Resizing east or west. */
    EWResize,

    /** Resizing north or south. */
    NSResize,

    /** Resizing north-east or south-west. */
    NESWResize,

    /** A column is to be resized horizontally. */
    ColResize,

    /** A row is to be resized vertically. */
    RowResize,

    /** Scrolling in any direction. */
    AllScroll,

    /** Something can be zoomed in. */
    ZoomIn,

    /** Something can be zoomed out. */
    ZoomOut,

    /** No cursor is shown. */
    None
};

} /* namespace Rocket */
