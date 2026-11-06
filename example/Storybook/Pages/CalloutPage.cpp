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
#include <Rocket/UI/Callout.hpp>
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
#include <Pages/CalloutPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _CalloutPageState {
    CalloutProps callout;
};

static auto const _intents = std::vector<std::tuple<CalloutIntent, std::string>>{
    { CalloutIntent::Default, "Default" },
    { CalloutIntent::Primary, "Primary" },
    { CalloutIntent::Success, "Success" },
    { CalloutIntent::Warning, "Warning" },
    { CalloutIntent::Danger, "Danger" }
};
static auto const _content = std::string("Your changes are saved on this device only.");
static auto const _previewWidth = 360.0f;

static std::string _GetCode(_CalloutPageState const& state) {
    auto fields = std::vector<std::string>();

    if (state.callout.intent.has_value()) {
        fields.push_back(std::format(".intent = CalloutIntent::{}", GetOptionName(_intents, state.callout.intent.value())));
    }

    if (state.callout.showIcon.value_or(false)) {
        fields.push_back(".showIcon = true");
    }

    auto code = std::string("Callout({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + std::format("}}, [&]{{\n    Node({{ .display = NodeDisplay::Text, .content = \"{}\" }});\n}});", _content));
}

void CalloutPage(CalloutPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_CalloutPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Callout" });

    Text({
        .content = "A box tinted by its intent that points something out on a page, with an optional icon before its "
                   "content. Its look, and the icon of each intent, come from CalloutTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto callout = state.callout;
            callout.nodeProps.width = _previewWidth;

            Callout(callout, [&]{
                Node({ .display = NodeDisplay::Text, .content = _content });
            });
        },
        .controls = [&]{
            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.callout.intent.value_or(CalloutIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.callout.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "icon" }, [&]{
                Switch({
                    .label = "showIcon",
                    .checked = state.callout.showIcon.value_or(false),
                    .onChange = [&](bool checked) {
                        state.callout.showIcon = checked;
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
