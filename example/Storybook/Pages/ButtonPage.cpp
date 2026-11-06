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
#include <Rocket/UI/Button.hpp>
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
#include <Pages/ButtonPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _ButtonPageState {
    ButtonProps button;
    std::size_t label = 0;
    bool icon = false;
    int clicks = 0;
};


static auto const _plusSvg = std::string(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M5 12h14M12 5v14\"/></svg>"
);
static auto const _labels = std::vector<std::tuple<std::optional<std::string>, std::string>>{
    { "Button", "Button" },
    { "Save changes", "Save changes" },
    { "Delete", "Delete" },
    { std::nullopt, "None" }
};
static auto const _intents = std::vector<std::tuple<ButtonIntent, std::string>>{
    { ButtonIntent::Default, "Default" },
    { ButtonIntent::Primary, "Primary" },
    { ButtonIntent::Success, "Success" },
    { ButtonIntent::Warning, "Warning" },
    { ButtonIntent::Danger, "Danger" }
};
static auto const _kinds = std::vector<std::tuple<ButtonKind, std::string>>{
    { ButtonKind::Default, "Default" },
    { ButtonKind::Outlined, "Outlined" },
    { ButtonKind::Transparent, "Transparent" },
    { ButtonKind::Flat, "Flat" }
};
static auto const _sizes = std::vector<std::tuple<ButtonSize, std::string>>{
    { ButtonSize::Small, "Small" },
    { ButtonSize::Medium, "Medium" },
    { ButtonSize::Large, "Large" }
};
static auto const _clicksMarginTop = 16.0f;

static std::string _GetCode(_ButtonPageState const& state) {
    auto fields = std::vector<std::string>();
    auto const& label = std::get<0>(_labels[state.label]);

    if (label.has_value()) {
        fields.push_back(std::format(".label = \"{}\"", label.value()));
    }

    if (state.icon) {
        fields.push_back(".icon = SvgIconData{ plusSvg }");
    }

    if (state.button.intent.has_value()) {
        fields.push_back(std::format(".intent = ButtonIntent::{}", GetOptionName(_intents, state.button.intent.value())));
    }

    if (state.button.kind.has_value()) {
        fields.push_back(std::format(".kind = ButtonKind::{}", GetOptionName(_kinds, state.button.kind.value())));
    }

    if (state.button.size.has_value()) {
        fields.push_back(std::format(".size = ButtonSize::{}", GetOptionName(_sizes, state.button.size.value())));
    }

    for (auto const& [name, enabled] : std::vector<std::tuple<std::string, std::optional<bool>>>{
        { "active", state.button.active },
        { "narrow", state.button.narrow },
        { "disabled", state.button.disabled },
        { "noLeftBorder", state.button.noLeftBorder },
        { "noTopBorder", state.button.noTopBorder },
        { "noRightBorder", state.button.noRightBorder },
        { "noBottomBorder", state.button.noBottomBorder }
    }) {
        if (enabled.value_or(false)) {
            fields.push_back(std::format(".{} = true", name));
        }
    }

    fields.push_back(".onClick = [&]{ ... }");

    auto code = std::string("Button({\n");

    for (auto index = 0llu; index < fields.size(); index++) {
        code += std::format("    {}{}\n", fields[index], ((index + 1) < fields.size() ? "," : ""));
    }

    return (code + "});");
}

void ButtonPage(ButtonPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_ButtonPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Button" });

    Text({
        .content = "A button that lightens when hovered, darkens while pressed, and calls onClick when clicked. Its look "
                   "comes from ButtonTheme: plain colors in the Default themes, a pixel-art material in PixelDark.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto button = state.button;
            button.label = std::get<0>(_labels[state.label]);
            button.onClick = [&]{
                state.clicks++;
            };

            if (state.icon) {
                button.icon = SvgIconData{ _plusSvg };
            }

            Button(button);

            Text({
                .content = std::format("Clicked {} {}", state.clicks, (state.clicks == 1 ? "time" : "times")),
                .size = TextSize::Small,
                .nodeProps = {
                    .marginTop = _clicksMarginTop,
                    .textColor = stage.text
                }
            });
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

            PlaygroundControl({ .label = "icon" }, [&]{
                Switch({
                    .label = "SvgIconData",
                    .checked = state.icon,
                    .onChange = [&](bool checked) {
                        state.icon = checked;
                    }
                });
            });

            PlaygroundControl({ .label = "intent" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_intents),
                    .selected = GetOptionIndex(_intents, state.button.intent.value_or(ButtonIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.button.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "kind" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_kinds),
                    .selected = GetOptionIndex(_kinds, state.button.kind.value_or(ButtonKind::Default)),
                    .onChange = [&](std::size_t index) {
                        state.button.kind = std::get<0>(_kinds[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.button.size.value_or(ButtonSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.button.size = std::get<0>(_sizes[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "active",
                    .checked = state.button.active.value_or(false),
                    .onChange = [&](bool checked) {
                        state.button.active = checked;
                    }
                });

                Switch({
                    .label = "narrow",
                    .checked = state.button.narrow.value_or(false),
                    .onChange = [&](bool checked) {
                        state.button.narrow = checked;
                    }
                });

                Switch({
                    .label = "disabled",
                    .checked = state.button.disabled.value_or(false),
                    .onChange = [&](bool checked) {
                        state.button.disabled = checked;
                    }
                });
            });

            PlaygroundControl({ .label = "edges" }, [&]{
                Switch({
                    .label = "noLeftBorder",
                    .checked = state.button.noLeftBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.button.noLeftBorder = checked;
                    }
                });

                Switch({
                    .label = "noTopBorder",
                    .checked = state.button.noTopBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.button.noTopBorder = checked;
                    }
                });

                Switch({
                    .label = "noRightBorder",
                    .checked = state.button.noRightBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.button.noRightBorder = checked;
                    }
                });

                Switch({
                    .label = "noBottomBorder",
                    .checked = state.button.noBottomBorder.value_or(false),
                    .onChange = [&](bool checked) {
                        state.button.noBottomBorder = checked;
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
