/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Select.hpp>
#include <Rocket/UI/Switch.hpp>
#include <Rocket/UI/Theme/PixelDark/Material.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Options.hpp>
#include <Pages/MaterialPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _MaterialPageState {
    MaterialVariantOptions options;
};

static auto const _kinds = std::vector<std::tuple<MaterialKind, std::string>>{
    { MaterialKind::Default, "Default" },
    { MaterialKind::Bordered, "Bordered" },
    { MaterialKind::Lowered, "Lowered" },
    { MaterialKind::Raised, "Raised" },
    { MaterialKind::Outlined, "Outlined" },
    { MaterialKind::Flat, "Flat" },
    { MaterialKind::FlatOutlined, "FlatOutlined" },
    { MaterialKind::Transparent, "Transparent" },
    { MaterialKind::TransparentOutlined, "TransparentOutlined" }
};
static auto const _intents = std::vector<std::tuple<MaterialIntent, std::string>>{
    { MaterialIntent::Default, "Default" },
    { MaterialIntent::Primary, "Primary" },
    { MaterialIntent::Success, "Success" },
    { MaterialIntent::Warning, "Warning" },
    { MaterialIntent::Danger, "Danger" }
};
static auto const _brightnesses = std::vector<std::tuple<MaterialBrightness, std::string>>{
    { MaterialBrightness::Normal, "Normal" },
    { MaterialBrightness::Lighter, "Lighter" },
    { MaterialBrightness::Darker, "Darker" }
};
static auto const _roundnesses = std::vector<std::tuple<MaterialRoundness, std::string>>{
    { MaterialRoundness::None, "None" },
    { MaterialRoundness::Small, "Small" },
    { MaterialRoundness::Large, "Large" }
};
static auto const _inheritName = std::string("Inherit");
static auto const _scales = std::vector<float>{ 1.0f, 1.25f, 1.5f, 2.0f, 3.0f };
static auto const _previewSize = Vec2{ 240.0f, 96.0f };

static std::string _FormatScale(float scale) {
    auto text = std::format("{}", scale);

    if (text.find('.') == std::string::npos) {
        text += ".0";
    }

    return (text + "f");
}

static std::string _GetCode(MaterialVariantOptions const& options) {
    auto fields = std::vector<std::string>();

    if (options.kind.has_value()) {
        fields.push_back(std::format(".kind = MaterialKind::{}", GetOptionName(_kinds, options.kind.value())));
    }

    if (options.intent.has_value()) {
        fields.push_back(std::format(".intent = MaterialIntent::{}", GetOptionName(_intents, options.intent.value())));
    }

    if (options.brightness.has_value()) {
        fields.push_back(std::format(".brightness = MaterialBrightness::{}", GetOptionName(_brightnesses, options.brightness.value())));
    }

    if (options.roundness.has_value()) {
        fields.push_back(std::format(".roundness = MaterialRoundness::{}", GetOptionName(_roundnesses, options.roundness.value())));
    }

    for (auto const& [name, roundness] : std::vector<std::tuple<std::string, std::optional<MaterialRoundness>>>{
        { "topLeftRoundness", options.topLeftRoundness },
        { "topRightRoundness", options.topRightRoundness },
        { "bottomLeftRoundness", options.bottomLeftRoundness },
        { "bottomRightRoundness", options.bottomRightRoundness }
    }) {
        if (roundness.has_value()) {
            fields.push_back(std::format(".{} = MaterialRoundness::{}", name, GetOptionName(_roundnesses, roundness.value())));
        }
    }

    if (options.scale.has_value()) {
        fields.push_back(std::format(".scale = {}", _FormatScale(options.scale.value())));
    }

    for (auto const& [name, enabled] : std::vector<std::tuple<std::string, std::optional<bool>>>{
        { "noLeftBorder", options.noLeftBorder },
        { "noTopBorder", options.noTopBorder },
        { "noRightBorder", options.noRightBorder },
        { "noBottomBorder", options.noBottomBorder }
    }) {
        if (enabled.value_or(false)) {
            fields.push_back(std::format(".{} = true", name));
        }
    }

    if (fields.empty()) {
        return "auto const material = GetPixelDarkMaterial().getVariant({});";
    }

    auto code = std::string("auto const material = GetPixelDarkMaterial().getVariant({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void MaterialPage(MaterialPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_MaterialPageState>();
    auto& previewRef = UseRef<class Node>();
    auto const& document = UseContext<class Document>();

    auto const scale = document.getScale();
    auto const isHover = (previewRef != nullptr && previewRef->isHover());
    auto const isActive = (previewRef != nullptr && previewRef->isActive());
    auto previewOptions = state.options;
    previewOptions.brightness = (
        isActive ? MaterialBrightness::Darker :
        isHover ? MaterialBrightness::Lighter :
                  state.options.brightness.value_or(MaterialBrightness::Normal)
    );
    auto cornerNames = GetOptionNames(_roundnesses);
    cornerNames.insert(cornerNames.begin(), _inheritName);

    auto const cornerIndex = [](std::optional<MaterialRoundness> const& roundness) {
        return (roundness.has_value() ? (GetOptionIndex(_roundnesses, roundness.value()) + 1) : 0llu);
    };
    auto const cornerRoundness = [](std::size_t index) {
        return (index == 0 ? std::nullopt : std::optional(std::get<0>(_roundnesses[index - 1])));
    };

    Title({ .content = "Material" });

    Text({
        .content = "Brushes for pixel-art panels, cut from a material: a sprite sheet drawn at 1x, 2x and 3x, every variant in three "
                   "brightnesses. getVariant returns one nine-patched ImageBrush, ready to be the background of a Node.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            Node({
                .ref = &previewRef,
                .width = _previewSize.width,
                .height = _previewSize.height,
                .cursor = Cursor::Pointer,
                .background = GetPixelDarkMaterial().getVariant(previewOptions).brush
            });
        },
        .controls = [&]{
            PlaygroundControl({ .label = "kind" }, [&]{
                Select({
                    .options = GetOptionNames(_kinds),
                    .selected = GetOptionIndex(_kinds, state.options.kind.value_or(MaterialKind::Default)),
                    .onChange = [&](std::size_t index) {
                        state.options.kind = std::get<0>(_kinds[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.options.intent.value_or(MaterialIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.options.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "brightness" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_brightnesses),
                    .selected = GetOptionIndex(_brightnesses, state.options.brightness.value_or(MaterialBrightness::Normal)),
                    .onChange = [&](std::size_t index) {
                        state.options.brightness = std::get<0>(_brightnesses[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "roundness" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_roundnesses),
                    .selected = GetOptionIndex(_roundnesses, state.options.roundness.value_or(MaterialRoundness::None)),
                    .onChange = [&](std::size_t index) {
                        state.options.roundness = std::get<0>(_roundnesses[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "topLeftRoundness" }, [&]{
                SegmentedControl({
                    .options = cornerNames,
                    .selected = cornerIndex(state.options.topLeftRoundness),
                    .onChange = [&, cornerRoundness](std::size_t index) {
                        state.options.topLeftRoundness = cornerRoundness(index);
                    }
                });
            });

            PlaygroundControl({ .label = "topRightRoundness" }, [&]{
                SegmentedControl({
                    .options = cornerNames,
                    .selected = cornerIndex(state.options.topRightRoundness),
                    .onChange = [&, cornerRoundness](std::size_t index) {
                        state.options.topRightRoundness = cornerRoundness(index);
                    }
                });
            });

            PlaygroundControl({ .label = "bottomLeftRoundness" }, [&]{
                SegmentedControl({
                    .options = cornerNames,
                    .selected = cornerIndex(state.options.bottomLeftRoundness),
                    .onChange = [&, cornerRoundness](std::size_t index) {
                        state.options.bottomLeftRoundness = cornerRoundness(index);
                    }
                });
            });

            PlaygroundControl({ .label = "bottomRightRoundness" }, [&]{
                SegmentedControl({
                    .options = cornerNames,
                    .selected = cornerIndex(state.options.bottomRightRoundness),
                    .onChange = [&, cornerRoundness](std::size_t index) {
                        state.options.bottomRightRoundness = cornerRoundness(index);
                    }
                });
            });

            PlaygroundControl({ .label = "scale" }, [&]{
                auto labels = std::vector<std::string>();

                for (auto const option : _scales) {
                    labels.push_back(std::format("{}", option));
                }

                SegmentedControl({
                    .options = labels,
                    .selected = static_cast<std::size_t>(std::ranges::find(_scales, state.options.scale.value_or(1.0f)) - _scales.begin()),
                    .onChange = [&](std::size_t index) {
                        state.options.scale = _scales[index];
                    }
                });
            });

            PlaygroundControl({ .label = "edges" }, [&]{
                Switch({
                    .label = "noLeftBorder",
                    .checked = state.options.noLeftBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.options.noLeftBorder = checked;
                    }
                });

                Switch({
                    .label = "noTopBorder",
                    .checked = state.options.noTopBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.options.noTopBorder = checked;
                    }
                });

                Switch({
                    .label = "noRightBorder",
                    .checked = state.options.noRightBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.options.noRightBorder = checked;
                    }
                });

                Switch({
                    .label = "noBottomBorder",
                    .checked = state.options.noBottomBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.options.noBottomBorder = checked;
                    }
                });
            });
        },
        .code = _GetCode(state.options)
    });
}

} /* namespace Storybook */
