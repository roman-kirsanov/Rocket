/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/FileDialog_Private.hpp>

namespace Rocket {

void OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback) {
    PROFILE

    __OpenFileDialog(options, callback);
}

void SaveFileDialog(SaveFileDialogOptions const& options, SaveFileDialogCallback const& callback) {
    PROFILE

    __SaveFileDialog(options, callback);
}

} /* namespace Rocket */