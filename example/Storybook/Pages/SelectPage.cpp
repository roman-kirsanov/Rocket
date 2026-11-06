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
#include <Pages/SelectPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _SelectPageState {
    SelectProps select;
    std::optional<std::size_t> selected;
};

static auto const _intents = std::vector<std::tuple<SelectIntent, std::string>>{
    { SelectIntent::Default, "Default" },
    { SelectIntent::Primary, "Primary" },
    { SelectIntent::Success, "Success" },
    { SelectIntent::Warning, "Warning" },
    { SelectIntent::Danger, "Danger" }
};
static auto const _sizes = std::vector<std::tuple<SelectSize, std::string>>{
    { SelectSize::Small, "Small" },
    { SelectSize::Medium, "Medium" },
    { SelectSize::Large, "Large" }
};
static auto const _fruits = std::vector<std::string>{
    "Apple", "Apricot", "Banana", "Blueberry", "Cherry", "Grape", "Kiwi", "Lemon", "Mango", "Orange", "Peach", "Pear", "Plum", "Strawberry"
};
static auto const _placeholder = std::string("Pick a fruit");
static auto const _searchPlaceholder = std::string("Search...");
static auto const _previewWidth = 240.0f;

static std::string _GetCode(_SelectPageState const& state) {
    auto fields = std::vector<std::string>{
        ".options = fruits",
        ".selected = state.fruit",
        std::format(".placeholder = \"{}\"", _placeholder)
    };

    if (state.select.intent.has_value()) {
        fields.push_back(std::format(".intent = SelectIntent::{}", GetOptionName(_intents, state.select.intent.value())));
    }

    if (state.select.size.has_value()) {
        fields.push_back(std::format(".size = SelectSize::{}", GetOptionName(_sizes, state.select.size.value())));
    }

    if (state.select.disabled.value_or(false)) {
        fields.push_back(".disabled = true");
    }

    if (state.select.searchable.value_or(false)) {
        fields.push_back(".searchable = true");
        fields.push_back(std::format(".searchPlaceholder = \"{}\"", _searchPlaceholder));
    }

    fields.push_back(".onChange = [&](std::size_t index) { state.fruit = index; }");

    auto code = std::string("Select({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void SelectPage(SelectPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SelectPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Select" });

    Text({
        .content = "A field that shows the selected option and opens a list of the options under it. The arrow keys move "
                   "through the list, Enter picks and Escape closes; a searchable select narrows the list to what is "
                   "typed. It keeps no selection of its own. Its look comes from SelectTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto select = state.select;
            select.options = _fruits;
            select.selected = state.selected;
            select.placeholder = _placeholder;
            select.searchPlaceholder = _searchPlaceholder;
            select.onChange = [&](std::size_t index) {
                state.selected = index;
            };
            select.nodeProps.width = _previewWidth;

            Select(select);
        },
        .controls = [&]{
            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.select.intent.value_or(SelectIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.select.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.select.size.value_or(SelectSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.select.size = std::get<0>(_sizes[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "searchable",
                    .checked = state.select.searchable.value_or(false),
                    .onChange = [&](bool checked) {
                        state.select.searchable = checked;
                    }
                });

                Switch({
                    .label = "disabled",
                    .checked = state.select.disabled.value_or(false),
                    .onChange = [&](bool checked) {
                        state.select.disabled = checked;
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
