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
#include <Rocket/UI/Button.hpp>
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
#include <Pages/ButtonPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _ButtonPageState {
    ButtonProps button;
    std::size_t label = 0;
    bool icon = false;
    int clicks = 0;
    Theme squareTheme;
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
static auto const _usageCode = std::string(
    "Button({\n"
    "    .label = \"Save changes\",\n"
    "    .intent = ButtonIntent::Primary,\n"
    "    .onClick = [&]{ save(); }\n"
    "});"
);
static auto const _themeCode = std::string(
    "struct _AppState {\n"
    "    Theme theme = _CreateTheme();\n"
    "};\n"
    "\n"
    "static Theme _CreateTheme() {\n"
    "    auto theme = GetPixelDarkTheme();\n"
    "    theme.button.rules.push_back({ .style = { .materialRoundness = MaterialRoundness::None }, .important = true });\n"
    "    theme.button.rules.push_back({ .selector = { .size = ButtonSize::Medium }, .style = { .minHeight = 36.0f } });\n"
    "    return theme;\n"
    "}\n"
    "\n"
    "Context(state.theme, [&]{\n"
    "    Button({ .label = \"Square\" });\n"
    "});"
);
static auto const _propsRows = std::vector<PropsTableRow>{
    { "label", "std::optional<std::string>", "", "Text of the label; unset shows none." },
    { "icon", "std::optional<IconData>", "", "Icon shown before the label: SvgIconData or ImageIconData." },
    { "intent", "std::optional<ButtonIntent>", "Default", "Color family: Default, Primary, Success, Warning or Danger." },
    { "kind", "std::optional<ButtonKind>", "Default", "Look: Default (filled, or raised in PixelDark), Outlined, Transparent or Flat." },
    { "size", "std::optional<ButtonSize>", "Medium", "Small, Medium or Large: padding, height, font size and icon size." },
    { "active", "std::optional<bool>", "false", "Draws the button pressed, as for a toggle that is on." },
    { "narrow", "std::optional<bool>", "false", "Pads the sides as much as the top and bottom, for an icon alone." },
    { "disabled", "std::optional<bool>", "false", "Fades the button and ignores clicks." },
    { "noLeftBorder", "std::optional<bool>", "false", "Leaves out the left edge, so the button joins a neighbor; as noTopBorder, noRightBorder and noBottomBorder." },
    { "onClick", "std::function<void()>", "", "Invoked on click, unless disabled." },
    { "textProps", "NodeProps", "", "Forwarded to the label text node." },
    { "iconProps", "IconProps", "", "Forwarded to the Icon. It takes the button's content color unless it sets an intent." },
    { "nodeProps", "NodeProps", "", "Forwarded to the button node; any field set here wins over the theme." }
};
static auto const _ruleRows = std::vector<PropsTableRow>{
    { "selector", "ButtonTheme::Selector", "", "What the rule matches; an unset field matches any value." },
    { "style", "ButtonTheme::Style", "", "What the rule sets; an unset field is left to other rules." },
    { "important", "bool", "false", "Beats every rule that is not important, whatever its selector." }
};
static auto const _selectorRows = std::vector<PropsTableRow>{
    { "kind", "std::optional<ButtonKind>", "", "Kind of the button." },
    { "intent", "std::optional<ButtonIntent>", "", "Intent of the button." },
    { "size", "std::optional<ButtonSize>", "", "Size of the button." },
    { "state", "std::optional<ButtonTheme::State>", "", "Normal, Hover, Active or Disabled; a disabled button is neither hovered nor active." },
    { "narrow", "std::optional<bool>", "", "Whether the button is narrow." }
};
static auto const _styleRows = std::vector<PropsTableRow>{
    { "materialKind", "std::optional<MaterialKind>", "", "Draws the button with the theme's material, of this kind. Without it, the button is drawn with the plain colors below." },
    { "materialIntent", "std::optional<MaterialIntent>", "Default", "Intent of the material." },
    { "materialBrightness", "std::optional<MaterialBrightness>", "Normal", "Normal, Lighter or Darker brightness of the material." },
    { "materialRoundness", "std::optional<MaterialRoundness>", "None", "Corner roundness of the material." },
    { "background", "std::optional<Vec4>", "none", "Fill color, without a material." },
    { "borderColor", "std::optional<Vec4>", "none", "Edge color, without a material." },
    { "borderWidth", "std::optional<float>", "0", "Edge width, without a material; a left-out edge has none." },
    { "borderRadius", "std::optional<float>", "0", "Corner radius, without a material; the corners on a left-out edge are square." },
    { "color", "std::optional<Vec4>", "", "Color of the label and icon." },
    { "padding", "std::optional<Vec4>", "0", "Padding as left, top, right, bottom." },
    { "gap", "std::optional<Vec2>", "0", "Gap between the icon and the label." },
    { "minHeight", "std::optional<float>", "", "Minimum height." },
    { "fontSize", "std::optional<float>", "", "Font size of the label." },
    { "iconSize", "std::optional<Vec2>", "", "Size of the icon." },
    { "opacity", "std::optional<float>", "", "Opacity of the whole button." }
};
static auto const _sectionGap = 12.0f;
static auto const _sectionMarginTop = 24.0f;
static auto const _rowGap = 12.0f;
static auto const _clicksMarginTop = 16.0f;
static auto const _kindLabelWidth = 120.0f;

static Theme _CreateSquareTheme() {
    auto theme = GetPixelDarkTheme();
    theme.button.rules.push_back({ .style = { .materialRoundness = MaterialRoundness::None }, .important = true });
    theme.button.rules.push_back({ .selector = { .size = ButtonSize::Medium }, .style = { .minHeight = 36.0f } });

    return theme;
}

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

void ButtonPage(ButtonPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_ButtonPageState>();
    auto const shouldCreateTheme = UseEffect();

    if (shouldCreateTheme) {
        state.squareTheme = _CreateSquareTheme();
    }

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

    _Section(
        "Usage",
        "Give the button a label or an icon, and a handler. Everything else is optional and falls back to the theme.",
        [&]{
            CodeBlock({ .content = _usageCode });
        }
    );

    _Section(
        "Kind and intent",
        "The kind sets how the button is drawn: Default is filled, Outlined draws an edge and colors its label, "
        "Transparent draws nothing until hovered, and Flat is lightly tinted. The intent picks the color family. "
        "Shown here in the theme picked in the sidebar; the Theme section compares the themes side by side.",
        [&]{
            Stage({ .nodeProps = { .direction = NodeDirection::Vertical, .alignment = NodeAlignment::Start } }, [&]{
                for (auto const& [kind, kindName] : _kinds) {
                    Node({
                        .key = kindName,
                        .direction = NodeDirection::Horizontal,
                        .alignment = NodeAlignment::Center,
                        .gapX = _rowGap
                    }, [&]{
                        Text({
                            .content = kindName,
                            .size = TextSize::Small,
                            .nodeProps = {
                                .width = _kindLabelWidth,
                                .textColor = stage.text
                            }
                        });

                        for (auto const& [intent, intentName] : _intents) {
                            Button({ .key = intentName, .label = intentName, .intent = intent, .kind = kind });
                        }
                    });
                }
            });
        }
    );

    _Section(
        "Size",
        "The size sets the padding, minimum height, font size and icon size, all read from the theme's metrics.",
        [&]{
            Stage({}, [&]{
                for (auto const& [size, name] : _sizes) {
                    Button({ .key = name, .label = name, .icon = SvgIconData{ _plusSvg }, .size = size });
                }
            });
        }
    );

    _Section(
        "States",
        "active draws the button pressed, as for a toggle that is on. disabled fades it and ignores clicks. narrow pads "
        "the sides like the top and bottom, which suits a button with an icon alone.",
        [&]{
            Stage({}, [&]{
                Button({ .label = "Normal" });
                Button({ .label = "Active", .active = true });
                Button({ .label = "Disabled", .disabled = true });
                Button({ .icon = SvgIconData{ _plusSvg }, .narrow = true });
            });
        }
    );

    _Section(
        "Icon",
        "icon takes SVG text or an image. An SVG icon takes the button's content color, unless iconProps gives it an "
        "intent of its own.",
        [&]{
            Stage({}, [&]{
                Button({ .label = "Add", .icon = SvgIconData{ _plusSvg } });
                Button({ .label = "Add", .icon = SvgIconData{ _plusSvg }, .intent = ButtonIntent::Primary, .kind = ButtonKind::Outlined });
                Button({ .label = "Add", .icon = SvgIconData{ _plusSvg }, .iconProps = { .intent = IconIntent::Success } });
            });
        }
    );

    _Section(
        "Groups",
        "noLeftBorder, noTopBorder, noRightBorder and noBottomBorder leave out an edge of the material, so buttons "
        "placed side by side join into one group.",
        [&]{
            Stage({ .nodeProps = { .gap = 0.0f } }, [&]{
                Button({ .label = "Left", .noRightBorder = true });
                Button({ .label = "Center", .noLeftBorder = true, .noRightBorder = true });
                Button({ .label = "Right", .noLeftBorder = true });
            });
        }
    );

    _Section(
        "Theme",
        "Every color, size and material choice comes from ButtonTheme, the button member of Theme: a list of rules, "
        "as in a CSS stylesheet. Each rule has a selector (kind, intent, size, state, narrow) and a style; where rules "
        "set the same field, an important rule wins, then the more specific one, then the later one. A style that "
        "names a material kind draws with the theme's material, as in PixelDark; otherwise the button is drawn with "
        "plain colors, as in the Default themes. The Default themes, dark and light, are built on the default palette, Tailwind's "
        "colors in steps of 50. Copy a theme, change it, keep it alive and provide it with Context; never change a "
        "theme while it is provided. Below, Default Dark, PixelDark and PixelDark with square corners, then Default "
        "Light, then every kind and intent in PixelDark.",
        [&]{
            CodeBlock({ .content = _themeCode });

            Stage({ .colors = GetDarkStageColors() }, [&]{
                Context(GetDefaultDarkTheme(), [&]{
                    Button({ .label = "Default Dark" });
                });

                Context(GetPixelDarkTheme(), [&]{
                    Button({ .label = "PixelDark" });
                });

                Context(state.squareTheme, [&]{
                    Button({ .label = "Square" });
                });
            });

            Context(GetDefaultLightTheme(), [&]{
                Stage({ .colors = GetLightStageColors() }, [&]{
                    for (auto const& [kind, kindName] : _kinds) {
                        Button({ .key = kindName, .label = kindName, .intent = ButtonIntent::Primary, .kind = kind });
                    }

                    Button({ .label = "Default Light" });
                });
            });

            Context(GetPixelDarkTheme(), [&]{
                Stage({ .colors = GetDarkStageColors(), .nodeProps = { .direction = NodeDirection::Vertical, .alignment = NodeAlignment::Start } }, [&]{
                    for (auto const& [kind, kindName] : _kinds) {
                        Node({
                            .key = kindName,
                            .direction = NodeDirection::Horizontal,
                            .alignment = NodeAlignment::Center,
                            .gapX = _rowGap
                        }, [&]{
                            Text({
                                .content = kindName,
                                .size = TextSize::Small,
                                .nodeProps = {
                                    .width = _kindLabelWidth,
                                    .textColor = stage.text
                                }
                            });

                            for (auto const& [intent, intentName] : _intents) {
                                Button({ .key = intentName, .label = intentName, .intent = intent, .kind = kind });
                            }
                        });
                    }
                });
            });
        }
    );

    _Section(
        "ButtonProps",
        "Every field is optional; an unset field takes its default from the theme.",
        [&]{
            PropsTable({ .rows = _propsRows });
        }
    );

    _Section(
        "ButtonTheme::Rule",
        "ButtonTheme is a list of rules. To change one thing in a theme, append a rule; to make sure it wins, make it "
        "important.",
        [&]{
            PropsTable({ .rows = _ruleRows });
        }
    );

    _Section(
        "ButtonTheme::Selector",
        "What a rule matches. The more fields it sets, the more specific the rule.",
        [&]{
            PropsTable({ .rows = _selectorRows });
        }
    );

    _Section(
        "ButtonTheme::Style",
        "What a rule sets. Every field cascades on its own, so a hover rule can change only the brightness.",
        [&]{
            PropsTable({ .rows = _styleRows });
        }
    );
}

} /* namespace Storybook */
