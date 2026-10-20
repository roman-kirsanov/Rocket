/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Cocoa/Cocoa.h>
#include <Rocket/Window/Window.hpp>

@interface __NSWindow : NSWindow<NSWindowDelegate>

@property (copy, nonatomic) void(^onShow)(void);
@property (copy, nonatomic) void(^onHide)(void);
@property (copy, nonatomic) void(^onClose)(void);
@property (copy, nonatomic) void(^onMaximize)(void);
@property (copy, nonatomic) void(^onMinimize)(void);
@property (copy, nonatomic) void(^onDemaximize)(void);
@property (copy, nonatomic) void(^onDeminimize)(void);
@property (copy, nonatomic) void(^onSize)(Rocket::Vec2);
@property (copy, nonatomic) void(^onMouseMove)(Rocket::Vec2, Rocket::KeyModifiers);
@property (copy, nonatomic) void(^onMouseEnter)(Rocket::Vec2, Rocket::KeyModifiers);
@property (copy, nonatomic) void(^onMouseExit)(Rocket::Vec2, Rocket::KeyModifiers);
@property (copy, nonatomic) void(^onMouseWheel)(Rocket::Vec2, Rocket::KeyModifiers);
@property (copy, nonatomic) void(^onMouseDown)(Rocket::Mouse, Rocket::Vec2, Rocket::KeyModifiers, int);
@property (copy, nonatomic) void(^onMouseUp)(Rocket::Mouse, Rocket::Vec2, Rocket::KeyModifiers);
@property (copy, nonatomic) void(^onKeyDown)(Rocket::Scancode, char const*, Rocket::KeyModifiers, bool);
@property (copy, nonatomic) void(^onKeyUp)(Rocket::Scancode, char const*, Rocket::KeyModifiers);
@property (copy, nonatomic) void(^onPixelRatio)(double);
@property (copy, nonatomic) void(^onFocus)(void);
@property (copy, nonatomic) void(^onBlur)(void);

/** The key's text, if any, is published separately, see __NSMetalView. */
- (void)dispatchKeyDown:(NSEvent*)event;

@end

namespace Rocket {

struct Window::_Window {
    __NSWindow* window;
    Vec2 size;
    Vec2 position;
    float pixelratio;
    bool cursorHidden;
    bool hover;
    bool maximizable;
};

} /* namespace Rocket */
