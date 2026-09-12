#include <Rocket/Window/FileDialog.hpp>

namespace Rocket {

/**
 * Platform implementation of OpenFileDialog.
 *
 * @param options  The dialog configuration.
 * @param callback Invoked synchronously with the outcome once the dialog is dismissed.
 */
void __OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback);

/**
 * Platform implementation of SaveFileDialog.
 *
 * @param options  The dialog configuration; currently unused (not implemented).
 * @param callback Never invoked; the dialog is not implemented yet.
 */
void __SaveFileDialog(SaveFileDialogOptions const& options, SaveFileDialogCallback const& callback);

} /* namespace Rocket */