/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_SDL3.hpp>
#include <Rocket/Window/Clipboard_Private.hpp>

namespace Rocket {

std::string __GetClipboardString() {
    PROFILE

    App::__InitVideo();

    auto const text = SDL_GetClipboardText();
    auto string = std::string((text != nullptr) ? text : "");

    SDL_free(text);

    return string;
}

void __SetClipboardString(std::string const& string) {
    PROFILE

    App::__InitVideo();

    SDL_SetClipboardText(string.c_str());
}

} /* namespace Rocket */
