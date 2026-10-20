/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <cassert>
#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Private.hpp>
#include <Rocket/Window/App_SDL3.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

/** SDL's dialog callback may run on another thread. */
struct _OpenFileDialogState {
    std::mutex mutex;
    std::atomic<bool> done = false;
    std::vector<std::string> paths;
};

static void SDLCALL _OnOpenFileDialog(void* userdata, char const* const* filelist, int filter) {
    PROFILE

    auto state = static_cast<_OpenFileDialogState*>(userdata);

    {
        auto lock = std::scoped_lock(state->mutex);

        /* an error (null list) is a cancel, like an empty selection */
        if (filelist != nullptr) {
            for (auto file = filelist; *file != nullptr; file++) {
                state->paths.emplace_back(*file);
            }
        }
    }

    state->done = true;
}

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    App::__InitVideo();

    auto state = _OpenFileDialogState{};

    /* filters must outlive the dialog: they are read when it shows */
    auto filters = std::vector<SDL_DialogFileFilter>();

    for (auto const& [ description, extension ] : options.extensions) {
        filters.push_back(SDL_DialogFileFilter{ description.c_str(), extension.c_str() });
    }

    auto const previousWindow = Window::GetFocusedWindow();
    auto const properties = SDL_CreateProperties();

    SDL_SetStringProperty(properties, SDL_PROP_FILE_DIALOG_TITLE_STRING, options.title.c_str());
    SDL_SetStringProperty(properties, SDL_PROP_FILE_DIALOG_ACCEPT_STRING, options.action.c_str());
    SDL_SetBooleanProperty(properties, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, options.multipleSelection);
    // NOTE: Not supported in SDL3 (options.message)

    if (filters.empty() == false) {
        SDL_SetPointerProperty(properties, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, filters.data());
        SDL_SetNumberProperty(properties, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, static_cast<Sint64>(filters.size()));
    }

    if (previousWindow != nullptr) {
        SDL_SetPointerProperty(properties, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, previousWindow->getHandle());
    }

    /* SDL has no dialog that picks files and folders alike: one that allows
       folders picks folders */
    auto const type = options.directorySelection ? SDL_FILEDIALOG_OPENFOLDER : SDL_FILEDIALOG_OPENFILE;

    SDL_ShowFileDialogWithProperties(type, _OnOpenFileDialog, &state, properties);
    SDL_DestroyProperties(properties);

    /* SDL's dialog runs asynchronously and Rocket's synchronously: tick the run
       loop, without App updates, until the dialog is dismissed */
    while (state.done == false) {
        App::__Tick(false);
    }

    auto paths = std::vector<std::string>();
    {
        auto lock = std::scoped_lock(state.mutex);
        paths = std::move(state.paths);
    }

    if (paths.empty() == false) {
        callback(OpenFileDialogOkResult{ std::move(paths) });
    } else {
        callback(OpenFileDialogCancelResult{});
    }

    if (previousWindow != nullptr) {
        previousWindow->setFocus();
    }
}

void __SaveFileDialog(SaveFileDialogOptions const& options, SaveFileDialogCallback const& callback) {
    PROFILE


}

} /* namespace Rocket */
