#include <map>
#include <iostream>
#include <cassert>
#include <functional>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/CADisplayLink.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/String.hpp>
#include <Rocket/Paint/Painter_Private.hpp>
#include <Rocket/Window/Window_MacOS.hpp>

namespace Rocket {

static Key _KeyConvert(std::int32_t);
static std::string _SanitizeKeyInput(NSString*);

} /* namespace Rocket */

/* The content view is the first responder and an NSTextInputClient, so key
   presses go through the input context: dead keys compose ("´" then "e" is
   "é"), the press-and-hold accent picker and input methods commit through
   insertText:, and only the composed text reaches the window's key handler.
   Editing commands the context suggests (doCommandBySelector:) are ignored;
   the document maps them from the key event itself. Marked (in-progress)
   composition text is tracked but not drawn. */
@interface __NSMetalView : NSView<NSTextInputClient>

@property (strong, nonatomic) NSCursor* activeCursor;
@property (copy, nonatomic) void(^onDisplay)(void);
@property (copy, nonatomic) void(^onKeyDown)(NSEvent*, NSString*);

@end

@implementation __NSMetalView {
    CADisplayLink* _displayLink;
    NSTrackingArea* _trackingArea;
    NSMutableString* _composedInput;
    NSString* _markedText;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    PROFILE

    _composedInput = [NSMutableString string];

    [self interpretKeyEvents: @[event]];

    if (_markedText != nil) {
        return; /* composition in progress: the input method owns the keys until it commits */
    }

    if (self.onKeyDown != nil) {
        self.onKeyDown(event, _composedInput);
    }
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    NSString* text = [string isKindOfClass: [NSAttributedString class]] ? [string string] : string;

    if (text != nil) {
        [_composedInput appendString: text];
    }

    _markedText = nil;
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    NSString* text = [string isKindOfClass: [NSAttributedString class]] ? [string string] : string;

    _markedText = (text.length > 0) ? text : nil;
}

- (void)unmarkText {
    _markedText = nil;
}

- (BOOL)hasMarkedText {
    return (_markedText != nil);
}

- (NSRange)markedRange {
    return (_markedText != nil) ? NSMakeRange(0, _markedText.length) : NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange {
    return NSMakeRange(NSNotFound, 0);
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText {
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    /* the candidate window is anchored at the pointer: the view does not know where the caret is */
    NSPoint point = [NSEvent mouseLocation];
    return NSMakeRect(point.x, point.y, 1.0, 1.0);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return NSNotFound;
}

- (void)doCommandBySelector:(SEL)selector {
    /* editing commands are derived from the key event by the document */
}

- (CALayer*)makeBackingLayer {
    CAMetalLayer* layer = [CAMetalLayer layer];

    layer.device = (__bridge id<MTLDevice>)Rocket::__GetDefaultGPUDevice();
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;
    layer.autoresizingMask = (kCALayerWidthSizable | kCALayerHeightSizable);
    layer.needsDisplayOnBoundsChange = YES;

    return layer;
}

- (id)initWithFrame:(NSRect)rect {
    self = [super initWithFrame: rect];

    if (self) {
        self.wantsLayer = YES;
        self.autoresizingMask = (NSViewWidthSizable | NSViewHeightSizable);
        self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
        [self _updateLayerSize];
    }

    return self;
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    [self _updateLayerSize];

    [_displayLink invalidate];
    _displayLink = nil;

    if (self.window != nil) {
        _displayLink = [self displayLinkWithTarget: self selector: @selector(_display:)];
        [_displayLink addToRunLoop: [NSRunLoop mainRunLoop] forMode: NSRunLoopCommonModes];
    }
}

- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];
    [self _updateLayerSize];
}

- (void)setFrameSize:(NSSize)size {
    [super setFrameSize: size];
    [self _updateLayerSize];
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];

    if (_trackingArea != nil) {
        [self removeTrackingArea: _trackingArea];
    }

    /* Enter/exit for the whole content area, in any activation state. The
       owner is the view; NSResponder forwards mouseEntered:/mouseExited: up
       the chain to __NSWindow, which dispatches them. */
    _trackingArea = [[NSTrackingArea alloc]
        initWithRect: NSZeroRect
             options: (NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways | NSTrackingInVisibleRect)
               owner: self
            userInfo: nil];
    [self addTrackingArea: _trackingArea];
}

- (void)resetCursorRects {
    [super resetCursorRects];

    if (self.activeCursor != nil) {
        [self addCursorRect: self.bounds cursor: self.activeCursor];
    }
}

- (void)_updateLayerSize {
    CGFloat scale = (self.window != nil)
        ? self.window.backingScaleFactor
        : 1.0;

    CAMetalLayer* layer = (CAMetalLayer*)self.layer;
    layer.contentsScale = scale;
    layer.drawableSize = ::CGSizeMake(
        (self.bounds.size.width * scale),
        (self.bounds.size.height * scale)
    );
}

- (void)_display:(CADisplayLink*)link {
    if (self.onDisplay != nil) {
        self.onDisplay();
    }
}

@end

@implementation __NSWindow {
    BOOL _leftMouseDown;
    BOOL _rightMouseDown;
}

- (id)init {
    self = [super init];

    if (self) {
        [self setStyleMask: NSWindowStyleMaskTitled];
        [self setDelegate: self];
    }

    return self;
}

- (void)mouseDown:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];

    if (NSPointInRect(point, self.contentView.frame) == false) {
        return;
    }

    if (self->_onMouseDown != nil) {
        self->_onMouseDown(Rocket::Mouse::LeftButton, {
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        }, static_cast<int>(event.clickCount));
        self->_leftMouseDown = YES;
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];

    if (NSPointInRect(point, self.contentView.frame) == false) {
        return;
    }

    if (self->_onMouseDown != nil) {
        self->_onMouseDown(Rocket::Mouse::RightButton, {
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        }, static_cast<int>(event.clickCount));
        self->_rightMouseDown = YES;
    }
}

- (void)otherMouseDown:(NSEvent*)event {
    PROFILE

    /* */
}

- (void)mouseUp:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];

    if (self->_onMouseUp != nil) {
        self->_onMouseUp(Rocket::Mouse::LeftButton, {
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        });
    }

    self->_leftMouseDown = NO;
}

- (void)rightMouseUp:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];

    if (self->_onMouseUp != nil) {
        self->_onMouseUp(Rocket::Mouse::RightButton, {
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        });
    }

    self->_rightMouseDown = NO;
}

- (void)otherMouseUp:(NSEvent*)event {
    PROFILE

    /* */
}

- (void)mouseMoved:(NSEvent*)event {
    PROFILE

    /* mouseMoved only fires with no button held: a still-unpaired down means
       an AppKit gesture (titlebar drag, edge double-click expand, window menu)
       swallowed the up — synthesize it to keep down/up pairing consistent */
    if ([NSEvent pressedMouseButtons] == 0) {
        if (self->_leftMouseDown == YES) {
            [self mouseUp: event];
        }
        if (self->_rightMouseDown == YES) {
            [self rightMouseUp: event];
        }
    }

    NSPoint point = [event locationInWindow];
    if (self->_onMouseMove != nil) {
        self->_onMouseMove({
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        });
    }
}

- (void)mouseEntered:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];
    if (self->_onMouseEnter != nil) {
        self->_onMouseEnter({
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        });
    }


}

- (void)mouseExited:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];
    if (self->_onMouseExit != nil) {
        self->_onMouseExit({
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        });
    }
}

- (void)mouseDragged:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];
    if (self->_onMouseMove != nil) {
        self->_onMouseMove({
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        });
    }
}

- (void)scrollWheel:(NSEvent*)event {
    PROFILE

    auto deltaX = static_cast<float>(event.scrollingDeltaX);
    auto deltaY = static_cast<float>(event.scrollingDeltaY);
    // auto isPrecise = event.hasPreciseScrollingDeltas;

    if (self->_onMouseWheel != nil) {
        self->_onMouseWheel({ deltaX, deltaY }, {
            .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
            .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
            .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
            .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption))
        });
    }
}

- (void)rightMouseDragged:(NSEvent*)event {
    PROFILE

    /* */
}

- (void)otherMouseDragged:(NSEvent*)event {
    PROFILE

    /* */
}

- (void)keyDown:(NSEvent*)event {
    PROFILE

    /* reached only while the content view is not the first responder */
    [self dispatchKeyDown: event input: event.characters];
}

- (void)dispatchKeyDown:(NSEvent*)event input:(NSString*)characters {
    PROFILE

    if (self->_onKeyDown != nil) {
        auto const input = Rocket::_SanitizeKeyInput(characters);

        self->_onKeyDown(
            Rocket::_KeyConvert(event.keyCode),
            { .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
              .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
              .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
              .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption)) },
            input.c_str()
        );
    }
}

- (void)keyUp:(NSEvent*)event {
    PROFILE

    if (self->_onKeyUp != nil) {
        self->_onKeyUp(
            Rocket::_KeyConvert(event.keyCode),
            { .control = static_cast<bool>((event.modifierFlags & NSEventModifierFlagControl)),
              .shift = static_cast<bool>((event.modifierFlags & NSEventModifierFlagShift)),
              .meta = static_cast<bool>((event.modifierFlags & NSEventModifierFlagCommand)),
              .alt = static_cast<bool>((event.modifierFlags & NSEventModifierFlagOption)) },
            (event.characters != NULL)
                ? [event.characters cStringUsingEncoding: NSUTF8StringEncoding]
                : ""
        );
    }
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
    PROFILE

    if (self->_onClose != nil) {
        self->_onClose();
    }
    return false;
}

- (void)windowWillClose:(NSWindow *)sender {
    PROFILE

    if (self->_onHide != nil) {
        self->_onHide();
    }
}

- (void)windowDidResize:(NSNotification*)notification {
    PROFILE

    if (self->_onSize != nil) {
        self->_onSize({
            static_cast<float>(self.contentView.frame.size.width),
            static_cast<float>(self.contentView.frame.size.height)
        });
    }
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    PROFILE

    if (self->_onMinimize != nil) {
        self->_onMinimize();
    }
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    PROFILE

    if (self->_onDeminimize != nil) {
        self->_onDeminimize();
    }
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
    PROFILE

    if (self->_onMaximize != nil) {
        self->_onMaximize();
    }
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
    PROFILE

    if (self->_onDemaximize != nil) {
        self->_onDemaximize();
    }
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
    PROFILE

    if (self->_onPixelRatio != nil) {
        self->_onPixelRatio(self.backingScaleFactor);
    }
    if (self->_onSize != nil) {
        self->_onSize({
            static_cast<float>(self.contentView.frame.size.width),
            static_cast<float>(self.contentView.frame.size.height)
        });
    }
}

@end

namespace Rocket {

static auto const _keyMap = std::map<std::int32_t, Key>({
    { kVK_ANSI_0, Key::Digit0 },
    { kVK_ANSI_1, Key::Digit1 },
    { kVK_ANSI_2, Key::Digit2 },
    { kVK_ANSI_3, Key::Digit3 },
    { kVK_ANSI_4, Key::Digit4 },
    { kVK_ANSI_5, Key::Digit5 },
    { kVK_ANSI_6, Key::Digit6 },
    { kVK_ANSI_7, Key::Digit7 },
    { kVK_ANSI_8, Key::Digit8 },
    { kVK_ANSI_9, Key::Digit9 },
    { kVK_ANSI_A, Key::KeyA },
    { kVK_ANSI_B, Key::KeyB },
    { kVK_ANSI_C, Key::KeyC },
    { kVK_ANSI_D, Key::KeyD },
    { kVK_ANSI_E, Key::KeyE },
    { kVK_ANSI_F, Key::KeyF },
    { kVK_ANSI_G, Key::KeyG },
    { kVK_ANSI_H, Key::KeyH },
    { kVK_ANSI_I, Key::KeyI },
    { kVK_ANSI_J, Key::KeyJ },
    { kVK_ANSI_K, Key::KeyK },
    { kVK_ANSI_L, Key::KeyL },
    { kVK_ANSI_M, Key::KeyM },
    { kVK_ANSI_N, Key::KeyN },
    { kVK_ANSI_O, Key::KeyO },
    { kVK_ANSI_P, Key::KeyP },
    { kVK_ANSI_Q, Key::KeyQ },
    { kVK_ANSI_R, Key::KeyR },
    { kVK_ANSI_S, Key::KeyS },
    { kVK_ANSI_T, Key::KeyT },
    { kVK_ANSI_U, Key::KeyU },
    { kVK_ANSI_V, Key::KeyV },
    { kVK_ANSI_W, Key::KeyW },
    { kVK_ANSI_X, Key::KeyX },
    { kVK_ANSI_Y, Key::KeyY },
    { kVK_ANSI_Z, Key::KeyZ },
    { kVK_F1, Key::F1 },
    { kVK_F2, Key::F2 },
    { kVK_F3, Key::F3 },
    { kVK_F4, Key::F4 },
    { kVK_F5, Key::F5 },
    { kVK_F6, Key::F6 },
    { kVK_F7, Key::F7 },
    { kVK_F8, Key::F8 },
    { kVK_F9, Key::F9 },
    { kVK_F10, Key::F10 },
    { kVK_F11, Key::F11 },
    { kVK_F12, Key::F12 },
    { kVK_F13, Key::F13 },
    { kVK_F14, Key::F14 },
    { kVK_F15, Key::F15 },
    { kVK_F16, Key::F16 },
    { kVK_F17, Key::F17 },
    { kVK_F18, Key::F18 },
    { kVK_F19, Key::F19 },
    { kVK_F20, Key::F20 },
    { kVK_Option, Key::Alt },
    { kVK_DownArrow, Key::ArrowDown },
    { kVK_LeftArrow, Key::ArrowLeft },
    { kVK_RightArrow, Key::ArrowRight },
    { kVK_UpArrow, Key::ArrowUp },
    { kVK_ANSI_Backslash, Key::Backslash },
    { kVK_ANSI_LeftBracket, Key::BracketLeft },
    { kVK_ANSI_RightBracket, Key::BracketRight },
    { kVK_CapsLock, Key::Capslock },
    { kVK_ANSI_Comma, Key::Comma },
    { kVK_Control, Key::Control },
    { kVK_Delete, Key::Backspace },
    { kVK_End, Key::End },
    { kVK_ANSI_Equal, Key::Equal },
    { kVK_Escape, Key::Escape },
    { kVK_ForwardDelete, Key::Delete },
    { kVK_Function, Key::Function },
    { kVK_ANSI_Grave, Key::Backquote },
    { kVK_Help, Key::Help },
    { kVK_Home, Key::Home },
    { kVK_Command, Key::Meta },
    { kVK_ANSI_Minus, Key::Minus },
    { kVK_Mute, Key::Mute },
    { kVK_PageDown, Key::PageDown },
    { kVK_PageUp, Key::PageUp },
    { kVK_ANSI_Period, Key::Period },
    { kVK_ANSI_Quote, Key::Quote },
    { kVK_Return, Key::Enter },
    { kVK_RightOption, Key::AltRight },
    { kVK_RightControl, Key::ControlRight },
    { kVK_RightShift, Key::ShiftRight },
    { kVK_ANSI_Semicolon, Key::Semicolon },
    { kVK_Shift, Key::Shift },
    { kVK_ANSI_Slash, Key::Slash },
    { kVK_Space, Key::Space },
    { kVK_Tab, Key::Tab },
    { kVK_VolumeDown, Key::VolumeDown },
    { kVK_VolumeUp, Key::VolumeUp },
    { kVK_ANSI_KeypadClear, Key::NumpadClear },
    { kVK_ANSI_KeypadDecimal, Key::NumpadDecimal },
    { kVK_ANSI_KeypadDivide, Key::NumpadDivide },
    { kVK_ANSI_KeypadEnter, Key::NumpadEnter },
    { kVK_ANSI_KeypadEquals, Key::NumpadEqual },
    { kVK_ANSI_KeypadMinus, Key::NumpadMinus },
    { kVK_ANSI_KeypadMultiply, Key::NumpadMultiply },
    { kVK_ANSI_KeypadPlus, Key::NumpadAdd },
    { kVK_ANSI_Keypad0, Key::Numpad0 },
    { kVK_ANSI_Keypad1, Key::Numpad1 },
    { kVK_ANSI_Keypad2, Key::Numpad2 },
    { kVK_ANSI_Keypad3, Key::Numpad3 },
    { kVK_ANSI_Keypad4, Key::Numpad4 },
    { kVK_ANSI_Keypad5, Key::Numpad5 },
    { kVK_ANSI_Keypad6, Key::Numpad6 },
    { kVK_ANSI_Keypad7, Key::Numpad7 },
    { kVK_ANSI_Keypad8, Key::Numpad8 },
    { kVK_ANSI_Keypad9, Key::Numpad9 }
});

static Key _KeyConvert(std::int32_t code) {
    PROFILE

    if (_keyMap.find(code) != _keyMap.end()) {
        return _keyMap.at(code);
    } else {
        return Key::Unknown;
    }
}

static std::string _SanitizeKeyInput(NSString* characters) {
    PROFILE

    if (characters == nil) {
        return "";
    }

    auto const cString = [characters cStringUsingEncoding: NSUTF8StringEncoding];

    if (cString == NULL) {
        return "";
    }

    static thread_local auto _codepoints = std::vector<std::uint32_t>();
    static thread_local auto _kept = std::vector<std::uint32_t>();

    StringToCodepoints(cString, _codepoints);

    _kept.clear();

    for (auto codepoint : _codepoints) {
        if (
            (codepoint < 0x20) ||
            (codepoint == 0x7F) ||
            ((codepoint >= 0xF700) && (codepoint <= 0xF8FF))
        ) {
            continue;
        }

        _kept.push_back(codepoint);
    }

    auto string = std::string();

    StringFromCodepoints(_kept, string);

    return string;
}

static NSCursor* _CursorConvert(Cursor cursor) {
    PROFILE

    /* Requires macOS 15: the frame, column, row and zoom cursors are the
       public factories that replaced the deprecated resize* cursors. */
    auto const frame = [](NSCursorFrameResizePosition position) {
        return [NSCursor frameResizeCursorFromPosition: position inDirections: NSCursorFrameResizeDirectionsAll];
    };

    switch (cursor) {
        case Cursor::Default:      return [NSCursor arrowCursor];
        case Cursor::ContextMenu:  return [NSCursor contextualMenuCursor];
        case Cursor::Help:         return [NSCursor arrowCursor];               /* approximation: no public help cursor */
        case Cursor::Pointer:      return [NSCursor pointingHandCursor];
        case Cursor::Progress:     return [NSCursor arrowCursor];               /* approximation: no public busy cursor */
        case Cursor::Wait:         return [NSCursor arrowCursor];               /* approximation: no public wait cursor */
        case Cursor::Cell:         return [NSCursor crosshairCursor];           /* approximation: no public cell cursor */
        case Cursor::Crosshair:    return [NSCursor crosshairCursor];
        case Cursor::Text:         return [NSCursor IBeamCursor];
        case Cursor::VerticalText: return [NSCursor IBeamCursorForVerticalLayout];
        case Cursor::Alias:        return [NSCursor dragLinkCursor];
        case Cursor::Copy:         return [NSCursor dragCopyCursor];
        case Cursor::Move:         return [NSCursor openHandCursor];            /* approximation: no public all-direction move cursor */
        case Cursor::NoDrop:       return [NSCursor operationNotAllowedCursor];
        case Cursor::NotAllowed:   return [NSCursor operationNotAllowedCursor];
        case Cursor::Grab:         return [NSCursor openHandCursor];
        case Cursor::Grabbing:     return [NSCursor closedHandCursor];
        case Cursor::EResize:      return frame(NSCursorFrameResizePositionRight);
        case Cursor::NResize:      return frame(NSCursorFrameResizePositionTop);
        case Cursor::NEResize:     return frame(NSCursorFrameResizePositionTopRight);
        case Cursor::NWResize:     return frame(NSCursorFrameResizePositionTopLeft);
        case Cursor::SResize:      return frame(NSCursorFrameResizePositionBottom);
        case Cursor::SEResize:     return frame(NSCursorFrameResizePositionBottomRight);
        case Cursor::SWResize:     return frame(NSCursorFrameResizePositionBottomLeft);
        case Cursor::WResize:      return frame(NSCursorFrameResizePositionLeft);
        case Cursor::EWResize:     return [NSCursor columnResizeCursorInDirections: NSHorizontalDirectionsAll];
        case Cursor::NSResize:     return [NSCursor rowResizeCursorInDirections: NSVerticalDirectionsAll];
        case Cursor::NESWResize:   return frame(NSCursorFrameResizePositionBottomLeft);
        case Cursor::ColResize:    return [NSCursor columnResizeCursorInDirections: NSHorizontalDirectionsAll];
        case Cursor::RowResize:    return [NSCursor rowResizeCursorInDirections: NSVerticalDirectionsAll];
        case Cursor::AllScroll:    return [NSCursor openHandCursor];            /* approximation: no public all-scroll cursor */
        case Cursor::ZoomIn:       return [NSCursor zoomInCursor];
        case Cursor::ZoomOut:      return [NSCursor zoomOutCursor];
        case Cursor::None:         return nil;
    }

    return [NSCursor arrowCursor];
}

void Window::__done() {
    PROFILE

    if (_impl->cursorHidden) {
        [NSCursor unhide]; /* balance the app-global hide from __setCursor(Cursor::None) */
    }

    [_impl->window setDelegate: nil];
    [_impl->window setContentView: nil]; /* detaching fires viewDidMoveToWindow(nil), which invalidates the display link */
    [_impl->window setReleasedWhenClosed: NO];
    [_impl->window close];
    _impl->window = nil;

    delete _impl;
}

void Window::__init() {
    PROFILE

    _impl = new Window::_Window{
        .window = nullptr,
        .size = { 0.0f, 0.0f },
        .position = { 0.0f, 0.0f },
        .pixelratio = 1.0f,
        .cursorHidden = false,
        .hover = false,
        .maximizable = true
    };

    __NSWindow* window = [[__NSWindow alloc] init];
    __NSMetalView* view = [[__NSMetalView alloc] init];
    __weak __NSWindow* weakWindow = window;

    window.onShow = ^() { _show(); };
    window.onHide = ^() { _hide(); };
    window.onClose = ^() { _close(); };
    window.onMaximize = ^() { _maximize(); };
    window.onMinimize = ^() { _minimize(); };
    window.onDemaximize = ^() { _demaximize(); };
    window.onDeminimize = ^() { _deminimize(); };
    window.onSize = ^(Vec2 size) { _resize(); };
    window.onMouseMove = ^(Vec2 point, KeyModifiers mods) { _mouseMove(point, mods); };
    window.onMouseEnter = ^(Vec2 point, KeyModifiers mods) { _impl->hover = true; _mouseEnter(point, mods); };
    window.onMouseExit = ^(Vec2 point, KeyModifiers mods) { _impl->hover = false; _mouseExit(point, mods); };
    window.onMouseWheel = ^(Vec2 point, KeyModifiers mods) { _mouseWheel(point, mods); };
    window.onMouseDown = ^(Mouse mouse, Vec2 point, KeyModifiers mods, int clickCount) { _mouseDown(mouse, point, mods, clickCount); };
    window.onMouseUp = ^(Mouse mouse, Vec2 point, KeyModifiers mods) { _mouseUp(mouse, point, mods); };
    window.onKeyDown = ^(Key key, KeyModifiers mods, char const* input) { _keyDown(key, mods, std::string(input)); };
    window.onKeyUp = ^(Key key, KeyModifiers mods, char const* input) { _keyUp(key, mods); };
    window.onPixelRatio = ^(double pixelratio) { _dpiChange(); };
    view.onDisplay = ^() { _paint(); };
    view.onKeyDown = ^(NSEvent* event, NSString* input) { [weakWindow dispatchKeyDown: event input: input]; };

    _impl->window = window;
    [_impl->window setContentView: view];
    [_impl->window makeFirstResponder: view];
    [_impl->window setAcceptsMouseMovedEvents: YES];
    [_impl->window setReleasedWhenClosed: NO];
    [_impl->window setIsVisible: NO];
}

Vec2 const& Window::__getSize() const {
    PROFILE

    _impl->size = {
        static_cast<float>(_impl->window.contentView.frame.size.width),
        static_cast<float>(_impl->window.contentView.frame.size.height)
    };

    return _impl->size;
}

float Window::__getScale() const {
    PROFILE

    _impl->pixelratio = static_cast<float>(_impl->window.backingScaleFactor);

    return _impl->pixelratio;
}

void* Window::__getHandle() const {
    PROFILE

    return (__bridge void*)_impl->window;
}

bool Window::__getVisible() const {
    PROFILE

    return _impl->window.visible;
}

bool Window::__getClosable() const {
    PROFILE

    return (_impl->window.styleMask & NSWindowStyleMaskClosable);
}

bool Window::__getSizable() const {
    PROFILE

    return (_impl->window.styleMask & NSWindowStyleMaskResizable);
}

bool Window::__getMaximizable() const {
    PROFILE

    /* Our own flag, not the zoom button's state: AppKit keeps that button
       disabled on non-resizable windows regardless of setEnabled:. */
    return _impl->maximizable;
}

bool Window::__getMinimizable() const {
    PROFILE

    return (_impl->window.styleMask & NSWindowStyleMaskMiniaturizable);
}

bool Window::__getTopmost() const {
    PROFILE

    return (_impl->window.level == NSStatusWindowLevel);
}


bool Window::__isMaximized() const {
    PROFILE

    return (_impl->window.styleMask & NSWindowStyleMaskFullScreen);
}

bool Window::__isMinimized() const {
    PROFILE

    return _impl->window.miniaturized;
}

bool Window::__isHover() const {
    PROFILE

    return _impl->hover;
}

bool Window::__isFocused() const {
    PROFILE

    return [_impl->window isKeyWindow];
}

void Window::__center() const {
    PROFILE

    [_impl->window center];
}

void Window::__paint() const {
    PROFILE

    [_impl->window.contentView setNeedsDisplay: YES];
}

void Window::__setTitle(std::string const& title) {
    PROFILE

    [_impl->window setTitle: [NSString stringWithUTF8String: title.c_str()]];
}

void Window::__setCursor(Cursor cursor) {
    PROFILE

    /* ponytail: [NSCursor hide] is app-global — it hides the cursor for the whole
       application, not just this window; per-window Cursor::None would need
       tracking-area-based hide/unhide on enter/exit. */
    if (cursor == Cursor::None) {
        if (_impl->cursorHidden == false) {
            [NSCursor hide];
            _impl->cursorHidden = true;
        }
    } else if (_impl->cursorHidden) {
        [NSCursor unhide];
        _impl->cursorHidden = false;
    }

    auto view = (__NSMetalView*)_impl->window.contentView;
    view.activeCursor = _CursorConvert(cursor);

    /* Reinstall the cursor rect so the cursor persists while the mouse stays over the window. */
    [_impl->window invalidateCursorRectsForView: view];

    if (_impl->hover && (view.activeCursor != nil)) {
        [view.activeCursor set]; /* apply immediately; the cursor rect only kicks in on the next mouse move */
    }
}

/* The screen whose coordinate space position values are expressed in: the
   window's own screen, or the primary screen while it is off-screen/hidden. */
static NSScreen* _PositionScreen(NSWindow* window) {
    PROFILE

    return (window.screen != nil) ? window.screen : [NSScreen screens].firstObject;
}

Vec2 const& Window::__getPosition() const {
    PROFILE

    auto const frame = _impl->window.frame;
    auto const screen = _PositionScreen(_impl->window);
    auto const screenFrame = (screen != nil) ? screen.frame : NSZeroRect;

    /* AppKit measures from the bottom-left; flip to a top-left origin. */
    _impl->position = {
        static_cast<float>(frame.origin.x - screenFrame.origin.x),
        static_cast<float>((screenFrame.origin.y + screenFrame.size.height) - (frame.origin.y + frame.size.height))
    };

    return _impl->position;
}

void Window::__setPosition(Vec2 const& position) {
    PROFILE

    auto const frame = _impl->window.frame;
    auto const screen = _PositionScreen(_impl->window);
    auto const screenFrame = (screen != nil) ? screen.frame : NSZeroRect;

    [_impl->window setFrameOrigin: NSMakePoint(
        (screenFrame.origin.x + position.x),
        ((screenFrame.origin.y + screenFrame.size.height) - position.y - frame.size.height)
    )];
}

void Window::__setSize(Vec2 const& size) {
    PROFILE

    NSRect rect = _impl->window.frame;
    rect.size.width = size.width;
    rect.size.height = size.height + (_impl->window.frame.size.height
                                     - [_impl->window contentRectForFrameRect:
                                         _impl->window.frame].size.height);
    [_impl->window setFrame: rect display: YES];
}

void Window::__setVisible(bool visible) {
    PROFILE

    if (visible) {
        if (!_impl->window.visible) {
            [_impl->window makeKeyAndOrderFront: nil];
            [_impl->window makeFirstResponder: _impl->window.contentView];
            _show();
        }
    } else {
        if (_impl->window.visible) {
            [_impl->window close]; /* windowWillClose: dispatches _hide() */
        }
    }
}

void Window::__setClosable(bool closable) {
    PROFILE

    if (closable != (bool)(_impl->window.styleMask & NSWindowStyleMaskClosable)) {
        if (closable) {
            _impl->window.styleMask |= NSWindowStyleMaskClosable;
        } else {
            _impl->window.styleMask &= ~NSWindowStyleMaskClosable;
        }
    }
}

void Window::__setSizable(bool sizable) {
    PROFILE

    if (sizable != (bool)(_impl->window.styleMask & NSWindowStyleMaskResizable)) {
        if (sizable) {
            _impl->window.styleMask |= NSWindowStyleMaskResizable;
        } else {
            _impl->window.styleMask &= ~NSWindowStyleMaskResizable;
        }

        /* AppKit re-evaluates the zoom button on a style change; reassert ours. */
        [[_impl->window standardWindowButton: NSWindowZoomButton] setEnabled: _impl->maximizable];
    }
}

void Window::__setMaximizable(bool maximizable) {
    PROFILE

    _impl->maximizable = maximizable;
    [[_impl->window standardWindowButton: NSWindowZoomButton] setEnabled: maximizable];
}

void Window::__setMinimizable(bool minimizable) {
    PROFILE

    if (minimizable != (bool)(_impl->window.styleMask & NSWindowStyleMaskMiniaturizable)) {
        if (minimizable) {
            _impl->window.styleMask |= NSWindowStyleMaskMiniaturizable;
        } else {
            _impl->window.styleMask &= ~NSWindowStyleMaskMiniaturizable;
        }
    }
}

void Window::__setMaximized(bool maximized) {
    PROFILE

    if (maximized != (bool)(_impl->window.styleMask & NSWindowStyleMaskFullScreen)) {
        [_impl->window toggleFullScreen: _impl->window];
    }
}

void Window::__setMinimized(bool minimized) {
    PROFILE

    if (minimized != _impl->window.miniaturized) {
        if (minimized) {
            [_impl->window miniaturize: nil];
        } else {
            [_impl->window deminiaturize: nil];
        }
    }
}

void Window::__setTopmost(bool topmost) {
    PROFILE

    [_impl->window setLevel: topmost ? NSStatusWindowLevel : NSNormalWindowLevel];
}


void Window::__setFocus() {
    PROFILE

    [_impl->window makeKeyAndOrderFront: nullptr];
}

void Window::__runModal() {
    PROFILE

    [NSApp runModalForWindow: _impl->window];
}

void Window::__stopModal() {
    PROFILE

    [NSApp stopModal];
}

} /* namespace Rocket */
