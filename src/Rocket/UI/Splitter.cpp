/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Color.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Splitter.hpp>

namespace Rocket {

/* Palette (fuego's splitter.scss and colors.scss). */
auto constexpr _blue400 = Vec4{ 96.0f / 255.0f, 165.0f / 255.0f, 250.0f / 255.0f, 1.0f }; /* #60a5fa */

static auto const _activeColor = _blue400;
static auto const _splitterWidth = 4.0f;

void Splitter(SplitterProps const& props) {
    PROFILE
    COMPONENT

    struct _State {
        bool isDragging;
        std::optional<float> beginValue;
        class Node* targetBox;
        SplitterDirection direction;
        SplitterTarget target;
        std::function<void(NodeEvent const&)> nodeOnEvent;
    };

    auto& state = UseState<_State>();
    auto& ref = UseRef<class Node>(props.nodeProps.ref);

    auto const isHover = (ref != nullptr ? ref->isHover() : false);

    state.target = props.target.value_or(SplitterTarget::Prev);
    state.direction = props.direction.value_or(SplitterDirection::Horizontal);
    state.nodeOnEvent = props.nodeProps.onEvent;

    auto node = props.nodeProps;
    node.ref = &ref;
    node.zIndex = node.zIndex.value_or(1);
    node.selfAlignment = node.selfAlignment.value_or(NodeAlignment::Stretch);
    node.onEvent = [&state, &ref](NodeEvent const& event) {
        if (state.nodeOnEvent) {
            state.nodeOnEvent(event);
        }

        if (event.is<MouseBeginDragNodeEvent>()) {
            if (ref != nullptr) {
                auto targetNode = (
                    state.target == SplitterTarget::Prev ? ref->getPrevSibling() :
                    state.target == SplitterTarget::Next ? ref->getNextSibling() : nullptr
                );

                state.targetBox = targetNode;

                if (state.targetBox != nullptr) {
                    state.isDragging = true;
                    state.beginValue = (
                        state.direction == SplitterDirection::Horizontal ? std::optional(state.targetBox->getComputedBorderRect().width) :
                        state.direction == SplitterDirection::Vertical ? std::optional(state.targetBox->getComputedBorderRect().height) : std::nullopt
                    );
                } else {
                    state.isDragging = false;
                    state.beginValue = std::nullopt;
                }
            } else {
                state.isDragging = false;
                state.beginValue = std::nullopt;
                state.targetBox = nullptr;
            }
        } else if (auto dragEvent = event.as<MouseDragNodeEvent>()) {
            if (state.beginValue.has_value()) {
                if (state.targetBox != nullptr) {
                    auto translate = (
                        state.target == SplitterTarget::Next
                            ? dragEvent->getTranslate() * -1.0f
                            : dragEvent->getTranslate()
                    );

                    if (state.direction == SplitterDirection::Horizontal) {
                        state.targetBox->setWidth(std::max(0.0f, state.beginValue.value() + translate.x));
                    } else if (state.direction == SplitterDirection::Vertical) {
                        state.targetBox->setHeight(std::max(0.0f, state.beginValue.value() + translate.y));
                    }
                }
            }
        } else if (event.is<MouseEndDragNodeEvent>()) {
            state.beginValue = std::nullopt;
            state.isDragging = false;
            state.targetBox = nullptr;
        }
    };

    if (state.direction == SplitterDirection::Horizontal) {
        node.cursor = node.cursor.value_or(Cursor::ColResize);
        node.marginLeft = node.marginLeft.value_or(-1.0f);
        node.marginRight = node.marginRight.value_or(-1.0f);
        node.width = node.width.value_or(_splitterWidth);
    } else if (state.direction == SplitterDirection::Vertical) {
        node.cursor = node.cursor.value_or(Cursor::RowResize);
        node.marginTop = node.marginTop.value_or(-1.0f);
        node.marginBottom = node.marginBottom.value_or(-1.0f);
        node.height = node.height.value_or(_splitterWidth);
    }

    if (state.isDragging || isHover) {
        node.background = node.background.value_or(
            ColorBrush{ props.activeColor.value_or(_activeColor) }
        );
    } else {
        if (props.color.has_value()) {
            node.background = node.background.value_or(
                ColorBrush{ *props.color }
            );
        }
    }

    Node(node);
}

} /* namespace Rocket */
