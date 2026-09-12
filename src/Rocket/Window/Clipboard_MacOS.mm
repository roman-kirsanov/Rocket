#import <AppKit/AppKit.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/Clipboard_Private.hpp>

namespace Rocket {

std::string __GetClipboardString() {
    PROFILE

    auto string = [[NSPasteboard generalPasteboard] stringForType: NSPasteboardTypeString];

    if (string != nil) {
        return [string UTF8String];
    } else {
        return std::string();
    }
}

void __SetClipboardString(std::string const& string) {
    PROFILE

    [[NSPasteboard generalPasteboard] clearContents];
    [[NSPasteboard generalPasteboard] setString: [NSString stringWithUTF8String: string.c_str()]
                                        forType: NSPasteboardTypeString];
}

} /* namespace Rocket */