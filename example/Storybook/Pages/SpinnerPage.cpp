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
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Spinner.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Options.hpp>
#include <Pages/SpinnerPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _SpinnerPageState {
    SpinnerProps spinner;
};

static auto const _sizes = std::vector<std::tuple<SpinnerSize, std::string>>{
    { SpinnerSize::Small, "Small" },
    { SpinnerSize::Medium, "Medium" },
    { SpinnerSize::Large, "Large" }
};

static std::string _GetCode(_SpinnerPageState const& state) {
    if (state.spinner.size.has_value()) {
        return std::format("Spinner({{ .size = SpinnerSize::{} }});", GetOptionName(_sizes, state.spinner.size.value()));
    }

    return "Spinner({});";
}

void SpinnerPage(SpinnerPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SpinnerPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Spinner" });

    Text({
        .content = "The theme's drawing of a spinner, to show that something is loading. It does not turn yet: nodes "
                   "cannot be rotated. Its drawing, color and size come from SpinnerTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            Spinner(state.spinner);
        },
        .controls = [&]{
            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.spinner.size.value_or(SpinnerSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.spinner.size = std::get<0>(_sizes[index]);
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
