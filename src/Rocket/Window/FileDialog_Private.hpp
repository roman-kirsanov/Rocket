/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Window/FileDialog.hpp>

namespace Rocket {

void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback);

void __SaveFileDialog(SaveFileDialogOptions const& options, SaveFileDialogCallback const& callback);

} /* namespace Rocket */