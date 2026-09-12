#include <print>
#include <Cocoa/Cocoa.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Private.hpp>

auto constexpr _APP_EVENT_TIMEOUT = 16.667;

@interface __Delegate : NSObject<NSApplicationDelegate>

- (id)initWithOnRun:(void(^)(void))onRun onExit:(void(^)(void))onExit;

@end

@implementation __Delegate {
    void (^_onRun)(void);
    void (^_onExit)(void);
}

- (id)initWithOnRun:(void(^)(void))onRun onExit:(void(^)(void))onExit {
    PROFILE

    self = [super init];
    if (self) {
        _onRun = onRun;
        _onExit = onExit;
    }

    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    PROFILE

    _onRun();
}

- (void)exit {
    PROFILE

    _onExit();
}

@end

namespace Rocket {
namespace App {

static auto _isExited = false;

static auto _execPath = std::string(
    [[[NSBundle mainBundle] executablePath] cStringUsingEncoding: NSUTF8StringEncoding]
);

static auto _execDir = std::string(
    [[[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent] cStringUsingEncoding: NSUTF8StringEncoding]
);

static auto _workDir = std::string(
    [[[NSFileManager defaultManager] currentDirectoryPath] cStringUsingEncoding: NSUTF8StringEncoding]
);

std::string const& __GetExecPath() {
    PROFILE

    return _execPath;
}

std::string const& __GetExecDir() {
    PROFILE

    return _execDir;
}

std::string const& __GetWorkDir() {
    PROFILE

    _workDir = std::string([[[NSFileManager defaultManager] currentDirectoryPath] cStringUsingEncoding: NSUTF8StringEncoding]);

    return _workDir;
}

std::string const& __GetUserDir() {
    PROFILE

    static auto const _userDir = std::string(
        [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject] cStringUsingEncoding: NSUTF8StringEncoding]
    );

    return _userDir;
}

void __Exit() {
    PROFILE

    _isExited = true;
}

void __Run(int argc, char const** argv) {
    // PROFILE

    [[NSApplication sharedApplication]
        setDelegate: [[__Delegate alloc]
            initWithOnRun: ^{
                id mainMenu = [[NSMenu alloc] init];
                id appMenu = [[NSMenu alloc] init];
                id appItem = [mainMenu addItemWithTitle: [NSString stringWithCString: "123" encoding: NSUTF8StringEncoding] action: nil keyEquivalent: @""];
                id quitItem = [appMenu addItemWithTitle: @"Quit" action: nil keyEquivalent: @"q"];
                [quitItem setTarget: [NSApp delegate]];
                [quitItem setAction: @selector(exit)];
                [appItem setSubmenu: appMenu];
                [NSApp setMainMenu: mainMenu];
                [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
                [NSApp activateIgnoringOtherApps: YES];
            }
            onExit: ^{
                _Close();
            }
        ]
    ];
    [[NSApplication sharedApplication] finishLaunching];

    _isExited = false;

    for (;;) {
        @autoreleasepool {
            NSEvent* event = [NSApp nextEventMatchingMask: NSEventMaskAny
                                                untilDate: ((_APP_EVENT_TIMEOUT > 0) ? [NSDate dateWithTimeIntervalSinceNow: (_APP_EVENT_TIMEOUT / 1000.0)] : nil)
                                                   inMode: NSDefaultRunLoopMode
                                                  dequeue: YES];
            if (event != nil) {
                [NSApp sendEvent: event];
                [NSApp updateWindows];
            }

            _Update();

            if (_isExited) {
                /* Leave the loop rather than [NSApp terminate:], which would
                   exit() the process here and never let Run return. */
                [[NSApplication sharedApplication] setDelegate: nil];
                return;
            }
        }
    }
}

} /* namespace App */
} /* namespace Rocket */