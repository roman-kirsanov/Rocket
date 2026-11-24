/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <tuple>
#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <shobjidl.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Win32.hpp>
#include <Rocket/Window/Window.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>

namespace Rocket {

static std::string _GetPath(::IShellItem* item) {
    PROFILE

    auto result = std::string();
    auto path = ::PWSTR(nullptr);

    if (SUCCEEDED(item->GetDisplayName(::SIGDN_FILESYSPATH, &path))) {
        result = App::__FromWide(path);
        ::CoTaskMemFree(path);
    }

    return result;
}

static std::vector<std::string> _GetPaths(::IShellItemArray* items) {
    PROFILE

    auto paths = std::vector<std::string>();
    auto count = ::DWORD(0);

    items->GetCount(&count);

    for (auto i = ::DWORD(0); i < count; i++) {
        auto item = static_cast<::IShellItem*>(nullptr);

        if (SUCCEEDED(items->GetItemAt(i, &item))) {
            if (auto path = _GetPath(item); path.empty() == false) {
                paths.push_back(std::move(path));
            }

            item->Release();
        }
    }

    return paths;
}

static bool _ShowDialog(::IFileDialog* dialog, std::string const& title, std::string const& action, std::vector<std::tuple<std::string, std::string>> const& extensions, bool fileTypes, Window* window) {
    PROFILE

    if (title.empty() == false) {
        dialog->SetTitle(App::__ToWide(title).c_str());
    }

    if (action.empty() == false) {
        dialog->SetOkButtonLabel(App::__ToWide(action).c_str());
    }

    auto names = std::vector<std::wstring>();
    auto patterns = std::vector<std::wstring>();
    auto filters = std::vector<::COMDLG_FILTERSPEC>();

    for (auto const& [ description, extension ] : extensions) {
        auto pattern = std::wstring();
        auto start = std::size_t(0);

        while (start <= extension.size()) {
            auto const end = std::min(extension.find(';', start), extension.size());

            if (end > start) {
                pattern += (pattern.empty() ? L"*." : L";*.") + App::__ToWide(extension.substr(start, end - start));
            }

            start = end + 1;
        }

        names.push_back(App::__ToWide(description.empty() ? extension : description));
        patterns.push_back(pattern);
    }

    for (auto i = std::size_t(0); i < names.size(); i++) {
        filters.push_back(::COMDLG_FILTERSPEC{ names[i].c_str(), patterns[i].c_str() });
    }

    if ((filters.empty() == false) && fileTypes) {
        dialog->SetFileTypes(static_cast<::UINT>(filters.size()), filters.data());
    }

    auto const owner = (window != nullptr) ? static_cast<::HWND>(window->getHandle()) : nullptr;

    App::__SetTimerPainting(true);
    auto const shown = dialog->Show(owner);
    App::__SetTimerPainting(false);

    return SUCCEEDED(shown);
}

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    App::__InitWin32();

    auto paths = std::vector<std::string>();
    auto dialog = static_cast<::IFileOpenDialog*>(nullptr);
    auto const previousWindow = Window::GetFocusedWindow();

    if (SUCCEEDED(::CoCreateInstance(::CLSID_FileOpenDialog, nullptr, ::CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        auto flags = ::FILEOPENDIALOGOPTIONS(0);
        dialog->GetOptions(&flags);

        flags |= (::FOS_FORCEFILESYSTEM | ::FOS_PATHMUSTEXIST | ::FOS_FILEMUSTEXIST);

        if (options.directorySelection) {
            flags |= ::FOS_PICKFOLDERS;
        }

        if (options.multipleSelection) {
            flags |= ::FOS_ALLOWMULTISELECT;
        }

        dialog->SetOptions(flags);

        // NOTE: Not supported in Win32 (options.message)

        if (_ShowDialog(dialog, options.title, options.action, options.extensions, (options.directorySelection == false), previousWindow)) {
            auto items = static_cast<::IShellItemArray*>(nullptr);

            if (SUCCEEDED(dialog->GetResults(&items))) {
                paths = _GetPaths(items);
                items->Release();
            }
        }

        dialog->Release();
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

    App::__InitWin32();

    auto path = std::string();
    auto dialog = static_cast<::IFileSaveDialog*>(nullptr);
    auto const previousWindow = Window::GetFocusedWindow();

    if (SUCCEEDED(::CoCreateInstance(::CLSID_FileSaveDialog, nullptr, ::CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        auto flags = ::FILEOPENDIALOGOPTIONS(0);
        dialog->GetOptions(&flags);

        flags |= (::FOS_FORCEFILESYSTEM | ::FOS_PATHMUSTEXIST | ::FOS_OVERWRITEPROMPT);

        dialog->SetOptions(flags);

        if (options.name.empty() == false) {
            dialog->SetFileName(App::__ToWide(options.name).c_str());
        }

        if (options.directory.empty() == false) {
            auto folder = static_cast<::IShellItem*>(nullptr);

            if (SUCCEEDED(::SHCreateItemFromParsingName(App::__ToWide(options.directory).c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
                dialog->SetFolder(folder);
                folder->Release();
            }
        }

        if (options.extensions.empty() == false) {
            auto const& extension = std::get<1>(options.extensions.front());
            dialog->SetDefaultExtension(App::__ToWide(extension.substr(0, extension.find(';'))).c_str());
        }

        // NOTE: Not supported in Win32 (options.message)

        if (_ShowDialog(dialog, options.title, options.action, options.extensions, true, previousWindow)) {
            auto item = static_cast<::IShellItem*>(nullptr);

            if (SUCCEEDED(dialog->GetResult(&item))) {
                path = _GetPath(item);
                item->Release();
            }
        }

        dialog->Release();
    }

    if (path.empty() == false) {
        callback(SaveFileDialogOkResult{ std::move(path) });
    } else {
        callback(SaveFileDialogCancelResult{});
    }

    if (previousWindow != nullptr) {
        previousWindow->setFocus();
    }
}

} /* namespace Rocket */
