/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <tuple>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <cassert>
#include <filesystem>
#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Private.hpp>
#include <Rocket/Window/App_SDL3.hpp>
#include <Rocket/Window/Window.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>

namespace Rocket {

/* SDL's dialog callback may run on another thread. */
struct _FileDialogState {
    std::mutex mutex;
    std::atomic<bool> done = false;
    std::vector<std::string> paths;
};

static void SDLCALL _OnFileDialog(void* userdata, char const* const* filelist, int filter) {
    PROFILE

    auto state = static_cast<_FileDialogState*>(userdata);

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

static std::vector<std::string> _ShowFileDialog(::SDL_FileDialogType type, ::SDL_PropertiesID properties, std::string const& title, std::string const& action, std::vector<std::tuple<std::string, std::string>> const& extensions, Window* window) {
    PROFILE

    auto state = _FileDialogState{};

    /* filters must outlive the dialog: they are read when it shows */
    auto filters = std::vector<::SDL_DialogFileFilter>();

    for (auto const& [ description, extension ] : extensions) {
        filters.push_back(::SDL_DialogFileFilter{ description.c_str(), extension.c_str() });
    }

    ::SDL_SetStringProperty(properties, SDL_PROP_FILE_DIALOG_TITLE_STRING, title.c_str());
    ::SDL_SetStringProperty(properties, SDL_PROP_FILE_DIALOG_ACCEPT_STRING, action.c_str());

    if (filters.empty() == false) {
        ::SDL_SetPointerProperty(properties, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, filters.data());
        ::SDL_SetNumberProperty(properties, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, static_cast<::Sint64>(filters.size()));
    }

    if (window != nullptr) {
        ::SDL_SetPointerProperty(properties, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, window->getHandle());
    }

    ::SDL_ShowFileDialogWithProperties(type, _OnFileDialog, &state, properties);

    /* SDL's dialog runs asynchronously and Rocket's synchronously: tick the run
       loop, without App updates, until the dialog is dismissed */
    while (state.done == false) {
        App::__Tick(false);
    }

    auto lock = std::scoped_lock(state.mutex);

    return std::move(state.paths);
}

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    App::__InitVideo();

    auto const previousWindow = Window::GetFocusedWindow();
    auto const properties = ::SDL_CreateProperties();

    ::SDL_SetBooleanProperty(properties, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, options.multipleSelection);
    // NOTE: Not supported in SDL3 (options.message)

    /* SDL has no dialog that picks files and folders alike: one that allows
       folders picks folders */
    auto const type = options.directorySelection ? ::SDL_FILEDIALOG_OPENFOLDER : ::SDL_FILEDIALOG_OPENFILE;
    auto paths = _ShowFileDialog(type, properties, options.title, options.action, options.extensions, previousWindow);

    ::SDL_DestroyProperties(properties);

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

    App::__InitVideo();

    auto const previousWindow = Window::GetFocusedWindow();
    auto const properties = ::SDL_CreateProperties();
    auto const location = (std::filesystem::path(options.directory) / options.name).string();

    if (location.empty() == false) {
        ::SDL_SetStringProperty(properties, SDL_PROP_FILE_DIALOG_LOCATION_STRING, location.c_str());
    }

    // NOTE: Not supported in SDL3 (options.message)

    auto const paths = _ShowFileDialog(::SDL_FILEDIALOG_SAVEFILE, properties, options.title, options.action, options.extensions, previousWindow);

    ::SDL_DestroyProperties(properties);

    if (paths.empty() == false) {
        callback(SaveFileDialogOkResult{ paths.front() });
    } else {
        callback(SaveFileDialogCancelResult{});
    }

    if (previousWindow != nullptr) {
        previousWindow->setFocus();
    }
}

} /* namespace Rocket */
