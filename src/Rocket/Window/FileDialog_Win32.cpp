/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <shobjidl.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Win32.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

static std::vector<std::string> _GetPaths(IShellItemArray* items) {
    PROFILE

    auto paths = std::vector<std::string>();
    auto count = DWORD(0);

    items->GetCount(&count);

    for (auto i = DWORD(0); i < count; i++) {
        auto item = static_cast<IShellItem*>(nullptr);

        if (SUCCEEDED(items->GetItemAt(i, &item))) {
            auto path = PWSTR(nullptr);

            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                paths.push_back(App::__FromWide(path));
                ::CoTaskMemFree(path);
            }

            item->Release();
        }
    }

    return paths;
}

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    App::__InitWin32();

    auto paths = std::vector<std::string>();
    auto dialog = static_cast<IFileOpenDialog*>(nullptr);
    auto const previousWindow = Window::GetFocusedWindow();

    if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        auto flags = FILEOPENDIALOGOPTIONS(0);
        dialog->GetOptions(&flags);

        flags |= (FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);

        if (options.directorySelection) {
            flags |= FOS_PICKFOLDERS;
        }

        if (options.multipleSelection) {
            flags |= FOS_ALLOWMULTISELECT;
        }

        dialog->SetOptions(flags);

        if (options.title.empty() == false) {
            dialog->SetTitle(App::__ToWide(options.title).c_str());
        }

        if (options.action.empty() == false) {
            dialog->SetOkButtonLabel(App::__ToWide(options.action).c_str());
        }

        // NOTE: Not supported in Win32 (options.message)

        auto names = std::vector<std::wstring>();
        auto patterns = std::vector<std::wstring>();
        auto filters = std::vector<COMDLG_FILTERSPEC>();

        for (auto const& [ description, extension ] : options.extensions) {
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
            filters.push_back(COMDLG_FILTERSPEC{ names[i].c_str(), patterns[i].c_str() });
        }

        if ((filters.empty() == false) && (options.directorySelection == false)) {
            dialog->SetFileTypes(static_cast<UINT>(filters.size()), filters.data());
        }

        auto const owner = (previousWindow != nullptr) ? static_cast<HWND>(previousWindow->getHandle()) : nullptr;

        App::__SetTimerPainting(true);
        auto const shown = dialog->Show(owner);
        App::__SetTimerPainting(false);

        if (SUCCEEDED(shown)) {
            auto items = static_cast<IShellItemArray*>(nullptr);

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


}

} /* namespace Rocket */
