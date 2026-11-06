/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Checkbox.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Select.hpp>
#include <Rocket/UI/Switch.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Options.hpp>
#include <Pages/CheckboxPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _CheckboxPageState {
    CheckboxProps checkbox;
    std::size_t label = 0;
};

static auto const _labels = std::vector<std::tuple<std::optional<std::string>, std::string>>{
    { "Remember me", "Remember me" },
    { "Send me a copy", "Send me a copy" },
    { std::nullopt, "None" }
};
static auto const _intents = std::vector<std::tuple<CheckboxIntent, std::string>>{
    { CheckboxIntent::Default, "Default" },
    { CheckboxIntent::Primary, "Primary" },
    { CheckboxIntent::Success, "Success" },
    { CheckboxIntent::Warning, "Warning" },
    { CheckboxIntent::Danger, "Danger" }
};
static auto const _sizes = std::vector<std::tuple<CheckboxSize, std::string>>{
    { CheckboxSize::Small, "Small" },
    { CheckboxSize::Medium, "Medium" },
    { CheckboxSize::Large, "Large" }
};

static std::string _GetCode(_CheckboxPageState const& state) {
    auto fields = std::vector<std::string>();
    auto const& label = std::get<0>(_labels[state.label]);

    if (label.has_value()) {
        fields.push_back(std::format(".label = \"{}\"", label.value()));
    }

    fields.push_back(std::format(".checked = {}", state.checkbox.checked.value_or(false)));

    if (state.checkbox.intent.has_value()) {
        fields.push_back(std::format(".intent = CheckboxIntent::{}", GetOptionName(_intents, state.checkbox.intent.value())));
    }

    if (state.checkbox.size.has_value()) {
        fields.push_back(std::format(".size = CheckboxSize::{}", GetOptionName(_sizes, state.checkbox.size.value())));
    }

    if (state.checkbox.disabled.value_or(false)) {
        fields.push_back(".disabled = true");
    }

    fields.push_back(".onChange = [&](bool checked) { ... }");

    auto code = std::string("Checkbox({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void CheckboxPage(CheckboxPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_CheckboxPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Checkbox" });

    Text({
        .content = "A box with an optional label that asks to be checked or unchecked when clicked or on Space. It keeps no "
                   "value of its own: the caller passes checked and sets it in onChange. Its look comes from CheckboxTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto checkbox = state.checkbox;
            checkbox.label = std::get<0>(_labels[state.label]);
            checkbox.onChange = [&](bool checked) {
                state.checkbox.checked = checked;
            };

            Checkbox(checkbox);
        },
        .controls = [&]{
            PlaygroundControl({ .label = "label" }, [&]{
                Select({
                    .options = GetOptionNames(_labels),
                    .selected = state.label,
                    .onChange = [&](std::size_t index) {
                        state.label = index;
                    }
                });
            });

            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.checkbox.intent.value_or(CheckboxIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.checkbox.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.checkbox.size.value_or(CheckboxSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.checkbox.size = std::get<0>(_sizes[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "checked",
                    .checked = state.checkbox.checked.value_or(false),
                    .onChange = [&](bool checked) {
                        state.checkbox.checked = checked;
                    }
                });

                Switch({
                    .label = "disabled",
                    .checked = state.checkbox.disabled.value_or(false),
                    .onChange = [&](bool checked) {
                        state.checkbox.disabled = checked;
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
