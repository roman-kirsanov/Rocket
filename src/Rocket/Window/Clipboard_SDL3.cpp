#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/Clipboard_Private.hpp>
#include <Rocket/Window/Window_SDL3.hpp>

namespace Rocket {

std::string __GetClipboardString() {
    PROFILE

    __InitVideo();

    auto const text = SDL_GetClipboardText();
    auto string = std::string((text != nullptr) ? text : "");

    SDL_free(text);

    return string;
}

void __SetClipboardString(std::string const& string) {
    PROFILE

    __InitVideo();

    SDL_SetClipboardText(string.c_str());
}

} /* namespace Rocket */
