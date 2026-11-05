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
#include <Rocket/UI/Checkbox.hpp>
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
#include <Pages/CheckboxPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _CheckboxPageState {
    CheckboxProps checkbox;
    std::size_t label = 0;
    std::vector<bool> intents = { true, true, true, true, true };
};

static auto const _labels = std::vector<std::tuple<std::optional<std::string>, std::string>>{
    { "Remember me", "Remember me" },
    { "Send me a copy", "Send me a copy" },
    { std::nullopt, "None" }
};
static auto const _intents = std::vector<std::tuple<CheckboxIntent, std::string>>{
    { CheckboxIntent::Default, "Default" },
    { CheckboxIntent::Primary, "Primary" },
    { CheckboxIntent::Success, "Success" },
    { CheckboxIntent::Warning, "Warning" },
    { CheckboxIntent::Danger, "Danger" }
};
static auto const _sizes = std::vector<std::tuple<CheckboxSize, std::string>>{
    { CheckboxSize::Small, "Small" },
    { CheckboxSize::Medium, "Medium" },
    { CheckboxSize::Large, "Large" }
};
static auto const _usageCode = std::string(
    "Checkbox({\n"
    "    .label = \"Remember me\",\n"
    "    .checked = state.remember,\n"
    "    .onChange = [&](bool checked) {\n"
    "        state.remember = checked;\n"
    "    }\n"
    "});"
);
static auto const _propsRows = std::vector<PropsTableRow>{
    { "label", "std::optional<std::string>", "", "Text after the box; unset shows none." },
    { "checked", "std::optional<bool>", "false", "Whether the box is checked. The checkbox keeps no value of its own." },
    { "intent", "std::optional<CheckboxIntent>", "Default", "Color family: Default, Primary, Success, Warning or Danger." },
    { "size", "std::optional<CheckboxSize>", "Medium", "Small, Medium or Large: box, mark, font size and gap." },
    { "disabled", "std::optional<bool>", "false", "Fades the checkbox and ignores clicks and keys." },
    { "onChange", "std::function<void(bool)>", "", "Invoked on a click or Space, unless disabled, with the value to take: the opposite of checked." },
    { "boxProps", "NodeProps", "", "Forwarded to the box node." },
    { "textProps", "NodeProps", "", "Forwarded to the label text node." },
    { "nodeProps", "NodeProps", "", "Forwarded to the checkbox node; any field set here wins over the theme." }
};
static auto const _selectorRows = std::vector<PropsTableRow>{
    { "intent", "std::optional<CheckboxIntent>", "", "Intent of the checkbox." },
    { "size", "std::optional<CheckboxSize>", "", "Size of the checkbox." },
    { "state", "std::optional<CheckboxTheme::State>", "", "Normal, Hover, Active or Disabled; a disabled checkbox is neither hovered nor active." },
    { "checked", "std::optional<bool>", "", "Whether the checkbox is checked." }
};
static auto const _styleRows = std::vector<PropsTableRow>{
    { "materialKind", "std::optional<MaterialKind>", "", "Draws the box with the theme's material, of this kind. Without it, the box is drawn with the plain colors below." },
    { "materialIntent", "std::optional<MaterialIntent>", "Default", "Intent of the material." },
    { "materialBrightness", "std::optional<MaterialBrightness>", "Normal", "Normal, Lighter or Darker brightness of the material." },
    { "materialRoundness", "std::optional<MaterialRoundness>", "None", "Corner roundness of the material." },
    { "background", "std::optional<Vec4>", "none", "Fill color of the box, without a material." },
    { "borderColor", "std::optional<Vec4>", "none", "Edge color of the box, without a material." },
    { "borderWidth", "std::optional<float>", "0", "Edge width of the box, without a material." },
    { "borderRadius", "std::optional<float>", "0", "Corner radius of the box, without a material." },
    { "boxSize", "std::optional<float>", "", "Width and height of the box." },
    { "mark", "std::optional<SvgIconData>", "", "Check mark drawn in the box; the themes set it for checked boxes only." },
    { "markColor", "std::optional<Vec4>", "", "Color of the check mark." },
    { "markSize", "std::optional<Vec2>", "", "Size of the check mark." },
    { "color", "std::optional<Vec4>", "", "Color of the label." },
    { "fontSize", "std::optional<float>", "", "Font size of the label." },
    { "gap", "std::optional<float>", "0", "Gap between the box and the label." },
    { "opacity", "std::optional<float>", "", "Opacity of the whole checkbox." }
};
static auto const _sectionGap = 12.0f;
static auto const _sectionMarginTop = 24.0f;

static std::string _GetCode(_CheckboxPageState const& state) {
    auto fields = std::vector<std::string>();
    auto const& label = std::get<0>(_labels[state.label]);

    if (label.has_value()) {
        fields.push_back(std::format(".label = \"{}\"", label.value()));
    }

    fields.push_back(std::format(".checked = {}", state.checkbox.checked.value_or(false)));

    if (state.checkbox.intent.has_value()) {
        fields.push_back(std::format(".intent = CheckboxIntent::{}", GetOptionName(_intents, state.checkbox.intent.value())));
    }

    if (state.checkbox.size.has_value()) {
        fields.push_back(std::format(".size = CheckboxSize::{}", GetOptionName(_sizes, state.checkbox.size.value())));
    }

    if (state.checkbox.disabled.value_or(false)) {
        fields.push_back(".disabled = true");
    }

    fields.push_back(".onChange = [&](bool checked) { ... }");

    auto code = std::string("Checkbox({\n");

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

void CheckboxPage(CheckboxPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_CheckboxPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Checkbox" });

    Text({
        .content = "A box with an optional label that asks to be checked or unchecked when clicked or on Space. It keeps no "
                   "value of its own: the caller passes checked and sets it in onChange. Its look comes from CheckboxTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            auto checkbox = state.checkbox;
            checkbox.label = std::get<0>(_labels[state.label]);
            checkbox.onChange = [&](bool checked) {
                state.checkbox.checked = checked;
            };

            Checkbox(checkbox);
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
                    .selected = GetOptionIndex(_intents, state.checkbox.intent.value_or(CheckboxIntent::Default)),
                    .onChange = [&](std::size_t index) {
                        state.checkbox.intent = std::get<0>(_intents[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "size" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_sizes),
                    .selected = GetOptionIndex(_sizes, state.checkbox.size.value_or(CheckboxSize::Medium)),
                    .onChange = [&](std::size_t index) {
                        state.checkbox.size = std::get<0>(_sizes[index]);
                    }
                });
            });

            PlaygroundControl({ .label = "states" }, [&]{
                Switch({
                    .label = "checked",
                    .checked = state.checkbox.checked.value_or(false),
                    .onChange = [&](bool checked) {
                        state.checkbox.checked = checked;
                    }
                });

                Switch({
                    .label = "disabled",
                    .checked = state.checkbox.disabled.value_or(false),
                    .onChange = [&](bool checked) {
                        state.checkbox.disabled = checked;
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
        "The intent picks the color of a checked box. Click them to toggle.",
        [&]{
            Stage({}, [&]{
                for (auto index = 0llu; index < _intents.size(); index++) {
                    Checkbox({
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
        "The size sets the box, the mark, the font size and the gap, all read from the theme.",
        [&]{
            Stage({}, [&]{
                for (auto const& [size, name] : _sizes) {
                    Checkbox({ .key = name, .label = name, .checked = true, .size = size });
                }
            });
        }
    );

    _Section(
        "States",
        "disabled fades the checkbox and ignores clicks and keys. An enabled checkbox takes focus, and Space toggles it.",
        [&]{
            Stage({}, [&]{
                Checkbox({ .label = "Unchecked" });
                Checkbox({ .label = "Checked", .checked = true });
                Checkbox({ .label = "Disabled", .disabled = true });
                Checkbox({ .label = "Checked and disabled", .checked = true, .disabled = true });
            });
        }
    );

    _Section(
        "Theme",
        "Every color, size and material choice comes from CheckboxTheme, the checkbox member of Theme, a list of rules "
        "resolved as Button's are. Its selector adds checked, so a rule can style a checked box alone. Below, Default "
        "Dark and PixelDark, then Default Light.",
        [&]{
            Stage({ .colors = GetDarkStageColors() }, [&]{
                Context(GetDefaultDarkTheme(), [&]{
                    Checkbox({ .label = "Default Dark", .checked = true });
                    Checkbox({ .label = "Off" });
                });

                Context(GetPixelDarkTheme(), [&]{
                    Checkbox({ .label = "PixelDark", .checked = true });
                    Checkbox({ .label = "Off" });
                });
            });

            Context(GetDefaultLightTheme(), [&]{
                Stage({ .colors = GetLightStageColors() }, [&]{
                    Checkbox({ .label = "Default Light", .checked = true });
                    Checkbox({ .label = "Off" });
                });
            });
        }
    );

    _Section(
        "CheckboxProps",
        "Every field is optional; an unset field takes its default from the theme.",
        [&]{
            PropsTable({ .rows = _propsRows });
        }
    );

    _Section(
        "CheckboxTheme::Selector",
        "What a rule matches. The more fields it sets, the more specific the rule.",
        [&]{
            PropsTable({ .rows = _selectorRows });
        }
    );

    _Section(
        "CheckboxTheme::Style",
        "What a rule sets. Every field cascades on its own.",
        [&]{
            PropsTable({ .rows = _styleRows });
        }
    );
}

} /* namespace Storybook */
