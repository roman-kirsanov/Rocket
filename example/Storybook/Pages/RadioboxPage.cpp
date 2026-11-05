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
#include <Rocket/UI/Radiobox.hpp>
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
    std::size_t intent = 1;
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
static auto const _usageCode = std::string(
    "for (auto index = 0llu; index < choices.size(); index++) {\n"
    "    Radiobox({\n"
    "        .key = choices[index],\n"
    "        .label = choices[index],\n"
    "        .checked = (state.choice == index),\n"
    "        .onChange = [&, index](bool) {\n"
    "            state.choice = index;\n"
    "        }\n"
    "    });\n"
    "}"
);
static auto const _propsRows = std::vector<PropsTableRow>{
    { "label", "std::optional<std::string>", "", "Text after the box; unset shows none." },
    { "checked", "std::optional<bool>", "false", "Whether this option is the one chosen. The caller keeps the choice of the group." },
    { "intent", "std::optional<RadioboxIntent>", "Default", "Color family: Default, Primary, Success, Warning or Danger." },
    { "size", "std::optional<RadioboxSize>", "Medium", "Small, Medium or Large: box, dot, font size and gap." },
    { "disabled", "std::optional<bool>", "false", "Fades the radiobox and ignores clicks and keys." },
    { "onChange", "std::function<void(bool)>", "", "Invoked with true on a click or Space while not checked, unless disabled; a checked radiobox stays checked." },
    { "boxProps", "NodeProps", "", "Forwarded to the round box node." },
    { "textProps", "NodeProps", "", "Forwarded to the label text node." },
    { "nodeProps", "NodeProps", "", "Forwarded to the radiobox node; any field set here wins over the theme." }
};
static auto const _selectorRows = std::vector<PropsTableRow>{
    { "intent", "std::optional<RadioboxIntent>", "", "Intent of the radiobox." },
    { "size", "std::optional<RadioboxSize>", "", "Size of the radiobox." },
    { "state", "std::optional<RadioboxTheme::State>", "", "Normal, Hover, Active or Disabled; a disabled radiobox is neither hovered nor active." },
    { "checked", "std::optional<bool>", "", "Whether the radiobox is checked." }
};
static auto const _styleRows = std::vector<PropsTableRow>{
    { "materialKind", "std::optional<MaterialKind>", "", "Draws the box as a circle of the theme's material, of this kind. Without it, the box is drawn with the plain colors below." },
    { "materialIntent", "std::optional<MaterialIntent>", "Default", "Intent of the box's material." },
    { "materialBrightness", "std::optional<MaterialBrightness>", "Normal", "Brightness of the box's material." },
    { "background", "std::optional<Vec4>", "none", "Fill color of the box, without a material." },
    { "borderColor", "std::optional<Vec4>", "none", "Edge color of the box, without a material." },
    { "borderWidth", "std::optional<float>", "0", "Edge width of the box, without a material." },
    { "boxSize", "std::optional<float>", "", "Diameter of the box." },
    { "dotMaterialKind", "std::optional<MaterialKind>", "", "Draws the dot as a circle of the theme's material, of this kind; without it, in dotBackground." },
    { "dotMaterialIntent", "std::optional<MaterialIntent>", "Default", "Intent of the dot's material." },
    { "dotMaterialBrightness", "std::optional<MaterialBrightness>", "Normal", "Brightness of the dot's material." },
    { "dotBackground", "std::optional<Vec4>", "none", "Fill color of the dot, without a material." },
    { "dotSize", "std::optional<float>", "", "Diameter of the dot in the middle of the box; unset draws none, so the themes set it for checked boxes only." },
    { "color", "std::optional<Vec4>", "", "Color of the label." },
    { "fontSize", "std::optional<float>", "", "Font size of the label." },
    { "gap", "std::optional<float>", "0", "Gap between the box and the label." },
    { "opacity", "std::optional<float>", "", "Opacity of the whole radiobox." }
};
static auto const _sectionGap = 12.0f;
static auto const _sectionMarginTop = 24.0f;
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

    _Section(
        "Usage",
        "Keep the choice of the group, pass each option whether it is the one chosen, and store the new choice in onChange.",
        [&]{
            CodeBlock({ .content = _usageCode });
        }
    );

    _Section(
        "Intent",
        "The intent picks the color of the checked box. Pick one.",
        [&]{
            Stage({}, [&]{
                for (auto index = 0llu; index < _intents.size(); index++) {
                    Radiobox({
                        .key = std::get<1>(_intents[index]),
                        .label = std::get<1>(_intents[index]),
                        .checked = (state.intent == index),
                        .intent = std::get<0>(_intents[index]),
                        .onChange = [&, index](bool) {
                            state.intent = index;
                        }
                    });
                }
            });
        }
    );

    _Section(
        "Size",
        "The size sets the box, the dot, the font size and the gap, all read from the theme.",
        [&]{
            Stage({}, [&]{
                for (auto const& [size, name] : _sizes) {
                    Radiobox({ .key = name, .label = name, .checked = true, .size = size });
                }
            });
        }
    );

    _Section(
        "States",
        "disabled fades the radiobox and ignores clicks and keys. An enabled radiobox takes focus, and Space checks it.",
        [&]{
            Stage({}, [&]{
                Radiobox({ .label = "Unchecked" });
                Radiobox({ .label = "Checked", .checked = true });
                Radiobox({ .label = "Disabled", .disabled = true });
                Radiobox({ .label = "Checked and disabled", .checked = true, .disabled = true });
            });
        }
    );

    _Section(
        "Theme",
        "Every color, size and material choice comes from RadioboxTheme, the radiobox member of Theme, a list of rules "
        "resolved as Button's are. Its selector adds checked, so a rule can style a checked box alone. In PixelDark the "
        "box and the dot are material circles. Below, Default Dark and PixelDark, then Default Light.",
        [&]{
            Stage({ .colors = GetDarkStageColors() }, [&]{
                Context(GetDefaultDarkTheme(), [&]{
                    Radiobox({ .label = "Default Dark", .checked = true });
                    Radiobox({ .label = "Off" });
                });

                Context(GetPixelDarkTheme(), [&]{
                    Radiobox({ .label = "PixelDark", .checked = true });
                    Radiobox({ .label = "Off" });
                });
            });

            Context(GetDefaultLightTheme(), [&]{
                Stage({ .colors = GetLightStageColors() }, [&]{
                    Radiobox({ .label = "Default Light", .checked = true });
                    Radiobox({ .label = "Off" });
                });
            });
        }
    );

    _Section(
        "RadioboxProps",
        "Every field is optional; an unset field takes its default from the theme.",
        [&]{
            PropsTable({ .rows = _propsRows });
        }
    );

    _Section(
        "RadioboxTheme::Selector",
        "What a rule matches. The more fields it sets, the more specific the rule.",
        [&]{
            PropsTable({ .rows = _selectorRows });
        }
    );

    _Section(
        "RadioboxTheme::Style",
        "What a rule sets. Every field cascades on its own.",
        [&]{
            PropsTable({ .rows = _styleRows });
        }
    );
}

} /* namespace Storybook */
