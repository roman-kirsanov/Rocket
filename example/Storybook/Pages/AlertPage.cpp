/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstddef>
#include <format>
#include <string>
#include <tuple>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Alert.hpp>
#include <Rocket/UI/Button.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Switch.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Options.hpp>
#include <Pages/AlertPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _AlertPageState {
    AlertProps alert;
    bool open = false;
};

static auto const _intents = std::vector<std::tuple<AlertIntent, std::string>>{
    { AlertIntent::Default, "Default" },
    { AlertIntent::Primary, "Primary" },
    { AlertIntent::Success, "Success" },
    { AlertIntent::Warning, "Warning" },
    { AlertIntent::Danger, "Danger" }
};
static auto const _title = std::string("Delete this file?");
static auto const _body = std::string("The file is removed from every device. This cannot be undone.");

static std::string _GetCode(_AlertPageState const& state) {
    auto fields = std::vector<std::string>();

    if (state.alert.intent.has_value()) {
        fields.push_back(std::format(".intent = AlertIntent::{}", GetOptionName(_intents, state.alert.intent.value())));
    }

    if (state.alert.showIcon.value_or(false)) {
        fields.push_back(".showIcon = true");
    }

    fields.push_back(std::format(".title = \"{}\"", _title));
    fields.push_back(std::format(".body = \"{}\"", _body));
    fields.push_back(
        ".buttons = [&]{\n"
        "        Button({ .label = \"Cancel\", .onClick = [&]{ state.open = false; } });\n"
        "        Button({ .label = \"Delete\", .intent = ButtonIntent::Danger, .onClick = [&]{ state.open = false; } });\n"
        "    }"
    );

    auto code = std::string("Alert({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void AlertPage(AlertPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_AlertPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Alert" });

    Text({
        .content = "A Modal that asks for the user's attention: an optional icon, a title and a body, and a row of "
                   "buttons under them. Closing it is up to the buttons. Its look comes from AlertTheme, over that of "
                   "ModalTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            Button({
                .label = "Show alert",
                .onClick = [&]{
                    state.open = true;
                }
            });

            if (state.open) {
                auto alert = state.alert;
                alert.title = _title;
                alert.body = _body;
                alert.buttons = [&]{
                    Button({
                        .label = "Cancel",
                        .onClick = [&]{
                            state.open = false;
                        }
                    });
                    Button({
                        .label = "Delete",
                        .intent = ButtonIntent::Danger,
                        .onClick = [&]{
                            state.open = false;
                        }
                    });
                };

                Alert(alert);
            }
        },
        .controls = [&]{
            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.alert.intent.value_or(AlertIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.alert.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "icon" }, [&]{
                Switch({
                    .label = "showIcon",
                    .checked = state.alert.showIcon.value_or(false),
                    .onChange = [&](bool checked) {
                        state.alert.showIcon = checked;
                    }
                });
            });
        },
        .code = _GetCode(state),
        .previewProps = {
            .background = ColorBrush{ stage.background }
        }
    });
}

} /* namespace Storybook */
