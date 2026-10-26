/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Node/Document.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/Window/Window.hpp>
#include <Rocket/Window/WindowEvent.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Popup.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static auto const _gray300 = ColorFromHex("#d1d5db");

/* An anchor box at a known place with a Popup inside it, plus a second box
   elsewhere to press on, under a manually provided Document. */
struct _PopupHarness {
    Window window;
    class Document document;
    Reconciler reconciler;
    PopupProps props;
    class Node* anchor = nullptr;
    class Node* popup = nullptr;
    class Node* content = nullptr;
    class Node* area = nullptr;
    class Node* elsewhere = nullptr;
    int closes = 0;
    bool renderPopup = true;
    float anchorTop = 50.0f;
    float anchorLeft = 50.0f;
    float anchorHeight = 40.0f;
    PopupFlip flip;
    PopupFlip flipSeenByChildren;

    _PopupHarness()
        : window()
        , document(window) {
        window.setSize({ 640.0f, 480.0f });

        reconciler.setUpdateFn([&] {
            Context(document, [&] {
                Node({ .ref = &anchor, .position = NodePosition::Absolute, .width = NodeValue(100.0f), .height = NodeValue(anchorHeight), .top = NodeValue(anchorTop), .left = NodeValue(anchorLeft) }, [&] {
                    if (renderPopup) {
                        auto rendered = props;
                        rendered.nodeProps.ref = &popup;
                        rendered.contentProps.ref = &area;
                        rendered.onClose = [&]{ closes++; };
                        rendered.onFlip = [&](PopupFlip const& flipped) { flip = flipped; };

                        Popup(rendered, [&] {
                            flipSeenByChildren = flip;
                            Node({ .ref = &content, .width = NodeValue(80.0f), .height = NodeValue(30.0f) });
                        });
                    }
                });

                Node({ .ref = &elsewhere, .position = NodePosition::Absolute, .width = NodeValue(60.0f), .height = NodeValue(60.0f), .top = NodeValue(300.0f), .left = NodeValue(300.0f) });
            });
        });
    }

    void render() {
        reconciler.update();
        document.update();
    }

    static Vec2 centerOf(class Node* node) {
        auto const& rect = node->getComputedBorderRect();
        return { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f };
    }

    void pressAt(Vec2 const& position) {
        window.onEvent.publish(MouseMoveWindowEvent(window, position, KeyModifiers{}));
        window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, position, KeyModifiers{}));
        window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, position, KeyModifiers{}));
        render();
    }
};

TEST(Popup, DefaultsAndLook) {
    auto h = _PopupHarness();
    h.render();

    ASSERT_TRUE(h.popup != nullptr);
    ASSERT_TRUE(h.popup->getParent() == h.anchor);
    ASSERT_TRUE(h.popup->getPosition() == NodePosition::Fixed);
    ASSERT_TRUE(h.popup->getDirection() == NodeDirection::Vertical);
    ASSERT_TRUE(h.popup->getOverflowY() == NodeOverflow::Hidden);

    /* Children live in a scrolling content node inside the popup. */
    ASSERT_TRUE(h.area != nullptr);
    ASSERT_TRUE(h.area->getParent() == h.popup);
    ASSERT_TRUE(h.area->getOverflowY() == NodeOverflow::Scroll);
    ASSERT_TRUE(h.area->getFlex() == true);
    ASSERT_TRUE(h.content->getParent() == h.area);
    ASSERT_TRUE(h.popup->getZIndex() == POPUP_ZINDEX);
    ASSERT_TRUE(h.popup->getBorderWidth() == 1.0f);
    ASSERT_TRUE(h.popup->getBorderRadius() == 5.0f);
    ASSERT_TRUE(h.popup->getBackground()->as<ColorBrush>()->color == COLOR_WHITE);
    ASSERT_TRUE(h.popup->getBorder()->as<ColorBrush>()->color == _gray300);
    ASSERT_TRUE(h.popup->getShadow().has_value());
    ASSERT_TRUE(h.popup->getShadow()->blur == 6.0f);
    ASSERT_TRUE(h.popup->getShadow()->offset == Vec2(0.0f, 4.0f));
    ASSERT_TRUE(h.popup->getCursor() == Cursor::Default);

    /* Default origins: the popup's top-left sits on the anchor's top-left. */
    ASSERT_TRUE(h.popup->getTop() == NodeValue(PercentValue{ 0.0f }));
    ASSERT_TRUE(h.popup->getLeft() == NodeValue(PercentValue{ 0.0f }));
    ASSERT_TRUE(h.popup->getTransform()->translateX == NodeValue(PercentValue{ 0.0f }));
    ASSERT_TRUE(h.popup->getTransform()->translateY == NodeValue(PercentValue{ 0.0f }));
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 50.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 50.0f);
}

TEST(Popup, Origins) {
    auto h = _PopupHarness();

    /* Anchored to the anchor's bottom-left, like a dropdown. */
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left };
    h.render();
    ASSERT_TRUE(h.popup->getTop() == NodeValue(PercentValue{ 100.0f }));
    ASSERT_TRUE(h.popup->getLeft() == NodeValue(PercentValue{ 0.0f }));
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 90.0f);

    /* Anchored to the top-right with the popup's top-left there, like a flyout. */
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Top, PopupHorizontalOrigin::Right };
    h.render();
    ASSERT_TRUE(h.popup->getLeft() == NodeValue(PercentValue{ 100.0f }));
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 150.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 50.0f);

    /* Centre on centre: the popup's own size shifts it back by half. */
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Center, PopupHorizontalOrigin::Center };
    h.props.transformOrigin = PopupOrigin{ PopupVerticalOrigin::Center, PopupHorizontalOrigin::Center };
    h.render();
    ASSERT_TRUE(h.popup->getTop() == NodeValue(PercentValue{ 50.0f }));
    ASSERT_TRUE(h.popup->getTransform()->translateX == NodeValue(PercentValue{ -50.0f }));
    ASSERT_TRUE(h.popup->getTransform()->translateY == NodeValue(PercentValue{ -50.0f }));

    /* Bottom-right transform origin: the popup hangs up and to the left. */
    h.props.transformOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Right };
    h.render();
    ASSERT_TRUE(h.popup->getTransform()->translateX == NodeValue(PercentValue{ -100.0f }));
    ASSERT_TRUE(h.popup->getTransform()->translateY == NodeValue(PercentValue{ -100.0f }));
}

TEST(Popup, ClosesOnOutsidePressOnly) {
    auto h = _PopupHarness();
    h.render();

    /* Presses on the anchor or inside the popup do not close it. */
    h.pressAt(_PopupHarness::centerOf(h.anchor));
    ASSERT_TRUE(h.closes == 0);
    h.pressAt(_PopupHarness::centerOf(h.content));
    ASSERT_TRUE(h.closes == 0);

    /* A press anywhere else does, once per press. */
    h.pressAt(_PopupHarness::centerOf(h.elsewhere));
    ASSERT_TRUE(h.closes == 1);
    h.pressAt({ 600.0f, 450.0f });
    ASSERT_TRUE(h.closes == 2);

    /* After unmounting, the subscription is gone. */
    h.renderPopup = false;
    h.render();
    ASSERT_TRUE(h.popup == nullptr);
    h.pressAt(_PopupHarness::centerOf(h.elsewhere));
    ASSERT_TRUE(h.closes == 2);
}

TEST(Popup, NodePropsOverride) {
    auto h = _PopupHarness();
    h.props.nodeProps.top = NodeValue(7.0f);
    h.props.nodeProps.zIndex = 99;
    h.props.nodeProps.background = ColorBrush{ COLOR_BLACK };
    h.props.nodeProps.borderRadius = 0.0f;
    h.render();

    ASSERT_TRUE(h.popup->getTop() == NodeValue(7.0f));
    ASSERT_TRUE(h.popup->getZIndex() == 99);
    ASSERT_TRUE(h.popup->getBackground()->as<ColorBrush>()->color == COLOR_BLACK);
    ASSERT_TRUE(h.popup->getBorderRadius() == 0.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 57.0f);
}

TEST(Popup, HeaderAndFooter) {
    class Node* header = nullptr; /* declared before the harness: its nodes clear these refs on teardown */
    class Node* footer = nullptr;
    auto h = _PopupHarness();
    h.props.header = [&]{ Node({ .ref = &header, .height = NodeValue(20.0f) }); };
    h.props.footer = [&]{ Node({ .ref = &footer, .height = NodeValue(20.0f) }); };
    h.render();

    ASSERT_TRUE(header != nullptr);
    ASSERT_TRUE(footer != nullptr);
    ASSERT_TRUE(h.popup->getFirstChild() == header);
    ASSERT_TRUE(header->getNextSibling() == h.area);
    ASSERT_TRUE(h.area->getNextSibling() == footer);
    ASSERT_TRUE(h.popup->getLastChild() == footer);

    /* The content is capped by the popup's height and scrolls; header and
       footer keep their full height. */
    h.props.nodeProps.height = NodeValue(60.0f);
    h.render();
    ASSERT_FLOAT_EQ(header->getComputedBorderRect().height, 20.0f);
    ASSERT_FLOAT_EQ(footer->getComputedBorderRect().height, 20.0f);
    ASSERT_FLOAT_EQ(h.area->getComputedBorderRect().height, 60.0f - 2.0f - 40.0f);
}

TEST(Popup, CollisionOffLeavesThePopupWhereItIs) {
    auto h = _PopupHarness();
    h.props.collision = PopupCollision::None;
    h.props.nodeProps.left = NodeValue(600.0f);
    h.render();
    h.render();

    /* Past the right edge and left there: nothing is measured or hidden. */
    ASSERT_TRUE(h.popup->getVisible().value_or(true) == true);
    ASSERT_TRUE(h.popup->getTransform()->translateX == NodeValue(PercentValue{ 0.0f }));
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 650.0f);
}

TEST(Popup, ShiftKeepsThePopupInsideTheDocument) {
    auto h = _PopupHarness();
    auto const size = h.document.getSize();

    /* A popup that fits is hidden for the render before it is measured,
       then shown where it would have been anyway. */
    h.render();
    ASSERT_TRUE(h.popup->getVisible() == false);
    h.render();
    ASSERT_TRUE(h.popup->getVisible().value_or(true) == true);
    ASSERT_TRUE(h.popup->getTransform()->translateX == NodeValue(PercentValue{ 0.0f }));
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 50.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 50.0f);

    /* Past the right and bottom edges: shifted back flush with them, and it
       stays there on later renders. */
    h.props.nodeProps.left = NodeValue(600.0f);
    h.props.nodeProps.top = NodeValue(440.0f);
    h.render();
    h.render();
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxX(), size.width);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), size.height);
    h.render();
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxX(), size.width);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), size.height);

    /* Past the left and top edges. */
    h.props.nodeProps.left = NodeValue(-200.0f);
    h.props.nodeProps.top = NodeValue(-200.0f);
    h.render();
    h.render();
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 0.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 0.0f);

    /* Back in range, the shift goes and the origin is a percentage again. */
    h.props.nodeProps.left = std::nullopt;
    h.props.nodeProps.top = std::nullopt;
    h.render();
    h.render();
    ASSERT_TRUE(h.popup->getTransform()->translateX == NodeValue(PercentValue{ 0.0f }));
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 50.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 50.0f);
}

TEST(Popup, ShiftKeepsTheTransformOrigin) {
    auto h = _PopupHarness();
    auto const size = h.document.getSize();

    /* Hanging up and to the left of a point near the top-left corner: the
       origin's percentage and the shift are applied together. */
    h.props.transformOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Right };
    h.props.nodeProps.left = NodeValue(-40.0f);
    h.props.nodeProps.top = NodeValue(-40.0f);
    h.render();
    h.render();
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 0.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 0.0f);

    /* Same origin, but in range: placed by the percentage alone. */
    h.props.nodeProps.left = NodeValue(300.0f);
    h.props.nodeProps.top = NodeValue(300.0f);
    h.render();
    h.render();
    ASSERT_TRUE(h.popup->getTransform()->translateX == NodeValue(PercentValue{ -100.0f }));
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxX(), 350.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), 350.0f);
    ASSERT_TRUE(size.width > 350.0f);
}

TEST(Popup, FlipsAboveItsAnchorWhenItRunsPastTheBottom) {
    auto h = _PopupHarness();
    h.props.collision = PopupCollision::Flip;
    h.anchorTop = 440.0f;
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left };

    h.render();
    h.render();

    /* Below the anchor it would end at 512; above it, it ends on the anchor's top. */
    ASSERT_TRUE(h.flip.vertical);
    ASSERT_FALSE(h.flip.horizontal);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), 440.0f);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().x, 50.0f);

    /* It stays flipped on later renders. */
    h.render();
    ASSERT_TRUE(h.flip.vertical);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), 440.0f);
}

TEST(Popup, StaysOnItsSideWhenItFits) {
    auto h = _PopupHarness();
    h.props.collision = PopupCollision::Flip;
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left };

    h.render();
    h.render();

    ASSERT_FALSE(h.flip.vertical);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 90.0f);
}

TEST(Popup, MirrorsItsOffsetWhenItFlips) {
    auto h = _PopupHarness();
    h.props.collision = PopupCollision::Flip;
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left };
    h.props.offset = Vec2{ 0.0f, 6.0f };

    /* Below an anchor that fits, the gap is under the anchor. */
    h.render();
    h.render();
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 96.0f);

    /* Flipped above an anchor at the bottom, the gap is over it. */
    h.anchorTop = 440.0f;
    h.render();
    h.render();
    ASSERT_TRUE(h.flip.vertical);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), 434.0f);
}

TEST(Popup, FlipsAroundThePointItIsPlacedAt) {
    auto h = _PopupHarness();
    h.props.collision = PopupCollision::Flip;
    auto const size = h.document.getSize();
    h.props.nodeProps.top = NodeValue(size.height - 70.0f);

    h.render();
    h.render();

    /* Placed with its top at 20 above the bottom, it hangs up from that point instead. */
    ASSERT_TRUE(h.flip.vertical);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), size.height - 20.0f);
}

TEST(Popup, NeverFlipsAnAxisWhoseBothEdgesAreSet) {
    auto h = _PopupHarness();
    h.props.collision = PopupCollision::Flip;
    auto const size = h.document.getSize();
    h.props.nodeProps.top = NodeValue(size.height - 70.0f);
    h.props.nodeProps.bottom = NodeValue(-100.0f);

    h.render();
    h.render();

    ASSERT_FALSE(h.flip.vertical);
}

TEST(Popup, ShiftsWithoutFlippingByDefault) {
    auto h = _PopupHarness();
    auto const size = h.document.getSize();
    h.anchorTop = 440.0f;
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left };

    h.render();
    h.render();

    /* Not flipped, only shifted up to the bottom edge. */
    ASSERT_FALSE(h.flip.vertical);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxY(), size.height);
}

TEST(Popup, FitCapsItsMaximumSizeToTheDocument) {
    auto h = _PopupHarness();
    auto const size = h.document.getSize();
    h.props.collision = PopupCollision::Fit;

    h.render();

    ASSERT_TRUE(h.popup->getMaxWidth() == NodeValue(size.width));
    ASSERT_TRUE(h.popup->getMaxHeight() == NodeValue(size.height));
}

TEST(Popup, FitKeepsASmallerMaximumItIsGiven) {
    auto h = _PopupHarness();
    h.props.collision = PopupCollision::Fit;
    h.props.nodeProps.maxHeight = NodeValue(100.0f);

    h.render();

    ASSERT_TRUE(h.popup->getMaxHeight() == NodeValue(100.0f));
}

TEST(Popup, FlipDoesNotShiftWhatStillRunsPastAnEdge) {
    auto h = _PopupHarness();
    auto const size = h.document.getSize();
    h.props.collision = PopupCollision::Flip;
    h.props.nodeProps.top = NodeValue(-200.0f);
    h.props.nodeProps.bottom = NodeValue(size.height);

    h.render();
    h.render();

    /* Both edges set, so it cannot flip, and Flip never slides it back. */
    ASSERT_FALSE(h.flip.vertical);
    ASSERT_TRUE(h.popup->getComputedBorderRect().y < 0.0f);
}

TEST(Popup, FitShiftsThePopupInsideTheDocument) {
    auto h = _PopupHarness();
    auto const size = h.document.getSize();
    h.props.collision = PopupCollision::Fit;
    h.props.nodeProps.left = NodeValue(600.0f);

    h.render();
    h.render();

    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxX(), size.width);
}

TEST(Popup, FlipsToTheLeftOfItsAnchorWhenItRunsPastTheRight) {
    auto h = _PopupHarness();
    h.anchorLeft = 530.0f;
    h.props.collision = PopupCollision::Flip;
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Top, PopupHorizontalOrigin::Right };

    h.render();
    h.render();

    /* Right of the anchor it would end at 712; left of it, it ends on the anchor's left edge. */
    ASSERT_TRUE(h.flip.horizontal);
    ASSERT_FALSE(h.flip.vertical);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().getMaxX(), 530.0f);
}

TEST(Popup, KeepsTheSideThatRunsPastLessWhenNeitherFits) {
    auto h = _PopupHarness();
    h.anchorTop = 10.0f;
    h.anchorHeight = 440.0f;
    h.props.collision = PopupCollision::Flip;
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left };

    h.render();
    h.render();

    /* Below, it runs 2 past the bottom; above, 22 past the top. */
    ASSERT_FALSE(h.flip.vertical);
    ASSERT_FLOAT_EQ(h.popup->getComputedBorderRect().y, 450.0f);
}

TEST(Popup, ReportsItsFlipBeforeItRendersItsChildren) {
    auto h = _PopupHarness();
    h.anchorTop = 440.0f;
    h.props.collision = PopupCollision::Flip;
    h.props.anchorOrigin = PopupOrigin{ PopupVerticalOrigin::Bottom, PopupHorizontalOrigin::Left };

    h.render();
    ASSERT_FALSE(h.flipSeenByChildren.vertical);

    /* The render that decides to flip shows its children the flip already. */
    h.render();
    ASSERT_TRUE(h.flipSeenByChildren.vertical);
}
