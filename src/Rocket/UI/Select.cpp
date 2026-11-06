/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Input.hpp>
#include <Rocket/UI/Popup.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/Window/Input.hpp>
#include <Rocket/UI/Select.hpp>

namespace Rocket {

struct _SelectLook {
    SelectTheme::Style style;
    Brush background;
    std::optional<Brush> border;
    float borderWidth;
    float borderRadius;
};

struct _SelectState {
    std::array<_SelectLook, 4> looks;
    Brush listBackground;
    Brush listBorder;
    float listBorderWidth;
    float listBorderRadius;
    std::function<void(std::size_t)> onChange;
    std::function<void(NodeEvent const&)> nodeOnEvent;
    bool open = false;
    bool searchable = false;
    bool focusSearch = false;
    bool scrollToHighlight = false;
    std::string search;
    std::optional<std::size_t> highlight;
    std::vector<std::size_t> shown;
    Effect<
        Theme const*,
        std::optional<SelectIntent>,
        std::optional<SelectSize>,
        float
    > shouldUpdate;
};

static auto constexpr _states = std::array<SelectTheme::State, 4>{
    SelectTheme::State::Normal,
    SelectTheme::State::Hover,
    SelectTheme::State::Focus,
    SelectTheme::State::Disabled
};
static auto constexpr _transparent = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

static SelectTheme::Style _ResolveStyle(SelectTheme const& theme, SelectIntent intent, SelectSize size, SelectTheme::State state) {
    auto const rules = _GetCascade<SelectTheme::Rule>(
        theme.rules,
        [&](SelectTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.size.has_value() == false || rule.selector.size == size) &&
                (rule.selector.state.has_value() == false || rule.selector.state == state)
            );
        },
        [](SelectTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.size.has_value() ? 1 : 0) +
                (rule.selector.state.has_value() ? 1 : 0)
            );
        }
    );

    auto style = SelectTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.materialKind, rule->style.materialKind);
        _Cascade(style.materialIntent, rule->style.materialIntent);
        _Cascade(style.materialBrightness, rule->style.materialBrightness);
        _Cascade(style.materialRoundness, rule->style.materialRoundness);
        _Cascade(style.background, rule->style.background);
        _Cascade(style.borderColor, rule->style.borderColor);
        _Cascade(style.borderWidth, rule->style.borderWidth);
        _Cascade(style.borderRadius, rule->style.borderRadius);
        _Cascade(style.shadow, rule->style.shadow);
        _Cascade(style.color, rule->style.color);
        _Cascade(style.placeholderColor, rule->style.placeholderColor);
        _Cascade(style.fontSize, rule->style.fontSize);
        _Cascade(style.padding, rule->style.padding);
        _Cascade(style.minHeight, rule->style.minHeight);
        _Cascade(style.gap, rule->style.gap);
        _Cascade(style.opacity, rule->style.opacity);
        _Cascade(style.caret, rule->style.caret);
        _Cascade(style.searchIcon, rule->style.searchIcon);
        _Cascade(style.caretColor, rule->style.caretColor);
        _Cascade(style.caretSize, rule->style.caretSize);
        _Cascade(style.listMaterialKind, rule->style.listMaterialKind);
        _Cascade(style.listMaterialIntent, rule->style.listMaterialIntent);
        _Cascade(style.listMaterialRoundness, rule->style.listMaterialRoundness);
        _Cascade(style.listBackground, rule->style.listBackground);
        _Cascade(style.listBorderColor, rule->style.listBorderColor);
        _Cascade(style.listBorderWidth, rule->style.listBorderWidth);
        _Cascade(style.listBorderRadius, rule->style.listBorderRadius);
        _Cascade(style.listShadow, rule->style.listShadow);
        _Cascade(style.listPadding, rule->style.listPadding);
        _Cascade(style.listGap, rule->style.listGap);
        _Cascade(style.listMaxHeight, rule->style.listMaxHeight);
        _Cascade(style.optionPadding, rule->style.optionPadding);
        _Cascade(style.optionBorderRadius, rule->style.optionBorderRadius);
        _Cascade(style.optionColor, rule->style.optionColor);
        _Cascade(style.optionHighlightBackground, rule->style.optionHighlightBackground);
        _Cascade(style.optionHighlightColor, rule->style.optionHighlightColor);
        _Cascade(style.optionSelectedBackground, rule->style.optionSelectedBackground);
        _Cascade(style.optionSelectedColor, rule->style.optionSelectedColor);
    }

    return style;
}

static std::string _ToLower(std::string text) {
    std::ranges::transform(text, text.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    return text;
}

static void _Open(_SelectState& state, std::optional<std::size_t> const& selected) {
    state.open = true;
    state.search.clear();
    state.highlight = selected;
    state.focusSearch = state.searchable;
    state.scrollToHighlight = true;
}

static void _Close(_SelectState& state, class Document& document, class Node* node) {
    state.open = false;
    state.search.clear();
    state.highlight = std::nullopt;

    if (state.searchable && node != nullptr) {
        document.focusNode(node);
    }
}

static void _Move(_SelectState& state, int step) {
    if (state.shown.empty()) {
        return;
    }

    auto const current = std::ranges::find(state.shown, state.highlight.value_or(state.shown.size()));

    if (current == state.shown.end()) {
        state.highlight = (step > 0 ? state.shown.front() : state.shown.back());
    } else {
        auto const index = std::clamp(static_cast<int>(current - state.shown.begin()) + step, 0, static_cast<int>(state.shown.size()) - 1);
        state.highlight = state.shown[static_cast<std::size_t>(index)];
    }

    state.scrollToHighlight = true;
}

static void _Pick(_SelectState& state, class Document& document, class Node* node, std::size_t index) {
    if (state.onChange != nullptr) {
        state.onChange(index);
    }

    _Close(state, document, node);
}

void Select(SelectProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_SelectState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
    auto& listRef = UseRef<class Node>(props.listProps.ref);
    auto& contentRef = UseRef<class Node>(nullptr);
    auto& searchRef = UseRef<class Node>(nullptr);
    auto const& theme = UseTheme();

    auto const& options = props.options.value_or(std::vector<std::string>());
    auto const disabled = props.disabled.value_or(false);
    auto const selected = ((props.selected.has_value() && props.selected.value() < options.size()) ? props.selected : std::nullopt);
    auto const isHover = (nodeRef != nullptr && nodeRef->isHover());
    auto const isFocus = (state.open || (nodeRef != nullptr && nodeRef->isFocusedWithin()));

    state.onChange = props.onChange;
    state.nodeOnEvent = props.nodeProps.onEvent;
    state.searchable = props.searchable.value_or(false);

    if (disabled) {
        state.open = false;
    }

    if (state.shouldUpdate(&theme, props.intent, props.size, document.getScale())) {
        for (auto index = 0llu; index < state.looks.size(); index++) {
            auto const style = _ResolveStyle(
                theme.select,
                props.intent.value_or(SelectIntent::Default),
                props.size.value_or(SelectSize::Medium),
                _states[index]
            );
            auto& look = state.looks[index];
            look.style = style;

            if (style.materialKind.has_value() && theme.material != nullptr) {
                look.background = theme.material->getVariant({
                    .kind = style.materialKind,
                    .intent = style.materialIntent,
                    .brightness = style.materialBrightness,
                    .roundness = style.materialRoundness,
                    .scale = document.getScale()
                }).brush;
                look.border = std::nullopt;
                look.borderWidth = 0.0f;
                look.borderRadius = 0.0f;
            } else {
                look.background = ColorBrush{ style.background.value_or(_transparent) };
                look.border = ColorBrush{ style.borderColor.value_or(_transparent) };
                look.borderWidth = style.borderWidth.value_or(0.0f);
                look.borderRadius = style.borderRadius.value_or(0.0f);
            }
        }

        auto const& list = state.looks[0].style;

        if (list.listMaterialKind.has_value() && theme.material != nullptr) {
            state.listBackground = theme.material->getVariant({
                .kind = list.listMaterialKind,
                .intent = list.listMaterialIntent,
                .roundness = list.listMaterialRoundness,
                .scale = document.getScale()
            }).brush;
            state.listBorder = ColorBrush{ _transparent };
            state.listBorderWidth = 0.0f;
            state.listBorderRadius = 0.0f;
        } else {
            state.listBackground = ColorBrush{ list.listBackground.value_or(_transparent) };
            state.listBorder = ColorBrush{ list.listBorderColor.value_or(_transparent) };
            state.listBorderWidth = list.listBorderWidth.value_or(0.0f);
            state.listBorderRadius = list.listBorderRadius.value_or(0.0f);
        }
    }

    auto const search = _ToLower(state.search);
    state.shown.clear();

    for (auto index = 0llu; index < options.size(); index++) {
        if (search.empty() || _ToLower(options[index]).find(search) != std::string::npos) {
            state.shown.push_back(index);
        }
    }

    if (state.focusSearch && searchRef != nullptr) {
        document.focusNode(searchRef);
        state.focusSearch = false;
    }

    if (state.scrollToHighlight && contentRef != nullptr) {
        auto const position = std::ranges::find(state.shown, state.highlight.value_or(options.size()));
        auto row = contentRef->getFirstChild();

        for (auto index = state.shown.begin(); index != position && row != nullptr; index++) {
            row = row->getNextSibling();
        }

        if (position != state.shown.end() && row != nullptr) {
            document.scrollNodeIntoView(*row);
        }

        state.scrollToHighlight = false;
    }

    auto const& look = state.looks[
        disabled ? 3llu :
        isFocus ? 2llu :
        isHover ? 1llu :
                  0llu
    ];
    auto const& style = look.style;
    auto const padding = style.padding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    auto const searching = (state.open && state.searchable);

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.direction = node.direction.value_or(NodeDirection::Horizontal);
    node.alignment = node.alignment.value_or(NodeAlignment::Center);
    node.gapX = node.gapX.value_or(node.gap.value_or(style.gap.value_or(0.0f)));
    node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(padding.left));
    node.paddingTop = node.paddingTop.value_or(node.padding.value_or(padding.top));
    node.paddingRight = node.paddingRight.value_or(node.padding.value_or(padding.right));
    node.paddingBottom = node.paddingBottom.value_or(node.padding.value_or(padding.bottom));
    node.borderLeftWidth = node.borderLeftWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderTopWidth = node.borderTopWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderRightWidth = node.borderRightWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderBottomWidth = node.borderBottomWidth.value_or(node.borderWidth.value_or(look.borderWidth));
    node.borderRadius = node.borderRadius.value_or(look.borderRadius);
    node.background = node.background.value_or(look.background);
    node.onEvent = [&, selected](NodeEvent const& event) {
        if (state.nodeOnEvent != nullptr) {
            state.nodeOnEvent(event);
        }

        if (listRef != nullptr && listRef->containsNode(event.getNode())) {
            return;
        }

        if (event.is<MouseClickNodeEvent>()) {
            if (state.open == false) {
                _Open(state, selected);
            } else if (state.searchable == false) {
                _Close(state, document, nodeRef);
            }

            event.stopPropagation();
        } else if (auto const keyDown = event.as<KeyDownNodeEvent>()) {
            if (state.open == false) {
                if (
                    keyDown->getKeycode() == Keycode::Enter ||
                    keyDown->getKeycode() == Keycode::Space ||
                    keyDown->getKeycode() == Keycode::ArrowDown
                ) {
                    _Open(state, selected);
                    event.preventDefault();
                    event.stopPropagation();
                }
            } else if (keyDown->getKeycode() == Keycode::ArrowUp || keyDown->getKeycode() == Keycode::ArrowDown) {
                _Move(state, (keyDown->getKeycode() == Keycode::ArrowUp ? -1 : 1));
                event.preventDefault();
                event.stopPropagation();
            } else if (keyDown->getKeycode() == Keycode::Enter) {
                if (state.highlight.has_value() && std::ranges::find(state.shown, state.highlight.value()) != state.shown.end()) {
                    _Pick(state, document, nodeRef, state.highlight.value());
                }

                event.preventDefault();
                event.stopPropagation();
            } else if (keyDown->getKeycode() == Keycode::Escape) {
                _Close(state, document, nodeRef);
                event.preventDefault();
                event.stopPropagation();
            }
        }
    };

    if (look.border.has_value()) {
        node.border = node.border.value_or(look.border.value());
    }

    if (style.shadow.has_value()) {
        node.shadow = node.shadow.value_or(style.shadow.value());
    }

    if (style.minHeight.has_value()) {
        node.minHeight = node.minHeight.value_or(style.minHeight.value());
    }

    if (style.color.has_value()) {
        node.textColor = node.textColor.value_or(style.color.value());
    }

    if (style.fontSize.has_value()) {
        node.fontSize = node.fontSize.value_or(style.fontSize.value());
    }

    if (style.opacity.has_value()) {
        node.opacity = node.opacity.value_or(style.opacity.value());
    }

    if (disabled) {
        node.mouseEvents = node.mouseEvents.value_or(false);
    } else {
        node.cursor = node.cursor.value_or(Cursor::Pointer);
        node.tabIndex = node.tabIndex.value_or(1);
    }

    Node(node, [&]{
        if (props.prefix != nullptr) {
            props.prefix();
        }

        if (searching) {
            Input({
                .value = state.search,
                .placeholder = props.searchPlaceholder,
                .onChange = [&](std::string const& text) {
                    state.search = text;
                    state.highlight = std::nullopt;
                },
                .fieldProps = { .ref = &searchRef, .padding = 0.0f },
                .nodeProps = {
                    .minHeight = 0.0f,
                    .padding = 0.0f,
                    .borderWidth = 0.0f,
                    .flex = true,
                    .cursor = Cursor::Text,
                    .background = ColorBrush{ _transparent },
                    .shadow = Shadow{ .color = _transparent }
                }
            });
        } else {
            auto text = props.textProps;
            text.display = NodeDisplay::Text;
            text.content = text.content.value_or(selected.has_value() ? options[selected.value()] : props.placeholder.value_or(""));

            if (selected.has_value() == false && style.placeholderColor.has_value()) {
                text.textColor = text.textColor.value_or(style.placeholderColor.value());
            }

            Node({ .overflowX = NodeOverflow::Hidden, .direction = NodeDirection::Horizontal, .flex = true }, [&]{
                Node(text);
            });
        }

        auto const& mark = (searching && style.searchIcon.has_value() ? style.searchIcon : style.caret);

        if (mark.has_value()) {
            auto icon = IconProps();
            icon.data.emplace(mark.value());
            icon.colorValue = style.caretColor;
            icon.sizeValue = style.caretSize;

            Icon(icon);
        }

        if (props.suffix != nullptr) {
            props.suffix();
        }

        if (state.open == false) {
            return;
        }

        auto const listPadding = style.listPadding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });

        auto list = props.listProps;
        list.ref = &listRef;
        list.left = list.left.value_or(0.0f);
        list.right = list.right.value_or(0.0f);
        list.background = list.background.value_or(state.listBackground);
        list.border = list.border.value_or(state.listBorder);
        list.borderWidth = list.borderWidth.value_or(state.listBorderWidth);
        list.borderRadius = list.borderRadius.value_or(state.listBorderRadius);
        list.shadow = list.shadow.value_or(style.listShadow.value_or(Shadow{ .color = _transparent }));
        list.cursor = list.cursor.value_or(Cursor::Default);

        if (style.listMaxHeight.has_value()) {
            list.maxHeight = list.maxHeight.value_or(style.listMaxHeight.value());
        }

        Popup({
            .anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left },
            .transformOrigin = PopupOrigin{ PopupVerticalOrigin::Top, PopupHorizontalOrigin::Left },
            .collision = PopupCollision::Flip,
            .offset = Vec2{ 0.0f, style.listGap.value_or(0.0f) },
            .onClose = [&]{
                state.open = false;
                state.search.clear();
                state.highlight = std::nullopt;
            },
            .header = props.header,
            .footer = props.footer,
            .contentProps = {
                .ref = &contentRef,
                .paddingLeft = listPadding.left,
                .paddingTop = listPadding.top,
                .paddingRight = listPadding.right,
                .paddingBottom = listPadding.bottom
            },
            .nodeProps = list
        }, [&]{
            auto const optionPadding = style.optionPadding.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });

            for (auto const index : state.shown) {
                auto const isSelected = (selected == index);
                auto const isHighlighted = (state.highlight == index);
                auto const background = (
                    isHighlighted ? style.optionHighlightBackground :
                    isSelected ? style.optionSelectedBackground :
                                 std::nullopt
                );
                auto const color = (
                    (isHighlighted && style.optionHighlightColor.has_value()) ? style.optionHighlightColor :
                    (isSelected && style.optionSelectedColor.has_value()) ? style.optionSelectedColor :
                                                                            style.optionColor
                );

                Node({
                    .key = std::to_string(index),
                    .direction = NodeDirection::Horizontal,
                    .alignment = NodeAlignment::Center,
                    .paddingLeft = optionPadding.left,
                    .paddingTop = optionPadding.top,
                    .paddingRight = optionPadding.right,
                    .paddingBottom = optionPadding.bottom,
                    .borderRadius = style.optionBorderRadius.value_or(0.0f),
                    .cursor = Cursor::Pointer,
                    .textColor = color,
                    .background = ColorBrush{ background.value_or(_transparent) },
                    .onEvent = [&, index](NodeEvent const& event) {
                        if (event.is<MouseMoveNodeEvent>() && state.highlight != index) {
                            state.highlight = index;
                        } else if (event.is<MouseClickNodeEvent>()) {
                            _Pick(state, document, nodeRef, index);
                            event.stopPropagation();
                        }
                    }
                }, [&]{
                    Node({ .display = NodeDisplay::Text, .content = options[index] });
                });
            }
        });
    });
}

} /* namespace Rocket */
