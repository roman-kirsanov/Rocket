#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/Clipboard_Private.hpp>

namespace Rocket {

std::string GetClipboardString() {
    PROFILE

    return __GetClipboardString();
}

void SetClipboardString(std::string const& string) {
    PROFILE

    __SetClipboardString(string);
}

} /* namespace Rocket */