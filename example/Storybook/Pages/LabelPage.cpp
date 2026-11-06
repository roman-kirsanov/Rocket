/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <format>
#include <string>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Label.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Switch.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Pages/LabelPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _LabelPageState {
    bool required = true;
    bool tooltip = true;
};

static auto const _text = std::string("Email");
static auto const _tooltip = std::string("We only use it to sign you in.");

static std::string _GetCode(_LabelPageState const& state) {
    auto fields = std::vector<std::string>{ std::format(".text = \"{}\"", _text) };

    if (state.required) {
        fields.push_back(".required = true");
    }

    if (state.tooltip) {
        fields.push_back(std::format(".tooltip = \"{}\"", _tooltip));
    }

    auto code = std::string("Label({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void LabelPage(LabelPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_LabelPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Label" });

    Text({
        .content = "The label of a form field: its text, the mark of a required field, and an icon that shows a Tooltip. "
                   "Its look, the mark and the icon come from LabelTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            Label({
                .text = _text,
                .required = state.required,
                .tooltip = (state.tooltip ? std::optional<std::string>(_tooltip) : std::nullopt)
            });
        },
        .controls = [&]{
            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "required",
                    .checked = state.required,
                    .onChange = [&](bool checked) {
                        state.required = checked;
                    }
                });

                Switch({
                    .label = "tooltip",
                    .checked = state.tooltip,
                    .onChange = [&](bool checked) {
                        state.tooltip = checked;
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
