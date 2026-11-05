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
#include <Rocket/UI/Switch.hpp>
#include <Rocket/UI/Theme/PixelDark/Material.hpp>
#include <Components/CodeBlock.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/PropsTable.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Select.hpp>
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
static auto const _usageCode = std::string(
    "auto const& document = UseContext<class Document>();\n"
    "auto const material = GetPixelDarkMaterial().getVariant({\n"
    "    .kind = MaterialKind::Raised,\n"
    "    .intent = MaterialIntent::Primary,\n"
    "    .brightness = (isActive ? MaterialBrightness::Darker : isHover ? MaterialBrightness::Lighter : MaterialBrightness::Normal),\n"
    "    .roundness = MaterialRoundness::Small,\n"
    "    .scale = document.getScale()\n"
    "});\n"
    "\n"
    "Node({\n"
    "    .padding = material.border,\n"
    "    .background = material.brush\n"
    "});"
);
static auto const _optionsRows = std::vector<PropsTableRow>{
    { "kind", "std::optional<MaterialKind>", "Default", "Look of the material: how its fill and its edge are drawn." },
    { "intent", "std::optional<MaterialIntent>", "Default", "Color family of the material, after what it is used for." },
    { "brightness", "std::optional<MaterialBrightness>", "Normal", "Normal, Lighter or Darker: the sheet draws every variant in these three, as for rest, hover and press." },
    { "roundness", "std::optional<MaterialRoundness>", "None", "Roundness of every corner that does not set its own." },
    { "topLeftRoundness", "std::optional<MaterialRoundness>", "roundness", "Roundness of the top-left corner." },
    { "topRightRoundness", "std::optional<MaterialRoundness>", "roundness", "Roundness of the top-right corner." },
    { "bottomLeftRoundness", "std::optional<MaterialRoundness>", "roundness", "Roundness of the bottom-left corner." },
    { "bottomRightRoundness", "std::optional<MaterialRoundness>", "roundness", "Roundness of the bottom-right corner." },
    { "scale", "std::optional<float>", "1", "Device pixels per node unit. Picks the 1x, 2x or 3x sheet drawn closest to it." },
    { "noLeftBorder", "std::optional<bool>", "false", "Leaves out the left edge and both left corners, so the material joins a neighbor on that side." },
    { "noTopBorder", "std::optional<bool>", "false", "Leaves out the top edge and both top corners." },
    { "noRightBorder", "std::optional<bool>", "false", "Leaves out the right edge and both right corners." },
    { "noBottomBorder", "std::optional<bool>", "false", "Leaves out the bottom edge and both bottom corners." }
};
static auto const _materialRows = std::vector<PropsTableRow>{
    { "brush", "ImageBrush", "", "The brush, in the brightness asked for. Its image is null when that brightness draws nothing, as the normal brightness of Transparent." },
    { "border", "float", "", "Width of the drawn edge in node units at the requested scale. Use it as the padding of the node, so content never covers the edge." }
};
static auto const _circleUsageCode = std::string(
    "auto const& document = UseContext<class Document>();\n"
    "auto const circle = GetPixelDarkMaterial().getCircleVariant({\n"
    "    .kind = MaterialKind::Raised,\n"
    "    .intent = MaterialIntent::Primary,\n"
    "    .brightness = (isActive ? MaterialBrightness::Darker : isHover ? MaterialBrightness::Lighter : MaterialBrightness::Normal),\n"
    "    .width = 32.0f,\n"
    "    .scale = document.getScale()\n"
    "});\n"
    "\n"
    "Node({\n"
    "    .width = circle.size.width,\n"
    "    .height = circle.size.height,\n"
    "    .padding = circle.border,\n"
    "    .background = circle.brush\n"
    "});"
);
static auto const _circleOptionsRows = std::vector<PropsTableRow>{
    { "kind", "std::optional<MaterialKind>", "Default", "Look of the circle: the rings of the panel of the same kind." },
    { "intent", "std::optional<MaterialIntent>", "Default", "Color family of the circle." },
    { "brightness", "std::optional<MaterialBrightness>", "Normal", "Normal, Lighter or Darker, as for a panel." },
    { "width", "float", "", "Diameter in node units, rounded to whole pixels of the sheet." },
    { "scale", "std::optional<float>", "1", "Device pixels per node unit. Picks the 1x, 2x or 3x sheet drawn closest to it." }
};
static auto const _circleRows = std::vector<PropsTableRow>{
    { "brush", "ImageBrush", "", "The brush, in the brightness asked for. Its image is null when that brightness draws nothing, as the normal brightness of Transparent." },
    { "border", "float", "", "Width of the edge in node units, the same as that of the panel of the same kind." },
    { "size", "Vec2", "", "Size in node units to draw the circle at, so its pixels stay square: the width asked for, rounded to whole pixels of the sheet." }
};
static auto const _circleWidths = std::vector<float>{ 16.0f, 24.0f, 32.0f, 48.0f, 64.0f };
static auto const _circleWidth = 32.0f;
static auto const _previewSize = Vec2{ 240.0f, 96.0f };
static auto const _tileSize = Vec2{ 96.0f, 48.0f };
static auto const _tileGap = 8.0f;
static auto const _galleryGap = 16.0f;
static auto const _sectionGap = 12.0f;
static auto const _sectionMarginTop = 24.0f;
static auto const _stateLabelWidth = 180.0f;

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

static void _Tile(std::string const& key, Brush const& brush, std::string const& label) {
    Node({
        .key = key,
        .direction = NodeDirection::Vertical,
        .alignment = NodeAlignment::Center,
        .gapY = _tileGap
    }, [&]{
        Node({
            .width = _tileSize.width,
            .height = _tileSize.height,
            .background = brush
        });

        Text({ .content = label, .size = TextSize::Small, .dimmed = true });
    });
}

static void _CircleTile(std::string const& key, MaterialCircleVariant const& circle, std::string const& label) {
    Node({
        .key = key,
        .direction = NodeDirection::Vertical,
        .alignment = NodeAlignment::Center,
        .gapY = _tileGap
    }, [&]{
        Node({
            .width = circle.size.width,
            .height = circle.size.height,
            .background = circle.brush
        });

        Text({ .content = label, .size = TextSize::Small, .dimmed = true });
    });
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

    _Section(
        "Usage",
        "Ask for a material with the look, color and corners you need, at the scale of the document, so the sprite sheet "
        "is drawn pixel for pixel. Switch between the three brushes as the node is hovered and pressed, and pad the node "
        "by the border so its content stays clear of the edge.",
        [&]{
            CodeBlock({ .content = _usageCode });
        }
    );

    _Section(
        "Kind",
        "The kind sets how the fill and the edge are drawn. Transparent and TransparentOutlined draw nothing in the "
        "normal state and only show up when hovered or pressed, which suits toolbar buttons and list rows.",
        [&]{
            Node({
                .direction = NodeDirection::Vertical,
                .gapY = _galleryGap
            }, [&]{
                for (auto const& [kind, name] : _kinds) {

                    Node({
                        .key = name,
                        .direction = NodeDirection::Horizontal,
                        .alignment = NodeAlignment::Center,
                        .gapX = _galleryGap
                    }, [&]{
                        Text({ .content = name, .nodeProps = { .width = _stateLabelWidth } });
                        for (auto const& [brightness, brightnessName] : _brightnesses) {
                            _Tile(
                                brightnessName,
                                GetPixelDarkMaterial().getVariant({ .kind = kind, .brightness = brightness, .roundness = MaterialRoundness::Small, .scale = scale }).brush,
                                brightnessName
                            );
                        }
                    });
                }
            });
        }
    );

    _Section(
        "Intent",
        "The intent picks the color family. Every kind comes in every intent.",
        [&]{
            Node({
                .direction = NodeDirection::Horizontal,
                .wrap = NodeWrap::Wrap,
                .gapX = _galleryGap,
                .gapY = _galleryGap
            }, [&]{
                for (auto const& [intent, name] : _intents) {
                    _Tile(
                        name,
                        GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .intent = intent, .roundness = MaterialRoundness::Small, .scale = scale }).brush,
                        name
                    );
                }
            });
        }
    );

    _Section(
        "Roundness",
        "roundness sets every corner at once; topLeftRoundness, topRightRoundness, bottomLeftRoundness and "
        "bottomRightRoundness override it one corner at a time. A material with mixed corners is put together from the "
        "variants drawn for each corner, one quadrant per corner, and the image is built once and kept for the rest of the run.",
        [&]{
            Node({
                .direction = NodeDirection::Horizontal,
                .wrap = NodeWrap::Wrap,
                .gapX = _galleryGap,
                .gapY = _galleryGap
            }, [&]{
                for (auto const& [roundness, name] : _roundnesses) {
                    _Tile(
                        name,
                        GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = roundness, .scale = scale }).brush,
                        name
                    );
                }

                _Tile(
                    "Top Large",
                    GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::None, .topLeftRoundness = MaterialRoundness::Large, .topRightRoundness = MaterialRoundness::Large, .scale = scale }).brush,
                    "Top Large"
                );

                _Tile(
                    "Left Large",
                    GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::None, .topLeftRoundness = MaterialRoundness::Large, .bottomLeftRoundness = MaterialRoundness::Large, .scale = scale }).brush,
                    "Left Large"
                );

                _Tile(
                    "Diagonal",
                    GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::None, .topLeftRoundness = MaterialRoundness::Large, .bottomRightRoundness = MaterialRoundness::Large, .scale = scale }).brush,
                    "Diagonal"
                );
            });
        }
    );

    _Section(
        "Edges",
        "noLeftBorder, noTopBorder, noRightBorder and noBottomBorder leave out an edge together with its two corners, so "
        "materials placed side by side join into one, as in a button group or a row of tabs.",
        [&]{
            Node({
                .direction = NodeDirection::Horizontal
            }, [&]{
                Node({
                    .width = _tileSize.width,
                    .height = _tileSize.height,
                    .background = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::Small, .scale = scale, .noRightBorder = true }).brush
                });

                Node({
                    .width = _tileSize.width,
                    .height = _tileSize.height,
                    .background = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::Small, .scale = scale, .noLeftBorder = true, .noRightBorder = true }).brush
                });

                Node({
                    .width = _tileSize.width,
                    .height = _tileSize.height,
                    .background = GetPixelDarkMaterial().getVariant({ .kind = MaterialKind::Raised, .roundness = MaterialRoundness::Small, .scale = scale, .noLeftBorder = true }).brush
                });
            });
        }
    );

    _Section(
        "Scale",
        "The sprite sheet is drawn at 1x, 2x and 3x. scale picks the sheet closest to it, and border is given in node units "
        "so it matches the drawn edge at any scale. Pass document.getScale() to draw the sheet pixel for pixel; the "
        "playground leaves it at 1 by default, so on a 2x display each pixel of the sheet is drawn as four.",
        []{}
    );

    _Section(
        "Circle",
        "getCircleVariant draws a circle of any width in every kind, intent and brightness. Circles are not on the sprite "
        "sheet: each one is drawn from the colors of the panel of the same kind, with the same rings lit from the top, "
        "and built once per width. Size the node by size, so the pixels of the circle stay square.",
        [&]{
            CodeBlock({ .content = _circleUsageCode });

            Node({
                .direction = NodeDirection::Vertical,
                .gapY = _galleryGap
            }, [&]{
                for (auto const& [kind, name] : _kinds) {
                    Node({
                        .key = name,
                        .direction = NodeDirection::Horizontal,
                        .alignment = NodeAlignment::Center,
                        .gapX = _galleryGap
                    }, [&]{
                        Text({ .content = name, .nodeProps = { .width = _stateLabelWidth } });

                        for (auto const& [brightness, brightnessName] : _brightnesses) {
                            _CircleTile(
                                brightnessName,
                                GetPixelDarkMaterial().getCircleVariant({ .kind = kind, .brightness = brightness, .width = _circleWidth, .scale = scale }),
                                brightnessName
                            );
                        }
                    });
                }

                for (auto const& [intent, name] : _intents) {
                    Node({
                        .key = ("Intent " + name),
                        .direction = NodeDirection::Horizontal,
                        .alignment = NodeAlignment::End,
                        .gapX = _galleryGap
                    }, [&]{
                        Text({ .content = name, .nodeProps = { .width = _stateLabelWidth } });

                        for (auto const width : _circleWidths) {
                            _CircleTile(
                                std::format("{}", width),
                                GetPixelDarkMaterial().getCircleVariant({ .kind = MaterialKind::Raised, .intent = intent, .width = width, .scale = scale }),
                                std::format("{}", width)
                            );
                        }
                    });
                }
            });
        }
    );

    _Section(
        "MaterialVariantOptions",
        "Every field is optional; an unset field takes its default.",
        [&]{
            PropsTable({ .rows = _optionsRows });
        }
    );

    _Section(
        "MaterialVariant",
        "What getVariant returns. A combination with no drawn variant returns an empty MaterialVariant.",
        [&]{
            PropsTable({ .rows = _materialRows });
        }
    );

    _Section(
        "MaterialCircleVariantOptions",
        "Every field but width is optional; an unset field takes its default.",
        [&]{
            PropsTable({ .rows = _circleOptionsRows });
        }
    );

    _Section(
        "MaterialCircleVariant",
        "What getCircleVariant returns. A combination with no drawn panel returns an empty MaterialCircleVariant.",
        [&]{
            PropsTable({ .rows = _circleRows });
        }
    );
}

} /* namespace Storybook */
