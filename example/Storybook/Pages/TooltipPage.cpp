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
#include <Rocket/UI/Button.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Tooltip.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Options.hpp>
#include <Pages/TooltipPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _TooltipPageState {
    TooltipProps tooltip;
};

static auto const _directions = std::vector<std::tuple<TooltipDirection, std::string>>{
    { TooltipDirection::Up, "Up" },
    { TooltipDirection::Down, "Down" },
    { TooltipDirection::Left, "Left" },
    { TooltipDirection::Right, "Right" }
};
static auto const _alignments = std::vector<std::tuple<TooltipAlignment, std::string>>{
    { TooltipAlignment::Start, "Start" },
    { TooltipAlignment::Center, "Center" },
    { TooltipAlignment::End, "End" }
};
static auto const _intents = std::vector<std::tuple<TooltipIntent, std::string>>{
    { TooltipIntent::Default, "Default" },
    { TooltipIntent::Primary, "Primary" },
    { TooltipIntent::Success, "Success" },
    { TooltipIntent::Warning, "Warning" },
    { TooltipIntent::Danger, "Danger" }
};
static auto const _text = std::string("Saves the file to this device");

static std::string _GetCode(_TooltipPageState const& state) {
    auto fields = std::vector<std::string>{ std::format(".text = \"{}\"", _text) };

    if (state.tooltip.direction.has_value()) {
        fields.push_back(std::format(".direction = TooltipDirection::{}", GetOptionName(_directions, state.tooltip.direction.value())));
    }

    if (state.tooltip.alignment.has_value()) {
        fields.push_back(std::format(".alignment = TooltipAlignment::{}", GetOptionName(_alignments, state.tooltip.alignment.value())));
    }

    if (state.tooltip.intent.has_value()) {
        fields.push_back(std::format(".intent = TooltipIntent::{}", GetOptionName(_intents, state.tooltip.intent.value())));
    }

    auto code = std::string("Tooltip({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "}, [&]{\n    Button({ .label = \"Save\" });\n});");
}

void TooltipPage(TooltipPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_TooltipPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Tooltip" });

    Text({
        .content = "A small Popup beside its children that shows a text once the pointer has rested on them, with an "
                   "arrow that points at them. It moves to the other side when there is no room, and hides when the "
                   "pointer leaves. Its look comes from TooltipTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto tooltip = state.tooltip;
            tooltip.text = _text;

            Tooltip(tooltip, [&]{
                Button({ .label = "Save" });
            });
        },
        .controls = [&]{
            PlaygroundControl({ .label = "direction" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_directions),
                    .selected = GetOptionIndex(_directions, state.tooltip.direction.value_or(TooltipDirection::Up)),
                    .onChange = [&](std::size_t index) {
                        state.tooltip.direction = std::get<0>(_directions[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "alignment" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_alignments),
                    .selected = GetOptionIndex(_alignments, state.tooltip.alignment.value_or(TooltipAlignment::Center)),
                    .onChange = [&](std::size_t index) {
                        state.tooltip.alignment = std::get<0>(_alignments[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.tooltip.intent.value_or(TooltipIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.tooltip.intent = std::get<0>(_intents[index]);
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
