#include <string>
#include <cstdlib>
#include <filesystem>
#include <SDL3/SDL.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Private.hpp>
#include <Rocket/Window/Window_SDL3.hpp>

namespace Rocket {
namespace App {

static auto _isExited = false;

/* SDL returns folders with a trailing separator; Rocket's have none. */
static std::string _TrimSeparator(std::string path) {
    PROFILE

    while ((path.size() > 1) && (path.back() == '/')) {
        path.pop_back();
    }

    return path;
}

std::string const& __GetExecPath() {
    PROFILE

    /* SDL has no function for it: the one POSIX call of the backend */
    static auto const _execPath = [] {
        auto error = std::error_code();
        auto const path = std::filesystem::read_symlink("/proc/self/exe", error);

        return error ? std::string() : path.string();
    }();

    return _execPath;
}

std::string const& __GetExecDir() {
    PROFILE

    static auto const _execDir = [] {
        auto const basePath = SDL_GetBasePath();

        return (basePath != nullptr) ? _TrimSeparator(basePath) : std::string();
    }();

    return _execDir;
}

std::string const& __GetWorkDir() {
    PROFILE

    static auto _workDir = std::string();

    if (auto const currentDirectory = SDL_GetCurrentDirectory()) {
        _workDir = _TrimSeparator(currentDirectory);
        SDL_free(currentDirectory);
    }

    return _workDir;
}

std::string const& __GetUserDir() {
    PROFILE

    /* The XDG folder for settings: $XDG_CONFIG_HOME, or ~/.config when it is not set. */
    static auto const _userDir = [] {
        auto const configHome = std::getenv("XDG_CONFIG_HOME");

        if ((configHome != nullptr) && (configHome[0] == '/')) {
            return _TrimSeparator(configHome);
        }

        auto const homeFolder = SDL_GetUserFolder(SDL_FOLDER_HOME);

        return (homeFolder != nullptr) ? (_TrimSeparator(homeFolder) + "/.config") : std::string();
    }();

    return _userDir;
}

void __Exit() {
    PROFILE

    _isExited = true;
}

void __Run(int argc, char const** argv) {
    // PROFILE

    __InitVideo();

    _isExited = false;

    while (_isExited == false) {
        __Tick(true);
    }
}

} /* namespace App */
} /* namespace Rocket */
