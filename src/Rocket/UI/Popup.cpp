/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Popup.hpp>

namespace Rocket {

/* Per-instance component state. */
struct _PopupState {
    std::function<void()> onClose;
    /* Correction, in points, that keeps an adjusted popup inside the document. */
    Vec2 shift;
    /* False until an adjusted popup has a layout to measure; it stays hidden until then. */
    bool measured = false;
};

/* Palette (fuego's popup.scss and colors.scss). */
auto constexpr _gray300 = Vec4{ 209.0f / 255.0f, 213.0f / 255.0f, 219.0f / 255.0f, 1.0f }; /* #d1d5db */
auto constexpr _white = COLOR_WHITE;
auto constexpr _shadowColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.1f };

static auto const _borderWidth = 1.0f;
static auto const _borderRadius = 5.0f;
static auto const _shadowOffset = Vec2{ 0.0f, 4.0f };
static auto const _shadowBlur = 6.0f;
/* Gap kept between an adjusted popup and the document's edge. */
static auto const _edgeGap = 4.0f;

/* Returns how far to move a span of `size` starting at `start` so that it
   fits in [0, limit]; the start edge wins when the span is longer than that. */
static float _GetShift(float start, float size, float limit) {
    auto shift = 0.0f;

    if (start + size > limit - _edgeGap) {
        shift = (limit - _edgeGap) - (start + size);
    }

    if (start + shift < _edgeGap) {
        shift = _edgeGap - start;
    }

    return shift;
}

void Popup(PopupProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    auto& state = UseState<_PopupState>();
    auto& document = UseContext<class Document>();
    auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);

    state.onClose = props.onClose;

    /* A press anywhere outside the popup's parent (the anchor) closes it.
       The handler is fixed on mount, so it reads the callback from state. */
    UseSubscription(document.onCaptureEvent, [&state, &nodeRef](NodeEvent const& event) {
        if (event.is<MouseDownNodeEvent>()) {
            auto parentBox = (
                (nodeRef != nullptr)
                    ? nodeRef->getParent()
                    : nullptr
            );

            auto const inside = (parentBox != nullptr && parentBox->containsNode(event.getNode()));

            if (inside == false) {
                if (state.onClose) {
                    state.onClose();
                }
            }
        }
    });

    auto const adjust = props.adjust.value_or(false);

    /* The rect comes from the last update and includes the shift applied
       then, so the unshifted position is the rect minus that shift. */
    if (adjust == true && nodeRef != nullptr && nodeRef->getComputedBorderRect().width > 0.0f) {
        auto const& rect = nodeRef->getComputedBorderRect();
        auto const size = document.getSize();

        state.shift = Vec2{
            _GetShift(rect.x - state.shift.x, rect.width, size.width),
            _GetShift(rect.y - state.shift.y, rect.height, size.height)
        };
        state.measured = true;
    } else if (adjust == false) {
        state.shift = Vec2{ 0.0f, 0.0f };
        state.measured = false;
    }

    auto const shifted = (state.shift.x != 0.0f || state.shift.y != 0.0f);

    auto const anchorOrigin = props.anchorOrigin.value_or(PopupOrigin{ PopupVerticalOrigin::Top, PopupHorizontalOrigin::Left });
    auto const transformOrigin = props.transformOrigin.value_or(PopupOrigin{ PopupVerticalOrigin::Top, PopupHorizontalOrigin::Left });

    auto const top = (
        anchorOrigin.vertical == PopupVerticalOrigin::Top ? 0.0f :
        anchorOrigin.vertical == PopupVerticalOrigin::Center ? 50.0f :
        anchorOrigin.vertical == PopupVerticalOrigin::Bottom ? 100.0f : 0.0f
    );

    auto const left = (
        anchorOrigin.horizontal == PopupHorizontalOrigin::Left ? 0.0f :
        anchorOrigin.horizontal == PopupHorizontalOrigin::Center ? 50.0f :
        anchorOrigin.horizontal == PopupHorizontalOrigin::Right ? 100.0f : 0.0f
    );

    auto const translateY = (
        transformOrigin.vertical == PopupVerticalOrigin::Top ? 0.0f :
        transformOrigin.vertical == PopupVerticalOrigin::Center ? -50.0f :
        transformOrigin.vertical == PopupVerticalOrigin::Bottom ? -100.0f : 0.0f
    );

    auto const translateX = (
        transformOrigin.horizontal == PopupHorizontalOrigin::Left ? 0.0f :
        transformOrigin.horizontal == PopupHorizontalOrigin::Center ? -50.0f :
        transformOrigin.horizontal == PopupHorizontalOrigin::Right ? -100.0f : 0.0f
    );

    auto node = props.nodeProps;
    node.ref = &nodeRef;
    node.overflowX = node.overflowX.value_or(NodeOverflow::Hidden);
    node.overflowY = node.overflowY.value_or(NodeOverflow::Hidden);
    node.position = node.position.value_or(NodePosition::Fixed);
    node.direction = node.direction.value_or(NodeDirection::Vertical);
    /* A popup escapes its ancestors' clipping (a scrolling parent, or the
       dropdown a flyout lives in), otherwise it could neither be seen nor
       hit outside them. */
    node.clipped = node.clipped.value_or(false);
    node.top = node.top.value_or(NodeValue{ PercentValue{ top } });
    node.left = node.left.value_or(NodeValue{ PercentValue{ left } });
    node.borderWidth = node.borderWidth.value_or(_borderWidth);
    node.borderRadius = node.borderRadius.value_or(_borderRadius);
    node.zIndex = node.zIndex.value_or(POPUP_ZINDEX);

    if (shifted == true) {
        /* A translation is either a percentage or points, so once there is
           a shift to add, the origin's percentage is resolved against the
           measured size. */
        auto const& rect = nodeRef->getComputedBorderRect();

        node.transform = node.transform.value_or(NodeTransform{
            .translateX = (translateX / 100.0f) * rect.width + state.shift.x,
            .translateY = (translateY / 100.0f) * rect.height + state.shift.y
        });
    } else {
        node.transform = node.transform.value_or(NodeTransform{
            .translateX = PercentValue{ translateX },
            .translateY = PercentValue{ translateY }
        });
    }

    if (adjust == true && state.measured == false) {
        node.visible = false;
    }

    node.cursor = node.cursor.value_or(Cursor::Default);
    node.background = node.background.value_or(ColorBrush{ _white });
    node.border = node.border.value_or(ColorBrush{ _gray300 });
    node.shadow = node.shadow.value_or(Shadow{
        .color = _shadowColor,
        .offset = _shadowOffset,
        .blur = _shadowBlur
    });

    /* The popup clips; the content inside it scrolls, so a header and
       footer stay put while `children` scroll between them. */
    auto content = props.contentProps;
    content.overflowX = content.overflowX.value_or(NodeOverflow::Scroll);
    content.overflowY = content.overflowY.value_or(NodeOverflow::Scroll);
    content.direction = content.direction.value_or(NodeDirection::Vertical);
    content.alignment = content.alignment.value_or(NodeAlignment::Stretch);
    content.flex = content.flex.value_or(true);

    Node(node, [&]{
        if (props.header) {
            props.header();
        }

        Node(content, children);

        if (props.footer) {
            props.footer();
        }
    });
}

} /* namespace Rocket */
