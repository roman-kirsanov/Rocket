/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import <Cocoa/Cocoa.h>
#include <cassert>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>

namespace Rocket {

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    auto const panel = [NSOpenPanel openPanel];
    [panel setTitle: [NSString stringWithUTF8String: options.title.c_str()]];
    [panel setPrompt: [NSString stringWithUTF8String: options.action.c_str()]];
    [panel setMessage: [NSString stringWithUTF8String: options.message.c_str()]];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: options.directorySelection];
    [panel setAllowsMultipleSelection: options.multipleSelection];

    if (options.extensions.size() > 0) {
        auto const array = [NSMutableArray array];
        for (auto const& [ description, extension ] : options.extensions) {
            [array addObject: [NSString stringWithUTF8String: extension.c_str()]];
        }
        [panel setAllowedFileTypes: array];
    }

    auto const previousKeyWindow = [NSApp keyWindow];

    if ([panel runModal] == ::NSModalResponseOK) {
        auto paths = std::vector<std::string>();

        for (NSURL* url in panel.URLs) {
            paths.emplace_back([[url path] UTF8String]);
        }

        assert(paths.size() > 0);

        callback(OpenFileDialogOkResult{ std::move(paths) });
    } else {
        callback(OpenFileDialogCancelResult{});
    }

    if (previousKeyWindow != nil) {
        [previousKeyWindow makeKeyAndOrderFront: nil];
    }
}

void __SaveFileDialog(SaveFileDialogOptions const& options, SaveFileDialogCallback const& callback) {
    PROFILE


}

} /* namespace Rocket */