/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import <Cocoa/Cocoa.h>
#include <tuple>
#include <string>
#include <vector>
#include <cassert>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>

namespace Rocket {

static void _SetUpPanel(NSSavePanel* panel, std::string const& title, std::string const& message, std::string const& action, std::vector<std::tuple<std::string, std::string>> const& extensions) {
    PROFILE

    [panel setTitle: [NSString stringWithUTF8String: title.c_str()]];
    [panel setPrompt: [NSString stringWithUTF8String: action.c_str()]];
    [panel setMessage: [NSString stringWithUTF8String: message.c_str()]];

    if (extensions.size() > 0) {
        auto const array = [NSMutableArray array];
        for (auto const& [ description, extension ] : extensions) {
            for (NSString* type in [[NSString stringWithUTF8String: extension.c_str()] componentsSeparatedByString: @";"]) {
                if ([type length] > 0) {
                    [array addObject: type];
                }
            }
        }
        [panel setAllowedFileTypes: array];
    }
}

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    auto const panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: options.directorySelection];
    [panel setAllowsMultipleSelection: options.multipleSelection];

    _SetUpPanel(panel, options.title, options.message, options.action, options.extensions);

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

    auto const panel = [NSSavePanel savePanel];
    [panel setNameFieldStringValue: [NSString stringWithUTF8String: options.name.c_str()]];
    [panel setCanCreateDirectories: YES];

    _SetUpPanel(panel, options.title, options.message, options.action, options.extensions);

    if (options.directory.empty() == false) {
        [panel setDirectoryURL: [NSURL fileURLWithPath: [NSString stringWithUTF8String: options.directory.c_str()] isDirectory: YES]];
    }

    auto const previousKeyWindow = [NSApp keyWindow];

    if ([panel runModal] == ::NSModalResponseOK) {
        callback(SaveFileDialogOkResult{ [[[panel URL] path] UTF8String] });
    } else {
        callback(SaveFileDialogCancelResult{});
    }

    if (previousKeyWindow != nil) {
        [previousKeyWindow makeKeyAndOrderFront: nil];
    }
}

} /* namespace Rocket */