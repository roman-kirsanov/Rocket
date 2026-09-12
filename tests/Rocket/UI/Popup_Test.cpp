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

    _PopupHarness()
        : window()
        , document(window) {
        window.setSize({ 640.0f, 480.0f });

        reconciler.setUpdateFn([&] {
            Context(document, [&] {
                Node({ .ref = &anchor, .position = NodePosition::Absolute, .width = NodeValue(100.0f), .height = NodeValue(40.0f), .top = NodeValue(50.0f), .left = NodeValue(50.0f) }, [&] {
                    if (renderPopup) {
                        auto rendered = props;
                        rendered.nodeProps.ref = &popup;
                        rendered.contentProps.ref = &area;
                        rendered.onClose = [&]{ closes++; };

                        Popup(rendered, [&] {
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
    auto h = _PopupHarness();
    class Node* header = nullptr;
    class Node* footer = nullptr;
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
