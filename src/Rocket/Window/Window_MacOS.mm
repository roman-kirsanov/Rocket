#include <map>
#include <optional>
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

static Scancode _KeyConvert(std::int32_t);
static KeyModifiers _ModifiersConvert(NSEventModifierFlags);
static std::string _GetKeycode(NSEvent*, Scancode);
static std::string _SanitizeInput(NSString*);

} /* namespace Rocket */

/* The content view is the first responder and an NSTextInputClient, so key
   presses go through the input context: dead keys compose ("´" then "e" is
   "é"), the press-and-hold accent picker and input methods commit through
   insertText:. Committed text is published as input and marked (in-progress)
   text as composition, both after the key event that produced them; text
   that arrives without a key (emoji picker, dictation) is published at once.
   The text system only runs while an input area is set (see
   Window::setInputArea); otherwise keys are plain key events. While an input
   method composes, it owns the keys: they are not dispatched as key events.
   Editing commands the context suggests (doCommandBySelector:) are ignored;
   the document maps them from the key event itself. */
@interface __NSMetalView : NSView<NSTextInputClient>

@property (strong, nonatomic) NSCursor* activeCursor;
@property (copy, nonatomic) void(^onDisplay)(void);
@property (copy, nonatomic) void(^onKeyDown)(NSEvent*);
@property (copy, nonatomic) void(^onInput)(NSString*);
@property (copy, nonatomic) void(^onComposition)(NSString*, NSInteger, NSInteger);

/** The caret rectangle in view points with a top-left origin; nil keeps the text system off (see Window::setInputArea). */
@property (strong, nonatomic) NSValue* inputArea;

@end

@implementation __NSMetalView {
    CADisplayLink* _displayLink;
    NSTrackingArea* _trackingArea;
    NSMutableString* _composedInput;
    NSString* _markedText;
    NSRange _markedSelection;
    BOOL _handlingKey;
    BOOL _compositionChanged;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    PROFILE

    if (self.inputArea == nil) {
        /* no text field: keys are plain key events */
        if (self.onKeyDown != nil) {
            self.onKeyDown(event);
        }
        return;
    }

    _composedInput = [NSMutableString string];
    _compositionChanged = NO;
    _handlingKey = YES;

    [self interpretKeyEvents: @[event]];

    _handlingKey = NO;

    /* composition in progress: the input method owns the keys until it commits */
    if (_markedText == nil && self.onKeyDown != nil) {
        self.onKeyDown(event);
    }

    if (_compositionChanged) {
        [self publishComposition];
    }

    if (_composedInput.length > 0 && self.onInput != nil) {
        self.onInput(_composedInput);
    }
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    NSString* text = [string isKindOfClass: [NSAttributedString class]] ? [string string] : string;

    if (_markedText != nil) {
        _markedText = nil;
        [self compositionDidChange];
    }

    if (text.length == 0) {
        return;
    }

    if (_handlingKey) {
        [_composedInput appendString: text];
    } else if (self.onInput != nil) {
        self.onInput(text); /* no key event: the emoji picker, dictation */
    }
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    NSString* text = [string isKindOfClass: [NSAttributedString class]] ? [string string] : string;

    if (text.length == 0 && _markedText == nil) {
        return;
    }

    _markedText = (text.length > 0) ? [text copy] : nil;
    _markedSelection = selectedRange;
    [self compositionDidChange];
}

- (void)unmarkText {
    if (_markedText != nil) {
        _markedText = nil;
        [self compositionDidChange];
    }
}

- (void)compositionDidChange {
    if (_handlingKey) {
        _compositionChanged = YES; /* published after the key event */
    } else {
        [self publishComposition];
    }
}

- (void)publishComposition {
    if (self.onComposition == nil) {
        return;
    }

    /* AppKit ranges count UTF-16 units; events count characters */
    auto const characters = [](NSString* string) {
        return static_cast<NSInteger>([string lengthOfBytesUsingEncoding: NSUTF32StringEncoding] / 4);
    };

    NSString* text = (_markedText != nil) ? _markedText : @"";
    NSUInteger const start = MIN(_markedSelection.location, text.length);
    NSUInteger const end = MIN((start + _markedSelection.length), text.length);

    self.onComposition(
        text,
        characters([text substringToIndex: start]),
        characters([text substringWithRange: NSMakeRange(start, (end - start))])
    );
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
    if (self.inputArea != nil && self.window != nil) {
        NSRect const area = self.inputArea.rectValue;
        NSRect const local = NSMakeRect(area.origin.x, (self.bounds.size.height - area.origin.y - area.size.height), area.size.width, area.size.height);
        return [self.window convertRectToScreen: [self convertRect: local toView: nil]];
    }

    /* no caret known: anchor the candidate window at the pointer */
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
    std::optional<Rocket::Vec2> _mousePosition;
}

- (void)_dispatchMouseMove:(NSPoint)point flags:(NSEventModifierFlags)flags {
    PROFILE

    auto position = Rocket::Vec2{
        static_cast<float>(point.x),
        static_cast<float>(self.contentView.frame.size.height - point.y)
    };

    if (self->_onMouseMove != nil) {
        self->_mousePosition = position;
        self->_onMouseMove(position, Rocket::_ModifiersConvert(flags));
    }
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

    auto position = Rocket::Vec2{
        static_cast<float>(point.x),
        static_cast<float>(self.contentView.frame.size.height - point.y)
    };

    if (self->_mousePosition != position) {
        [self _dispatchMouseMove: point flags: event.modifierFlags];
    }

    if (self->_onMouseDown != nil) {
        self->_onMouseDown(Rocket::Mouse::LeftButton, {
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, Rocket::_ModifiersConvert(event.modifierFlags), static_cast<int>(event.clickCount));
        self->_leftMouseDown = YES;
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];

    if (NSPointInRect(point, self.contentView.frame) == false) {
        return;
    }

    auto position = Rocket::Vec2{
        static_cast<float>(point.x),
        static_cast<float>(self.contentView.frame.size.height - point.y)
    };

    if (self->_mousePosition != position) {
        [self _dispatchMouseMove: point flags: event.modifierFlags];
    }

    if (self->_onMouseDown != nil) {
        self->_onMouseDown(Rocket::Mouse::RightButton, {
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, Rocket::_ModifiersConvert(event.modifierFlags), static_cast<int>(event.clickCount));
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
        }, Rocket::_ModifiersConvert(event.modifierFlags));
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
        }, Rocket::_ModifiersConvert(event.modifierFlags));
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

    [self _dispatchMouseMove: [event locationInWindow] flags: event.modifierFlags];
}

- (void)mouseEntered:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];
    if (self->_onMouseEnter != nil) {
        self->_onMouseEnter({
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, Rocket::_ModifiersConvert(event.modifierFlags));
    }


}

- (void)mouseExited:(NSEvent*)event {
    PROFILE

    NSPoint point = [event locationInWindow];
    if (self->_onMouseExit != nil) {
        self->_onMouseExit({
            static_cast<float>(point.x),
            static_cast<float>(self.contentView.frame.size.height - point.y)
        }, Rocket::_ModifiersConvert(event.modifierFlags));
    }
}

- (void)mouseDragged:(NSEvent*)event {
    PROFILE

    [self _dispatchMouseMove: [event locationInWindow] flags: event.modifierFlags];
}

- (void)scrollWheel:(NSEvent*)event {
    PROFILE

    auto deltaX = static_cast<float>(event.scrollingDeltaX);
    auto deltaY = static_cast<float>(event.scrollingDeltaY);
    // auto isPrecise = event.hasPreciseScrollingDeltas;

    if (self->_onMouseWheel != nil) {
        self->_onMouseWheel({ deltaX, deltaY }, Rocket::_ModifiersConvert(event.modifierFlags));
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
    [self dispatchKeyDown: event];
}

- (void)dispatchKeyDown:(NSEvent*)event {
    PROFILE

    if (self->_onKeyDown != nil) {
        auto const scancode = Rocket::_KeyConvert(event.keyCode);
        auto const keycode = Rocket::_GetKeycode(event, scancode);

        self->_onKeyDown(
            scancode,
            keycode.c_str(),
            Rocket::_ModifiersConvert(event.modifierFlags),
            static_cast<bool>(event.isARepeat)
        );
    }
}

- (void)keyUp:(NSEvent*)event {
    PROFILE

    if (self->_onKeyUp != nil) {
        auto const scancode = Rocket::_KeyConvert(event.keyCode);
        auto const keycode = Rocket::_GetKeycode(event, scancode);

        self->_onKeyUp(
            scancode,
            keycode.c_str(),
            Rocket::_ModifiersConvert(event.modifierFlags)
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

- (void)windowDidBecomeKey:(NSNotification*)notification {
    PROFILE

    if (self->_onFocus != nil) {
        self->_onFocus();
    }

    /* macOS sends no mouseMoved while the window is not key, so report the
       cursor position on activation to refresh hover without a move */
    NSPoint point = [self mouseLocationOutsideOfEventStream];

    if (NSPointInRect(point, self.contentView.frame) == false) {
        return;
    }

    [self _dispatchMouseMove: point flags: [NSEvent modifierFlags]];
}

- (void)windowDidResignKey:(NSNotification*)notification {
    PROFILE

    if (self->_onBlur != nil) {
        self->_onBlur();
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

static auto const _keyMap = std::map<std::int32_t, Scancode>({
    { kVK_ANSI_0, Scancode::Digit0 },
    { kVK_ANSI_1, Scancode::Digit1 },
    { kVK_ANSI_2, Scancode::Digit2 },
    { kVK_ANSI_3, Scancode::Digit3 },
    { kVK_ANSI_4, Scancode::Digit4 },
    { kVK_ANSI_5, Scancode::Digit5 },
    { kVK_ANSI_6, Scancode::Digit6 },
    { kVK_ANSI_7, Scancode::Digit7 },
    { kVK_ANSI_8, Scancode::Digit8 },
    { kVK_ANSI_9, Scancode::Digit9 },
    { kVK_ANSI_A, Scancode::KeyA },
    { kVK_ANSI_B, Scancode::KeyB },
    { kVK_ANSI_C, Scancode::KeyC },
    { kVK_ANSI_D, Scancode::KeyD },
    { kVK_ANSI_E, Scancode::KeyE },
    { kVK_ANSI_F, Scancode::KeyF },
    { kVK_ANSI_G, Scancode::KeyG },
    { kVK_ANSI_H, Scancode::KeyH },
    { kVK_ANSI_I, Scancode::KeyI },
    { kVK_ANSI_J, Scancode::KeyJ },
    { kVK_ANSI_K, Scancode::KeyK },
    { kVK_ANSI_L, Scancode::KeyL },
    { kVK_ANSI_M, Scancode::KeyM },
    { kVK_ANSI_N, Scancode::KeyN },
    { kVK_ANSI_O, Scancode::KeyO },
    { kVK_ANSI_P, Scancode::KeyP },
    { kVK_ANSI_Q, Scancode::KeyQ },
    { kVK_ANSI_R, Scancode::KeyR },
    { kVK_ANSI_S, Scancode::KeyS },
    { kVK_ANSI_T, Scancode::KeyT },
    { kVK_ANSI_U, Scancode::KeyU },
    { kVK_ANSI_V, Scancode::KeyV },
    { kVK_ANSI_W, Scancode::KeyW },
    { kVK_ANSI_X, Scancode::KeyX },
    { kVK_ANSI_Y, Scancode::KeyY },
    { kVK_ANSI_Z, Scancode::KeyZ },
    { kVK_F1, Scancode::F1 },
    { kVK_F2, Scancode::F2 },
    { kVK_F3, Scancode::F3 },
    { kVK_F4, Scancode::F4 },
    { kVK_F5, Scancode::F5 },
    { kVK_F6, Scancode::F6 },
    { kVK_F7, Scancode::F7 },
    { kVK_F8, Scancode::F8 },
    { kVK_F9, Scancode::F9 },
    { kVK_F10, Scancode::F10 },
    { kVK_F11, Scancode::F11 },
    { kVK_F12, Scancode::F12 },
    { kVK_F13, Scancode::F13 },
    { kVK_F14, Scancode::F14 },
    { kVK_F15, Scancode::F15 },
    { kVK_F16, Scancode::F16 },
    { kVK_F17, Scancode::F17 },
    { kVK_F18, Scancode::F18 },
    { kVK_F19, Scancode::F19 },
    { kVK_F20, Scancode::F20 },
    { kVK_Option, Scancode::Alt },
    { kVK_DownArrow, Scancode::ArrowDown },
    { kVK_LeftArrow, Scancode::ArrowLeft },
    { kVK_RightArrow, Scancode::ArrowRight },
    { kVK_UpArrow, Scancode::ArrowUp },
    { kVK_ANSI_Backslash, Scancode::Backslash },
    { kVK_ANSI_LeftBracket, Scancode::BracketLeft },
    { kVK_ANSI_RightBracket, Scancode::BracketRight },
    { kVK_CapsLock, Scancode::Capslock },
    { kVK_ANSI_Comma, Scancode::Comma },
    { kVK_Control, Scancode::Control },
    { kVK_Delete, Scancode::Backspace },
    { kVK_End, Scancode::End },
    { kVK_ANSI_Equal, Scancode::Equal },
    { kVK_Escape, Scancode::Escape },
    { kVK_ForwardDelete, Scancode::Delete },
    { kVK_Function, Scancode::Function },
    { kVK_ANSI_Grave, Scancode::Backquote },
    { kVK_Help, Scancode::Help },
    { kVK_Home, Scancode::Home },
    { kVK_Command, Scancode::Meta },
    { kVK_ANSI_Minus, Scancode::Minus },
    { kVK_Mute, Scancode::Mute },
    { kVK_PageDown, Scancode::PageDown },
    { kVK_PageUp, Scancode::PageUp },
    { kVK_ANSI_Period, Scancode::Period },
    { kVK_ANSI_Quote, Scancode::Quote },
    { kVK_Return, Scancode::Enter },
    { kVK_RightOption, Scancode::AltRight },
    { kVK_RightControl, Scancode::ControlRight },
    { kVK_RightShift, Scancode::ShiftRight },
    { kVK_ANSI_Semicolon, Scancode::Semicolon },
    { kVK_Shift, Scancode::Shift },
    { kVK_ANSI_Slash, Scancode::Slash },
    { kVK_Space, Scancode::Space },
    { kVK_Tab, Scancode::Tab },
    { kVK_VolumeDown, Scancode::VolumeDown },
    { kVK_VolumeUp, Scancode::VolumeUp },
    { kVK_ANSI_KeypadClear, Scancode::NumpadClear },
    { kVK_ANSI_KeypadDecimal, Scancode::NumpadDecimal },
    { kVK_ANSI_KeypadDivide, Scancode::NumpadDivide },
    { kVK_ANSI_KeypadEnter, Scancode::NumpadEnter },
    { kVK_ANSI_KeypadEquals, Scancode::NumpadEqual },
    { kVK_ANSI_KeypadMinus, Scancode::NumpadMinus },
    { kVK_ANSI_KeypadMultiply, Scancode::NumpadMultiply },
    { kVK_ANSI_KeypadPlus, Scancode::NumpadAdd },
    { kVK_ANSI_Keypad0, Scancode::Numpad0 },
    { kVK_ANSI_Keypad1, Scancode::Numpad1 },
    { kVK_ANSI_Keypad2, Scancode::Numpad2 },
    { kVK_ANSI_Keypad3, Scancode::Numpad3 },
    { kVK_ANSI_Keypad4, Scancode::Numpad4 },
    { kVK_ANSI_Keypad5, Scancode::Numpad5 },
    { kVK_ANSI_Keypad6, Scancode::Numpad6 },
    { kVK_ANSI_Keypad7, Scancode::Numpad7 },
    { kVK_ANSI_Keypad8, Scancode::Numpad8 },
    { kVK_ANSI_Keypad9, Scancode::Numpad9 }
});

static KeyModifiers _ModifiersConvert(NSEventModifierFlags flags) {
    PROFILE

    return {
        .control = static_cast<bool>((flags & NSEventModifierFlagControl)),
        .shift = static_cast<bool>((flags & NSEventModifierFlagShift)),
        .meta = static_cast<bool>((flags & NSEventModifierFlagCommand)),
        .alt = static_cast<bool>((flags & NSEventModifierFlagOption))
    };
}

static bool _IsControlCodepoint(std::uint32_t codepoint) {
    PROFILE

    /* control characters and the private-use range AppKit gives function keys */
    return (codepoint < 0x20u)
        || (codepoint == 0x7Fu)
        || ((codepoint >= 0xF700u) && (codepoint <= 0xF8FFu));
}

static Scancode _KeyConvert(std::int32_t code) {
    PROFILE

    if (_keyMap.find(code) != _keyMap.end()) {
        return _keyMap.at(code);
    } else {
        return Scancode::Unknown;
    }
}

static bool _IsCharacterScancode(Scancode scancode) {
    PROFILE

    switch (scancode) {
        case Scancode::Digit0: case Scancode::Digit1: case Scancode::Digit2: case Scancode::Digit3: case Scancode::Digit4:
        case Scancode::Digit5: case Scancode::Digit6: case Scancode::Digit7: case Scancode::Digit8: case Scancode::Digit9:
        case Scancode::KeyA: case Scancode::KeyB: case Scancode::KeyC: case Scancode::KeyD: case Scancode::KeyE:
        case Scancode::KeyF: case Scancode::KeyG: case Scancode::KeyH: case Scancode::KeyI: case Scancode::KeyJ:
        case Scancode::KeyK: case Scancode::KeyL: case Scancode::KeyM: case Scancode::KeyN: case Scancode::KeyO:
        case Scancode::KeyP: case Scancode::KeyQ: case Scancode::KeyR: case Scancode::KeyS: case Scancode::KeyT:
        case Scancode::KeyU: case Scancode::KeyV: case Scancode::KeyW: case Scancode::KeyX: case Scancode::KeyY:
        case Scancode::KeyZ:
        case Scancode::Backquote: case Scancode::Minus: case Scancode::Equal:
        case Scancode::BracketLeft: case Scancode::BracketRight: case Scancode::Backslash:
        case Scancode::Semicolon: case Scancode::Quote:
        case Scancode::Comma: case Scancode::Period: case Scancode::Slash:
        case Scancode::Space:
            return true;
        default:
            return false;
    }
}

/* What the key means on the current layout: character keys take the
   character the layout gives them without modifiers (lowercased, so Shift
   does not change it); every other key, and a character key that yields no
   usable character (a dead key, a control character), keeps its name. */
static std::string _GetKeycode(NSEvent* event, Scancode scancode) {
    PROFILE

    if (_IsCharacterScancode(scancode) == false) {
        return GetDefaultKeycode(scancode);
    }

    NSString* const characters = [event.charactersIgnoringModifiers lowercaseString];

    if (characters.length == 0) {
        return GetDefaultKeycode(scancode);
    }

    auto const cString = [characters cStringUsingEncoding: NSUTF8StringEncoding];

    if (cString == NULL) {
        return GetDefaultKeycode(scancode);
    }

    static thread_local auto _codepoints = std::vector<std::uint32_t>();
    StringToCodepoints(cString, _codepoints);

    for (auto const codepoint : _codepoints) {
        if (_IsControlCodepoint(codepoint)) {
            return GetDefaultKeycode(scancode);
        }
    }

    return std::string(cString);
}

static std::string _SanitizeInput(NSString* characters) {
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
        if (_IsControlCodepoint(codepoint) == false) {
            _kept.push_back(codepoint);
        }
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

/* AppKit hands key status to the next window only under [NSApp run]. With
   our own event loop (see App_MacOS.mm) closing the key window leaves the
   app with no key window, so pass it on by hand. */
static void _CloseWindow(NSWindow* window) {
    PROFILE

    auto const wasKey = [window isKeyWindow];

    [window close];

    if (wasKey == false) return;

    /* orderedWindows is front to back, so the first match is the window
       right behind the one that closed. */
    for (NSWindow* next in [NSApp orderedWindows]) {
        if (next != window && [next isVisible] && [next canBecomeKeyWindow]) {
            [next makeKeyAndOrderFront: nil];
            break;
        }
    }
}

void Window::__done() {
    PROFILE

    if (_impl->cursorHidden) {
        [NSCursor unhide]; /* balance the app-global hide from __setCursor(Cursor::None) */
    }

    [_impl->window setDelegate: nil];
    [_impl->window setContentView: nil]; /* detaching fires viewDidMoveToWindow(nil), which invalidates the display link */
    [_impl->window setReleasedWhenClosed: NO];

    _CloseWindow(_impl->window);

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
    window.onKeyDown = ^(Scancode scancode, char const* keycode, KeyModifiers mods, bool repeat) { _keyDown(scancode, std::string(keycode), mods, repeat); };
    window.onKeyUp = ^(Scancode scancode, char const* keycode, KeyModifiers mods) { _keyUp(scancode, std::string(keycode), mods); };
    window.onPixelRatio = ^(double pixelratio) { _dpiChange(); };
    window.onFocus = ^() { _focus(); };
    window.onBlur = ^() { _blur(); };
    view.onDisplay = ^() { _paint(); };
    view.onKeyDown = ^(NSEvent* event) { [weakWindow dispatchKeyDown: event]; };
    view.onInput = ^(NSString* text) {
        auto const input = _SanitizeInput(text); /* control characters never reach input */
        if (input.empty() == false) {
            _input(input);
        }
    };
    view.onComposition = ^(NSString* text, NSInteger cursor, NSInteger selectionLength) {
        _composition(std::string([text UTF8String] != NULL ? [text UTF8String] : ""), static_cast<std::int32_t>(cursor), static_cast<std::int32_t>(selectionLength));
    };

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

void Window::__setInputArea(std::optional<Vec4> const& caretRect) {
    PROFILE

    auto view = (__NSMetalView*)_impl->window.contentView;

    if (caretRect.has_value()) {
        view.inputArea = [NSValue valueWithRect: NSMakeRect(caretRect->x, caretRect->y, caretRect->width, caretRect->height)];
        [view.inputContext invalidateCharacterCoordinates];
    } else if (view.inputArea != nil) {
        view.inputArea = nil;
        [view.inputContext discardMarkedText]; /* drop a composition in progress */
        [view unmarkText];
    }
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
            _CloseWindow(_impl->window); /* windowWillClose: dispatches _hide() */
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
