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
#include <Rocket/UI/Radiobox.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Switch.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Options.hpp>
#include <Pages/RadioboxPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _RadioboxPageState {
    RadioboxProps radiobox;
    std::size_t choice = 0;
};

static auto const _choices = std::vector<std::string>{ "Email", "Phone", "Mail" };
static auto const _intents = std::vector<std::tuple<RadioboxIntent, std::string>>{
    { RadioboxIntent::Default, "Default" },
    { RadioboxIntent::Primary, "Primary" },
    { RadioboxIntent::Success, "Success" },
    { RadioboxIntent::Warning, "Warning" },
    { RadioboxIntent::Danger, "Danger" }
};
static auto const _sizes = std::vector<std::tuple<RadioboxSize, std::string>>{
    { RadioboxSize::Small, "Small" },
    { RadioboxSize::Medium, "Medium" },
    { RadioboxSize::Large, "Large" }
};
static auto const _choiceGap = 10.0f;

static std::string _GetCode(_RadioboxPageState const& state) {
    auto fields = std::vector<std::string>{
        ".key = choices[index]",
        ".label = choices[index]",
        ".checked = (state.choice == index)"
    };

    if (state.radiobox.intent.has_value()) {
        fields.push_back(std::format(".intent = RadioboxIntent::{}", GetOptionName(_intents, state.radiobox.intent.value())));
    }

    if (state.radiobox.size.has_value()) {
        fields.push_back(std::format(".size = RadioboxSize::{}", GetOptionName(_sizes, state.radiobox.size.value())));
    }

    if (state.radiobox.disabled.value_or(false)) {
        fields.push_back(".disabled = true");
    }

    fields.push_back(".onChange = [&, index](bool) { state.choice = index; }");

    auto code = std::string("for (auto index = 0llu; index < choices.size(); index++) {\n    Radiobox({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("        {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "    });\n}");
}

void RadioboxPage(RadioboxPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_RadioboxPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Radiobox" });

    Text({
        .content = "A round box with an optional label: one option of a group. It asks to be checked when clicked or on Space "
                   "and stays checked when clicked again; the caller keeps the choice of the group and passes checked to "
                   "each option. Its look comes from RadioboxTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            Node({
                .direction = NodeDirection::Vertical,
                .alignment = NodeAlignment::Start,
                .gapY = _choiceGap
            }, [&]{
                for (auto index = 0llu; index < _choices.size(); index++) {
                    auto radiobox = state.radiobox;
                    radiobox.key = _choices[index];
                    radiobox.label = _choices[index];
                    radiobox.checked = (state.choice == index);
                    radiobox.onChange = [&, index](bool) {
                        state.choice = index;
                    };

                    Radiobox(radiobox);
                }
            });
        },
        .controls = [&]{
            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.radiobox.intent.value_or(RadioboxIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.radiobox.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.radiobox.size.value_or(RadioboxSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.radiobox.size = std::get<0>(_sizes[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "disabled",
                    .checked = state.radiobox.disabled.value_or(false),
                    .onChange = [&](bool checked) {
                        state.radiobox.disabled = checked;
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
