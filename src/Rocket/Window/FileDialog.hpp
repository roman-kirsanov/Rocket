#pragma once

#include <tuple>
#include <vector>
#include <string>
#include <variant>
#include <functional>

namespace Rocket {

/** Successful open-dialog outcome: the absolute paths the user selected (never empty). */
struct OpenFileDialogOkResult {
    std::vector<std::string> paths;
};

/** The user dismissed the open dialog without selecting anything. */
struct OpenFileDialogCancelResult {};

/** Outcome of an open dialog: either OpenFileDialogOkResult or OpenFileDialogCancelResult. */
using OpenFileDialogResult = std::variant<OpenFileDialogOkResult, OpenFileDialogCancelResult>;

/** Invoked with the outcome when the open dialog is dismissed. */
using OpenFileDialogCallback = std::function<void(OpenFileDialogResult const&)>;

/** Configuration for OpenFileDialog. */
struct OpenFileDialogOptions {
    /** Allow picking directories (files are always selectable). */
    bool directorySelection;

    /** Allow selecting more than one item. */
    bool multipleSelection;

    /** Dialog window title. */
    std::string title;

    /** Explanatory text shown inside the dialog. */
    std::string message;

    /** Label of the confirm button (e.g. "Open"). */
    std::string action;

    /**
     * Allowed file types as (description, extension) pairs; empty allows any
     * type. Only the extension is used on macOS.
     */
    std::vector<
        std::tuple<std::string, std::string>
    > extensions;
};

/**
 * Shows a modal open-file dialog.
 *
 * Runs the dialog synchronously: the callback is invoked with the result
 * before this function returns, and the previously focused window regains
 * key status afterwards.
 *
 * @param options  The dialog configuration.
 * @param callback Invoked synchronously with the outcome once the dialog is dismissed.
 */
void OpenFileDialog(OpenFileDialogOptions const& options, OpenFileDialogCallback const& callback);

/** Invoked with the chosen path when the save dialog is confirmed. */
using SaveFileDialogCallback = std::function<void(std::string const&)>;

/** Configuration for SaveFileDialog; currently empty (the dialog is not implemented yet). */
struct SaveFileDialogOptions {

};

/**
 * Shows a modal save-file dialog.
 *
 * Note: not implemented yet — the macOS backend is a stub and the callback
 * is never invoked.
 *
 * @param options  The dialog configuration; currently unused (not implemented).
 * @param callback Never invoked; the dialog is not implemented yet.
 */
void SaveFileDialog(SaveFileDialogOptions const& options, SaveFileDialogCallback const& callback);

} /* namespace Rocket */
