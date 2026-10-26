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
    /* Correction, in points, that keeps the popup inside the document. */
    Vec2 shift;
    /* The axes the popup was mirrored on when it was last rendered. */
    PopupFlip flip;
    /* False until a popup that flips or shifts has a layout to measure; it stays hidden until then. */
    bool measured = false;
};

/* One axis of a popup as measured: its span, the document's length on that
   axis, the anchor's span when the axis is placed by the origins (none when
   `nodeProps` places it at a point), and the origins' fractions and the
   offset as the props give them, before any flip. */
struct _PopupAxis {
    float start;
    float size;
    float limit;
    std::optional<Vec2> anchor;
    float anchorFraction;
    float fraction;
    float offset;
    bool flipped;
    bool flippable;
};

/* Where one axis of a popup goes: whether it is mirrored, and the shift. */
struct _PopupPlacement {
    bool flip;
    float shift;
};

/* Palette (fuego's popup.scss and colors.scss). */
auto constexpr _gray300 = Vec4{ 209.0f / 255.0f, 213.0f / 255.0f, 219.0f / 255.0f, 1.0f }; /* #d1d5db */
auto constexpr _white = COLOR_WHITE;
auto constexpr _shadowColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.1f };

static auto const _borderWidth = 1.0f;
static auto const _borderRadius = 5.0f;
static auto const _shadowOffset = Vec2{ 0.0f, 4.0f };
static auto const _shadowBlur = 6.0f;

static float _GetFraction(PopupVerticalOrigin origin) {
    return (
        origin == PopupVerticalOrigin::Center ? 0.5f :
        origin == PopupVerticalOrigin::Bottom ? 1.0f : 0.0f
    );
}

static float _GetFraction(PopupHorizontalOrigin origin) {
    return (
        origin == PopupHorizontalOrigin::Center ? 0.5f :
        origin == PopupHorizontalOrigin::Right ? 1.0f : 0.0f
    );
}

static PopupVerticalOrigin _Mirror(PopupVerticalOrigin origin) {
    return (
        origin == PopupVerticalOrigin::Top ? PopupVerticalOrigin::Bottom :
        origin == PopupVerticalOrigin::Bottom ? PopupVerticalOrigin::Top : origin
    );
}

static PopupHorizontalOrigin _Mirror(PopupHorizontalOrigin origin) {
    return (
        origin == PopupHorizontalOrigin::Left ? PopupHorizontalOrigin::Right :
        origin == PopupHorizontalOrigin::Right ? PopupHorizontalOrigin::Left : origin
    );
}

/* Returns how far to move a span of `size` starting at `start` so that it
   fits in [0, limit]; the start edge wins when the span is longer than that. */
static float _GetShift(float start, float size, float limit) {
    auto shift = 0.0f;

    if (start + size > limit) {
        shift = limit - (start + size);
    }

    if (start + shift < 0.0f) {
        shift = -start;
    }

    return shift;
}

/* Returns how far a span runs past [0, limit], over both edges. */
static float _GetOverflow(float start, float size, float limit) {
    return std::max(0.0f, -start) + std::max(0.0f, (start + size) - limit);
}

/* Returns the start of a span placed with the given fractions and offset,
   mirrored to the other side of the anchor's point: the anchor's other edge
   when the axis is placed by the origins, or the same point otherwise. */
static float _GetMirroredStart(float start, float size, std::optional<Vec2> const& anchor, float anchorFraction, float fraction, float offset) {
    auto const point = (
        anchor.has_value()
            ? anchor->x + (1.0f - anchorFraction) * anchor->y
            : start + fraction * size - offset
    );

    return point - offset - (1.0f - fraction) * size;
}

/* The preferred placement is worked out from the measured one, so the
   choice to flip never depends on the flip of the last render. */
static _PopupPlacement _PlacePopupAxis(_PopupAxis const& axis, PopupCollision collision) {
    auto const preferred = (
        axis.flipped
            ? _GetMirroredStart(axis.start, axis.size, axis.anchor, 1.0f - axis.anchorFraction, 1.0f - axis.fraction, -axis.offset)
            : axis.start
    );
    auto const mirrored = _GetMirroredStart(preferred, axis.size, axis.anchor, axis.anchorFraction, axis.fraction, axis.offset);
    auto const overflow = _GetOverflow(preferred, axis.size, axis.limit);
    auto const flip = (
        collision == PopupCollision::Flip &&
        axis.flippable &&
        overflow > 0.0f &&
        _GetOverflow(mirrored, axis.size, axis.limit) < overflow
    );

    return _PopupPlacement{
        .flip = flip,
        .shift = (collision == PopupCollision::Flip ? 0.0f : _GetShift(preferred, axis.size, axis.limit))
    };
}

/* Returns `limit` when the caller set no maximum in points, or the smaller of the two. */
static NodeValue _GetFittedMax(std::optional<NodeValue> const& max, float limit) {
    if (max.has_value() == false) {
        return NodeValue(limit);
    }

    if (auto const pixels = max->as<PixelValue>()) {
        return NodeValue(std::min(pixels->value, limit));
    }

    return max.value();
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
    UseSubscription(document.onCaptureEvent, [&](NodeEvent const& event) {
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

    auto const collision = props.collision.value_or(PopupCollision::Shift);
    auto const measures = (collision != PopupCollision::None);
    auto const preferredAnchor = props.anchorOrigin.value_or(PopupOrigin{ PopupVerticalOrigin::Top, PopupHorizontalOrigin::Left });
    auto const preferredTransform = props.transformOrigin.value_or(PopupOrigin{ PopupVerticalOrigin::Top, PopupHorizontalOrigin::Left });
    auto const preferredOffset = props.offset.value_or(Vec2{ 0.0f, 0.0f });
    auto const placedX = (props.nodeProps.left.has_value() || props.nodeProps.right.has_value());
    auto const placedY = (props.nodeProps.top.has_value() || props.nodeProps.bottom.has_value());

    /* The rect comes from the last update and includes the shift applied
       then, so the unshifted position is the rect minus that shift. */
    if (measures && nodeRef != nullptr && nodeRef->getComputedBorderRect().width > 0.0f) {
        auto const& rect = nodeRef->getComputedBorderRect();
        auto const size = document.getSize();
        auto const parent = nodeRef->getParent();
        auto const anchor = (parent != nullptr ? parent->getComputedBorderRect() : Vec4{});
        auto const x = _PlacePopupAxis({
            .start = rect.x - state.shift.x,
            .size = rect.width,
            .limit = size.width,
            .anchor = ((parent != nullptr && placedX == false) ? std::optional<Vec2>(Vec2{ anchor.x, anchor.width }) : std::nullopt),
            .anchorFraction = _GetFraction(preferredAnchor.horizontal),
            .fraction = _GetFraction(preferredTransform.horizontal),
            .offset = preferredOffset.x,
            .flipped = state.flip.horizontal,
            .flippable = ((props.nodeProps.left.has_value() && props.nodeProps.right.has_value()) == false)
        }, collision);
        auto const y = _PlacePopupAxis({
            .start = rect.y - state.shift.y,
            .size = rect.height,
            .limit = size.height,
            .anchor = ((parent != nullptr && placedY == false) ? std::optional<Vec2>(Vec2{ anchor.y, anchor.height }) : std::nullopt),
            .anchorFraction = _GetFraction(preferredAnchor.vertical),
            .fraction = _GetFraction(preferredTransform.vertical),
            .offset = preferredOffset.y,
            .flipped = state.flip.vertical,
            .flippable = ((props.nodeProps.top.has_value() && props.nodeProps.bottom.has_value()) == false)
        }, collision);

        state.measured = true;
        state.flip = PopupFlip{ .vertical = y.flip, .horizontal = x.flip };
        state.shift = Vec2{ x.shift, y.shift };
    } else if (measures == false) {
        state.measured = false;
        state.flip = PopupFlip{};
        state.shift = Vec2{ 0.0f, 0.0f };
    }

    auto const shifted = (state.shift.x != 0.0f || state.shift.y != 0.0f);

    auto const anchorOrigin = PopupOrigin{
        .vertical = (state.flip.vertical ? _Mirror(preferredAnchor.vertical) : preferredAnchor.vertical),
        .horizontal = (state.flip.horizontal ? _Mirror(preferredAnchor.horizontal) : preferredAnchor.horizontal)
    };
    auto const transformOrigin = PopupOrigin{
        .vertical = (state.flip.vertical ? _Mirror(preferredTransform.vertical) : preferredTransform.vertical),
        .horizontal = (state.flip.horizontal ? _Mirror(preferredTransform.horizontal) : preferredTransform.horizontal)
    };

    auto const top = _GetFraction(anchorOrigin.vertical) * 100.0f;
    auto const left = _GetFraction(anchorOrigin.horizontal) * 100.0f;
    auto const translateY = _GetFraction(transformOrigin.vertical) * -100.0f;
    auto const translateX = _GetFraction(transformOrigin.horizontal) * -100.0f;

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

    if (props.offset.has_value()) {
        node.offset = node.offset.value_or(Vec2{
            (state.flip.horizontal ? -preferredOffset.x : preferredOffset.x),
            (state.flip.vertical ? -preferredOffset.y : preferredOffset.y)
        });
    }

    if (collision == PopupCollision::Fit) {
        node.maxWidth = _GetFittedMax(node.maxWidth, document.getSize().width);
        node.maxHeight = _GetFittedMax(node.maxHeight, document.getSize().height);
    }

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

    if (measures && state.measured == false) {
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

    if (props.onFlip) {
        props.onFlip(state.flip);
    }

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
