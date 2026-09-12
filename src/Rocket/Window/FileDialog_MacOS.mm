#import <Cocoa/Cocoa.h>
#include <cassert>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>

namespace Rocket {

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    __block auto retainedCallback = callback;

    auto panel = [NSOpenPanel openPanel];
    [panel setTitle: [NSString stringWithUTF8String: options.title.c_str()]];
    [panel setPrompt: [NSString stringWithUTF8String: options.action.c_str()]];
    [panel setMessage: [NSString stringWithUTF8String: options.message.c_str()]];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: options.directorySelection];
    [panel setAllowsMultipleSelection: options.multipleSelection];

    if (options.extensions.size() > 0) {
        auto* array = [NSMutableArray array];
        for (auto const& [ description, extension ] : options.extensions) {
            [array addObject: [NSString stringWithUTF8String: extension.c_str()]];
        }
        [panel setAllowedFileTypes: array];
    }

    NSWindow* previousKeyWindow = [NSApp keyWindow];
    NSModalResponse modalResult = [panel runModal];

    if (modalResult == NSModalResponseOK) {
        auto paths = std::vector<std::string>();

        for (NSURL* url in panel.URLs) {
            paths.emplace_back([[url path] UTF8String]);
        }

        assert(paths.size() > 0);

        retainedCallback(OpenFileDialogOkResult{ std::move(paths) });
    } else {
        retainedCallback(OpenFileDialogCancelResult{});
    }

    if (previousKeyWindow) {
        [previousKeyWindow makeKeyAndOrderFront: nullptr];
    }
}

void __SaveFileDialog(SaveFileDialogOptions const& options, SaveFileDialogCallback const& callback) {
    PROFILE


}

} /* namespace Rocket */