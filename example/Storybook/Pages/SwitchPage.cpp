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
#include <Pages/SwitchPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _SwitchPageState {
    SwitchProps toggle;
    std::size_t label = 0;
};

static auto const _labels = std::vector<std::tuple<std::optional<std::string>, std::string>>{
    { "Wi-Fi", "Wi-Fi" },
    { "Notifications", "Notifications" },
    { std::nullopt, "None" }
};
static auto const _intents = std::vector<std::tuple<SwitchIntent, std::string>>{
    { SwitchIntent::Default, "Default" },
    { SwitchIntent::Primary, "Primary" },
    { SwitchIntent::Success, "Success" },
    { SwitchIntent::Warning, "Warning" },
    { SwitchIntent::Danger, "Danger" }
};
static auto const _sizes = std::vector<std::tuple<SwitchSize, std::string>>{
    { SwitchSize::Small, "Small" },
    { SwitchSize::Medium, "Medium" },
    { SwitchSize::Large, "Large" }
};

static std::string _GetCode(_SwitchPageState const& state) {
    auto fields = std::vector<std::string>();
    auto const& label = std::get<0>(_labels[state.label]);

    if (label.has_value()) {
        fields.push_back(std::format(".label = \"{}\"", label.value()));
    }

    fields.push_back(std::format(".checked = {}", state.toggle.checked.value_or(false)));

    if (state.toggle.intent.has_value()) {
        fields.push_back(std::format(".intent = SwitchIntent::{}", GetOptionName(_intents, state.toggle.intent.value())));
    }

    if (state.toggle.size.has_value()) {
        fields.push_back(std::format(".size = SwitchSize::{}", GetOptionName(_sizes, state.toggle.size.value())));
    }

    if (state.toggle.disabled.value_or(false)) {
        fields.push_back(".disabled = true");
    }

    fields.push_back(".onChange = [&](bool checked) { ... }");

    auto code = std::string("Switch({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void SwitchPage(SwitchPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SwitchPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Switch" });

    Text({
        .content = "A track with a knob that sits at its start when off and at its end when on, and an optional label. It asks "
                   "to be turned on or off when clicked or on Space and keeps no value of its own: the caller passes checked "
                   "and sets it in onChange. Its look comes from SwitchTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto toggle = state.toggle;
            toggle.label = std::get<0>(_labels[state.label]);
            toggle.onChange = [&](bool checked) {
                state.toggle.checked = checked;
            };

            Switch(toggle);
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
                    .selected = GetOptionIndex(_intents, state.toggle.intent.value_or(SwitchIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.toggle.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.toggle.size.value_or(SwitchSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.toggle.size = std::get<0>(_sizes[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "checked",
                    .checked = state.toggle.checked.value_or(false),
                    .onChange = [&](bool checked) {
                        state.toggle.checked = checked;
                    }
                });

                Switch({
                    .label = "disabled",
                    .checked = state.toggle.disabled.value_or(false),
                    .onChange = [&](bool checked) {
                        state.toggle.disabled = checked;
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
