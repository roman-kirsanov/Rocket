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
#include <Rocket/UI/Input.hpp>
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
#include <Pages/InputPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _InputPageState {
    InputProps input;
    std::string value;
    int enters = 0;
};

static auto const _intents = std::vector<std::tuple<InputIntent, std::string>>{
    { InputIntent::Default, "Default" },
    { InputIntent::Primary, "Primary" },
    { InputIntent::Success, "Success" },
    { InputIntent::Warning, "Warning" },
    { InputIntent::Danger, "Danger" }
};
static auto const _sizes = std::vector<std::tuple<InputSize, std::string>>{
    { InputSize::Small, "Small" },
    { InputSize::Medium, "Medium" },
    { InputSize::Large, "Large" }
};
static auto const _placeholder = std::string("Your name");
static auto const _previewWidth = 280.0f;
static auto const _statusMarginTop = 16.0f;

static std::string _GetCode(_InputPageState const& state) {
    auto fields = std::vector<std::string>{
        ".value = state.name",
        std::format(".placeholder = \"{}\"", _placeholder)
    };

    if (state.input.intent.has_value()) {
        fields.push_back(std::format(".intent = InputIntent::{}", GetOptionName(_intents, state.input.intent.value())));
    }

    if (state.input.size.has_value()) {
        fields.push_back(std::format(".size = InputSize::{}", GetOptionName(_sizes, state.input.size.value())));
    }

    if (state.input.disabled.value_or(false)) {
        fields.push_back(".disabled = true");
    }

    if (state.input.password.value_or(false)) {
        fields.push_back(".password = true");
    }

    fields.push_back(".onChange = [&](std::string const& text) { state.name = text; }");

    auto code = std::string("Input({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void InputPage(InputPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_InputPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Input" });

    Text({
        .content = "A one-line text field, with an optional prefix and suffix inside its edge. The caret, the selection, "
                   "the editing keys, the clipboard and undo are those of an editable Node. Each edit calls onChange "
                   "with the whole text, and Enter calls onEnter. Its look comes from InputTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto input = state.input;
            input.value = state.value;
            input.placeholder = _placeholder;
            input.onChange = [&](std::string const& text) {
                state.value = text;
            };
            input.onEnter = [&]{
                state.enters++;
            };
            input.nodeProps.width = _previewWidth;

            Node({ .direction = NodeDirection::Vertical, .alignment = NodeAlignment::Center }, [&]{
                Input(input);

                Text({
                    .content = std::format("{} characters, Enter pressed {} {}", state.value.size(), state.enters, (state.enters == 1 ? "time" : "times")),
                    .size = TextSize::Small,
                    .nodeProps = {
                        .marginTop = _statusMarginTop,
                        .textColor = stage.text
                    }
                });
            });
        },
        .controls = [&]{
            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.input.intent.value_or(InputIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.input.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.input.size.value_or(InputSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.input.size = std::get<0>(_sizes[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "disabled",
                    .checked = state.input.disabled.value_or(false),
                    .onChange = [&](bool checked) {
                        state.input.disabled = checked;
                    }
                });

                Switch({
                    .label = "password",
                    .checked = state.input.password.value_or(false),
                    .onChange = [&](bool checked) {
                        state.input.password = checked;
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
