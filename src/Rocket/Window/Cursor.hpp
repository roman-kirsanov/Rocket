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
 * macOS, Help, Progress, Wait, ZoomIn and ZoomOut fall back to the plain
 * arrow). None hides the cursor entirely.
 */
enum class Cursor {
    Default,
    ContextMenu,
    Help,
    Pointer,
    Progress,
    Wait,
    Cell,
    Crosshair,
    Text,
    VerticalText,
    Alias,
    Copy,
    Move,
    NoDrop,
    NotAllowed,
    Grab,
    Grabbing,
    EResize,
    NResize,
    NEResize,
    NWResize,
    SResize,
    SEResize,
    SWResize,
    WResize,
    EWResize,
    NSResize,
    NESWResize,
    ColResize,
    RowResize,
    AllScroll,
    ZoomIn,
    ZoomOut,
    /** No cursor is shown. */
    None
};

} /* namespace Rocket */
