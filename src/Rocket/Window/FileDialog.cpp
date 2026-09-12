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