/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstddef>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Switch.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <Components/CodeBlock.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/PropsTable.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Select.hpp>
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
    std::vector<bool> intents = { true, true, true, true, true };
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
static auto const _usageCode = std::string(
    "Switch({\n"
    "    .label = \"Wi-Fi\",\n"
    "    .checked = state.wifi,\n"
    "    .onChange = [&](bool checked) {\n"
    "        state.wifi = checked;\n"
    "    }\n"
    "});"
);
static auto const _propsRows = std::vector<PropsTableRow>{
    { "label", "std::optional<std::string>", "", "Text after the track; unset shows none." },
    { "checked", "std::optional<bool>", "false", "Whether the switch is on. The switch keeps no value of its own." },
    { "intent", "std::optional<SwitchIntent>", "Default", "Color family: Default, Primary, Success, Warning or Danger." },
    { "size", "std::optional<SwitchSize>", "Medium", "Small, Medium or Large: track, knob, font size and gap." },
    { "disabled", "std::optional<bool>", "false", "Fades the switch and ignores clicks and keys." },
    { "onChange", "std::function<void(bool)>", "", "Invoked on a click or Space, unless disabled, with the value to take: the opposite of checked." },
    { "trackProps", "NodeProps", "", "Forwarded to the track node." },
    { "textProps", "NodeProps", "", "Forwarded to the label text node." },
    { "nodeProps", "NodeProps", "", "Forwarded to the switch node; any field set here wins over the theme." }
};
static auto const _selectorRows = std::vector<PropsTableRow>{
    { "intent", "std::optional<SwitchIntent>", "", "Intent of the switch." },
    { "size", "std::optional<SwitchSize>", "", "Size of the switch." },
    { "state", "std::optional<SwitchTheme::State>", "", "Normal, Hover, Active or Disabled; a disabled switch is neither hovered nor active." },
    { "checked", "std::optional<bool>", "", "Whether the switch is on." }
};
static auto const _styleRows = std::vector<PropsTableRow>{
    { "materialKind", "std::optional<MaterialKind>", "", "Draws the track as a capsule of the theme's material, of this kind. Without it, the track is drawn with the plain colors below." },
    { "materialIntent", "std::optional<MaterialIntent>", "Default", "Intent of the track's material." },
    { "materialBrightness", "std::optional<MaterialBrightness>", "Normal", "Brightness of the track's material." },
    { "background", "std::optional<Vec4>", "none", "Fill color of the track, without a material." },
    { "borderColor", "std::optional<Vec4>", "none", "Edge color of the track, without a material." },
    { "borderWidth", "std::optional<float>", "0", "Edge width of the track, without a material." },
    { "trackSize", "std::optional<Vec2>", "", "Width and height of the track; its ends are half circles." },
    { "knobMaterialKind", "std::optional<MaterialKind>", "", "Draws the knob as a circle of the theme's material, of this kind; without it, in knobBackground." },
    { "knobMaterialIntent", "std::optional<MaterialIntent>", "Default", "Intent of the knob's material." },
    { "knobMaterialBrightness", "std::optional<MaterialBrightness>", "Normal", "Brightness of the knob's material." },
    { "knobBackground", "std::optional<Vec4>", "none", "Fill color of the knob, without a material." },
    { "knobSize", "std::optional<float>", "", "Diameter of the knob; it sits as far from the ends of the track as from its sides." },
    { "color", "std::optional<Vec4>", "", "Color of the label." },
    { "fontSize", "std::optional<float>", "", "Font size of the label." },
    { "gap", "std::optional<float>", "0", "Gap between the track and the label." },
    { "opacity", "std::optional<float>", "", "Opacity of the whole switch." }
};
static auto const _sectionGap = 12.0f;
static auto const _sectionMarginTop = 24.0f;

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

static void _Section(std::string const& title, std::string const& description, std::function<void()> const& demo) {
    Node({
        .key = title,
        .direction = NodeDirection::Vertical,
        .alignment = NodeAlignment::Stretch,
        .marginTop = _sectionMarginTop,
        .gapY = _sectionGap
    }, [&]{
        Title({ .content = title, .order = TitleOrder::H2 });
        Text({ .content = description });
        demo();
    });
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

    _Section(
        "Usage",
        "Keep the value, pass it as checked and store the new one in onChange.",
        [&]{
            CodeBlock({ .content = _usageCode });
        }
    );

    _Section(
        "Intent",
        "The intent picks the color of the track of a switch that is on. Click them to toggle.",
        [&]{
            Stage({}, [&]{
                for (auto index = 0llu; index < _intents.size(); index++) {
                    Switch({
                        .key = std::get<1>(_intents[index]),
                        .label = std::get<1>(_intents[index]),
                        .checked = state.intents[index],
                        .intent = std::get<0>(_intents[index]),
                        .onChange = [&, index](bool checked) {
                            state.intents[index] = checked;
                        }
                    });
                }
            });
        }
    );

    _Section(
        "Size",
        "The size sets the track, the knob, the font size and the gap, all read from the theme.",
        [&]{
            Stage({}, [&]{
                for (auto const& [size, name] : _sizes) {
                    Switch({ .key = name, .label = name, .checked = true, .size = size });
                }
            });
        }
    );

    _Section(
        "States",
        "disabled fades the switch and ignores clicks and keys. An enabled switch takes focus, and Space toggles it.",
        [&]{
            Stage({}, [&]{
                Switch({ .label = "Off" });
                Switch({ .label = "On", .checked = true });
                Switch({ .label = "Disabled", .disabled = true });
                Switch({ .label = "On and disabled", .checked = true, .disabled = true });
            });
        }
    );

    _Section(
        "Theme",
        "Every color, size and material choice comes from SwitchTheme, the switch_ member of Theme (switch is a "
        "keyword of C++), a list of rules resolved as Button's are. Its selector adds checked, so a rule can style a "
        "switch that is on alone. In PixelDark the track is a material capsule and the knob a material circle. Below, "
        "Default Dark and PixelDark, then Default Light.",
        [&]{
            Stage({ .colors = GetDarkStageColors() }, [&]{
                Context(GetDefaultDarkTheme(), [&]{
                    Switch({ .label = "Default Dark", .checked = true });
                    Switch({ .label = "Off" });
                });

                Context(GetPixelDarkTheme(), [&]{
                    Switch({ .label = "PixelDark", .checked = true });
                    Switch({ .label = "Off" });
                });
            });

            Context(GetDefaultLightTheme(), [&]{
                Stage({ .colors = GetLightStageColors() }, [&]{
                    Switch({ .label = "Default Light", .checked = true });
                    Switch({ .label = "Off" });
                });
            });
        }
    );

    _Section(
        "SwitchProps",
        "Every field is optional; an unset field takes its default from the theme.",
        [&]{
            PropsTable({ .rows = _propsRows });
        }
    );

    _Section(
        "SwitchTheme::Selector",
        "What a rule matches. The more fields it sets, the more specific the rule.",
        [&]{
            PropsTable({ .rows = _selectorRows });
        }
    );

    _Section(
        "SwitchTheme::Style",
        "What a rule sets. Every field cascades on its own.",
        [&]{
            PropsTable({ .rows = _styleRows });
        }
    );
}

} /* namespace Storybook */
