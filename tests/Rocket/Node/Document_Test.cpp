/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <string>
#include <vector>
#include <stdexcept>
#include <gmock/gmock.h>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/Window/Clipboard.hpp>
#include <gtest/gtest.h>

using namespace Rocket;
using namespace testing;

/* Note: windows are never made visible in this test, so no window flashes on
   screen while the suite runs. Input is synthesized by publishing WindowEvents
   directly through the public window.onEvent source, which the Document's
   constructor subscribes to. Hit testing uses computed rects, so update() must
   run before any synthetic mouse event can find a node. */

/* -------------------------------------------------------------------------
 * Link-seam Painter mock.
 *
 * This TU defines all of Painter's public symbols itself, so the linker
 * never pulls the real Painter.cpp.o out of libRocket.a (archive members
 * are only pulled to resolve undefined symbols). Painter_MacOS.mm.o is
 * still pulled — Image needs its GPU device functions — but its Painter
 * internals (__init/__paint/...) are referenced by nothing in this binary
 * and collide with nothing. Every Painter in this binary — the Document's
 * and Text's — forwards to the active sink; with no sink bound, painting
 * is a silent no-op (which is what the non-render tests here want).
 *
 * Caveat: if Painter.cpp ever gains a symbol that other production code
 * references, that member would be pulled and collide with these
 * definitions; the link error will point straight here.
 * ------------------------------------------------------------------------- */

class PainterSink {
public:
    MOCK_METHOD(void, beginPaint, (PaintTarget const&));
    MOCK_METHOD(void, endPaint, ());
    MOCK_METHOD(void, paint, (Shape const&, Brush const&, PaintOptions const&));
};

static PainterSink* _sink = nullptr;

/** Binds a sink for the duration of a test. */
struct SinkScope {
    SinkScope(PainterSink& sink) { _sink = &sink; }
    ~SinkScope() { _sink = nullptr; }
};

Painter::Painter() : _impl(nullptr) {}
Painter::~Painter() {}

void Painter::beginPaint(PaintTarget const& target) {
    if (_sink != nullptr) _sink->beginPaint(target);
}

void Painter::endPaint() {
    if (_sink != nullptr) _sink->endPaint();
}

void Painter::paint(Shape const& shape, Brush const& brush, PaintOptions const& options) const {
    if (_sink != nullptr) _sink->paint(shape, brush, options);
}

/* ------------------------------- matchers -------------------------------- */

MATCHER(TargetsWindow, "targets the window") {
    return (arg.template as<WindowPaintTarget>() != nullptr);
}

MATCHER(TargetsAnImage, "targets an offscreen image") {
    return (arg.template as<ImagePaintTarget>() != nullptr);
}

MATCHER_P(PaintsRect, rect, "paints a quad at the given rect") {
    auto const quad = arg.template as<QuadShape>();
    return (quad != nullptr) && (quad->rect == rect);
}

MATCHER_P(WithScissor, rect, "has the given scissor rect") {
    return arg.scissor.has_value() && (arg.scissor.value() == rect);
}

MATCHER_P(WithOpacity, opacity, "has the given opacity") {
    return arg.opacity.has_value() && (arg.opacity.value() == opacity);
}

MATCHER(IsImageBrush, "is an image brush") {
    return (arg.template as<ImageBrush>() != nullptr);
}

namespace {

std::string eventName(NodeEvent const& event) {
    if (event.is<MouseEnterNodeEvent>())     return "enter";
    if (event.is<MouseExitNodeEvent>())      return "exit";
    if (event.is<MouseMoveNodeEvent>())      return "move";
    if (event.is<MouseDownNodeEvent>())      return "down";
    if (event.is<MouseUpNodeEvent>())        return "up";
    if (event.is<MouseClickNodeEvent>())     return "click";
    if (event.is<MouseBeginDragNodeEvent>()) return "begindrag";
    if (event.is<MouseDragNodeEvent>())      return "drag";
    if (event.is<MouseEndDragNodeEvent>())   return "enddrag";
    if (event.is<MouseWheelNodeEvent>())     return "wheel";
    if (event.is<KeyDownNodeEvent>())        return "keydown";
    if (event.is<KeyUpNodeEvent>())          return "keyup";
    return "other";
}

} /* namespace */

/* Construction binds the document to the window and adopts its scale. */
TEST(Document, Construction) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    ASSERT_TRUE(&document.getWindow() == &window);
    ASSERT_TRUE(document.getSize() == Vec2(640.0f, 480.0f));
    ASSERT_TRUE(document.getScale() == window.getScale());
    ASSERT_TRUE(document.getDocument() == &document);
}

/* Layout: a fixed-size child and a flex child share a row. The computed
   border rect is (x, y, width, height) in window content coordinates. */
TEST(Document, LayoutRow) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    auto b = Node();
    b.setFlex(true);
    document.appendChild(b);

    document.update();

    ASSERT_TRUE(document.getComputedBorderRect() == Vec4(0.0f, 0.0f, 640.0f, 480.0f));
    ASSERT_TRUE(a.getComputedBorderRect() == Vec4(0.0f, 0.0f, 100.0f, 50.0f));

    /* The flex child fills the remaining main-axis space. The default
       cross-axis alignment is Start, so its auto height stays 0. */
    ASSERT_TRUE(b.getComputedBorderRect().x == 100.0f);
    ASSERT_TRUE(b.getComputedBorderRect().width == 540.0f);
}

/* Layout: sibling flex children split a definite row equally, whatever
   their content, because flex uses a zero basis. */
TEST(Document, LayoutFlexSiblingsShareEqually) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto row = Node();
    row.setWidth(400.0f);
    row.setDirection(NodeDirection::Horizontal);
    document.appendChild(row);

    auto a = Node();
    a.setFlex(true);
    row.appendChild(a);

    auto aContent = Node();
    aContent.setWidth(30.0f);
    aContent.setHeight(10.0f);
    a.appendChild(aContent);

    auto b = Node();
    b.setFlex(true);
    row.appendChild(b);

    auto bContent = Node();
    bContent.setWidth(300.0f);
    bContent.setHeight(10.0f);
    b.appendChild(bContent);

    document.update();

    ASSERT_TRUE(a.getComputedBorderRect().width == 200.0f);
    ASSERT_TRUE(b.getComputedBorderRect().x == 200.0f);
    ASSERT_TRUE(b.getComputedBorderRect().width == 200.0f);
}

/* Layout: a flex child needs a parent with a definite main size. Inside a
   content-sized column it collapses to the zero basis (React Native's
   `flex: 1` rule); once the column has a height, the child fills it. */
TEST(Document, LayoutFlexNeedsDefiniteParent) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto column = Node();
    column.setWidth(100.0f);
    column.setDirection(NodeDirection::Vertical);
    document.appendChild(column);

    auto child = Node();
    child.setFlex(true);
    column.appendChild(child);

    auto content = Node();
    content.setWidth(100.0f);
    content.setHeight(40.0f);
    child.appendChild(content);

    document.update();
    ASSERT_FLOAT_EQ(child.getComputedBorderRect().height, 0.0f);
    ASSERT_FLOAT_EQ(column.getComputedBorderRect().height, 0.0f);

    column.setHeight(120.0f);
    document.update();
    ASSERT_FLOAT_EQ(child.getComputedBorderRect().height, 120.0f);
}

/* Layout: a percent width resolves against the parent dimension. */
TEST(Document, LayoutPercentWidth) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto p = Node();
    p.setWidth(NodeValue(PercentValue{ 50.0f }));
    p.setHeight(50.0f);
    document.appendChild(p);

    document.update();
    ASSERT_TRUE(p.getComputedBorderRect() == Vec4(0.0f, 0.0f, 320.0f, 50.0f));
}

/* Hover: moving the mouse over a node marks it hovered and dispatches
   MouseEnterNodeEvent followed by MouseMoveNodeEvent on it. */
TEST(Document, Hover) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&events](NodeEvent const& event) {
        events.push_back(eventName(event));
    });

    ASSERT_TRUE(a.isHover() == false);
    window.onEvent.publish(MouseMoveWindowEvent(window, { 10.0f, 10.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isHover() == true);
    ASSERT_TRUE(document.isHover() == true);
    ASSERT_TRUE(events == std::vector<std::string>({ "enter", "move" }));

    /* Moving off the node dispatches MouseExitNodeEvent and clears hover. */
    events.clear();
    window.onEvent.publish(MouseMoveWindowEvent(window, { 300.0f, 300.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isHover() == false);
    ASSERT_TRUE(events == std::vector<std::string>({ "exit" }));
}

/* Click: press marks the hovered chain active; release dispatches
   MouseUpNodeEvent then MouseClickNodeEvent and clears the active state. */
TEST(Document, Click) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&events](NodeEvent const& event) {
        events.push_back(eventName(event));
    });

    /* Mouse down targets the current hover node, so move first. */
    window.onEvent.publish(MouseMoveWindowEvent(window, { 10.0f, 10.0f }, KeyModifiers{}));
    events.clear();

    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 10.0f, 10.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isActive() == true);
    ASSERT_TRUE(document.isActive() == true);

    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 10.0f, 10.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isActive() == false);
    ASSERT_TRUE(document.isActive() == false);
    ASSERT_TRUE(events == std::vector<std::string>({ "down", "up", "click" }));

    /* A click with tab index 0 does not focus the node. */
    ASSERT_TRUE(a.isFocused() == false);
}

/* Drag: movement beyond the 2px threshold while pressed begins a drag;
   release ends it and re-runs hover for the release position. */
TEST(Document, Drag) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto beginTranslate = Vec2();
    auto endTranslate = Vec2();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        events.push_back(eventName(event));
        if (auto e = event.as<MouseBeginDragNodeEvent>()) beginTranslate = e->getTranslate();
        if (auto e = event.as<MouseEndDragNodeEvent>()) endTranslate = e->getTranslate();
    });

    window.onEvent.publish(MouseMoveWindowEvent(window, { 10.0f, 10.0f }, KeyModifiers{}));
    events.clear();

    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 10.0f, 10.0f }, KeyModifiers{}));

    /* A 1px move stays below the drag threshold: no drag events fire. */
    window.onEvent.publish(MouseMoveWindowEvent(window, { 11.0f, 11.0f }, KeyModifiers{}));
    ASSERT_TRUE(events == std::vector<std::string>({ "down" }));

    /* A 10px move crosses the threshold: begin-drag then drag. */
    window.onEvent.publish(MouseMoveWindowEvent(window, { 20.0f, 20.0f }, KeyModifiers{}));
    ASSERT_TRUE(events == std::vector<std::string>({ "down", "begindrag", "drag" }));
    ASSERT_TRUE(beginTranslate == Vec2(10.0f, 10.0f));

    /* Release after a drag: up, end-drag, then the synthetic post-drag
       hover move at the release position. No click — click and drag are
       mutually exclusive gestures; crossing the drag threshold resolves
       the gesture as a drag. */
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 20.0f, 20.0f }, KeyModifiers{}));
    ASSERT_TRUE(events == std::vector<std::string>({
        "down", "begindrag", "drag", "up", "enddrag", "move"
    }));
    ASSERT_TRUE(endTranslate == Vec2(10.0f, 10.0f));
    ASSERT_TRUE(a.isActive() == false);
}

/* Focus: clicking a node with a positive tab index focuses it; key events
   are then delivered to the focused node. */
TEST(Document, Focus) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setTabIndex(1);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto keycode = std::string();
    auto key = Rocket::Scancode::Unknown;
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        events.push_back(eventName(event));
        if (auto e = event.as<KeyDownNodeEvent>()) {
            key = e->getScancode();
            keycode = e->getKeycode();
        }
    });

    window.onEvent.publish(MouseMoveWindowEvent(window, { 10.0f, 10.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 10.0f, 10.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 10.0f, 10.0f }, KeyModifiers{}));

    ASSERT_TRUE(a.isFocused() == true);
    ASSERT_TRUE(a.isFocusedWithin() == true);
    ASSERT_TRUE(document.isFocused() == false);
    ASSERT_TRUE(document.isFocusedWithin() == true);

    events.clear();
    window.onEvent.publish(KeyDownWindowEvent(window, Rocket::Scancode::KeyA, GetDefaultKeycode(Rocket::Scancode::KeyA), KeyModifiers{}));
    window.onEvent.publish(KeyUpWindowEvent(window, Rocket::Scancode::KeyA, GetDefaultKeycode(Rocket::Scancode::KeyA), KeyModifiers{}));
    ASSERT_TRUE(events == std::vector<std::string>({ "keydown", "keyup" }));
    ASSERT_TRUE(key == Rocket::Scancode::KeyA);
    ASSERT_TRUE(keycode == Keycode::A);
}

/* Wheel: scrolling over a scrollable node shifts its children by the
   scroll position on the next update, clamped to the scroll overflow. */
TEST(Document, Wheel) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto container = Node();
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(container);

    auto tall = Node();
    tall.setWidth(100.0f);
    tall.setHeight(300.0f);
    container.appendChild(tall);

    document.update();
    ASSERT_TRUE(tall.getComputedBorderRect() == Vec4(0.0f, 0.0f, 100.0f, 300.0f));

    /* Hover the container first: wheel events go to the hover chain.
       The wheel delta is negated, so a negative y scrolls the content up. */
    window.onEvent.publish(MouseMoveWindowEvent(window, { 10.0f, 10.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -30.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(tall.getComputedBorderRect().y == -30.0f);

    /* Scrolling far past the end clamps to the overflow (300 - 100). */
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -1000.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(tall.getComputedBorderRect().y == -200.0f);

    /* Scrolling back past the start clamps to zero. */
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, 1000.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(tall.getComputedBorderRect().y == 0.0f);
}

/* Cursor: update() resolves the cursor from the hovered node chain and
   applies it to the window; leaving the node restores Cursor::Default. */
TEST(Document, Cursor) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setCursor(Cursor::Pointer);
    document.appendChild(a);

    document.update();
    ASSERT_TRUE(a.isHover() == false);
    ASSERT_TRUE(window.getCursor() == Cursor::Default);

    window.onEvent.publish(MouseMoveWindowEvent(window, { 10.0f, 10.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(a.isHover() == true);
    ASSERT_TRUE(window.getCursor() == Cursor::Pointer);

    window.onEvent.publish(MouseMoveWindowEvent(window, { 300.0f, 300.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(a.isHover() == false);
    ASSERT_TRUE(window.getCursor() == Cursor::Default);
}

/* Scale: setScale overrides the scale adopted from the window. The content
   size is (window size × window scale) / document scale, so a bigger scale
   yields a smaller content area, layout resolves against it, and window
   positions map into content space by (window scale / document scale). */
TEST(Document, Scale) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(NodeValue(PercentValue{ 50.0f }));
    a.setHeight(50.0f);
    document.appendChild(a);

    /* With the default scale the content size equals the window size. */
    ASSERT_TRUE(document.getSize() == window.getSize());

    document.setScale(document.getScale() * 2.0f);
    ASSERT_TRUE(document.getScale() == (window.getScale() * 2.0f));

    auto const contentSize = ((window.getSize() * window.getScale()) / document.getScale());
    ASSERT_TRUE(document.getSize() == contentSize);
    ASSERT_TRUE(contentSize.width < window.getSize().width);
    ASSERT_TRUE(contentSize.height < window.getSize().height);

    document.update();
    ASSERT_TRUE(document.getComputedBorderRect() == Vec4(0.0f, 0.0f, contentSize.width, contentSize.height));

    /* The percent child resolves against the shrunken content width. */
    ASSERT_TRUE(a.getComputedBorderRect() == Vec4(0.0f, 0.0f, (contentSize.width / 2.0f), 50.0f));

    /* Mouse positions arrive in window points and map into content space
       by (window scale / document scale): a window position whose mapped
       point lies inside the node hits it. */
    auto const factor = (window.getScale() / document.getScale());

    window.onEvent.publish(MouseMoveWindowEvent(window, (Vec2(10.0f, 10.0f) / factor), KeyModifiers{}));
    ASSERT_TRUE(a.isHover() == true);

    /* A window position mapping outside the node (but inside the content
       area) misses it. */
    window.onEvent.publish(MouseMoveWindowEvent(window, (Vec2(200.0f, 200.0f) / factor), KeyModifiers{}));
    ASSERT_TRUE(a.isHover() == false);
}

/* At a fractional scale, values applied after Yoga's own pixel-grid rounding
   (offset, transform, scroll position, border widths) are snapped so they
   land on whole device pixels. */
TEST(Document, PixelGridSnapsPostLayoutValues) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.5f);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(17.0f);
    box.setBorderWidth(1.0f);
    box.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(box);

    auto tall = Node();
    tall.setWidth(50.0f);
    tall.setHeight(100.0f);
    box.appendChild(tall);

    auto centered = Node();
    centered.setPosition(NodePosition::Absolute);
    centered.setTop(NodeValue{ PercentValue{ 50.0f } });
    centered.setWidth(9.0f);
    centered.setHeight(9.0f);
    centered.setTransform(NodeTransform{ .translateY = PercentValue{ -50.0f } });
    box.appendChild(centered);

    auto lifted = Node();
    lifted.setWidth(10.0f);
    lifted.setHeight(10.0f);
    lifted.setOffset(Vec2{ 0.0f, -3.0f });
    document.appendChild(lifted);

    document.update();

    auto const isWholePixel = [](float points) {
        auto const pixels = (points * 1.5f);
        return std::fabsf(pixels - std::roundf(pixels)) < 0.001f;
    };

    ASSERT_TRUE(isWholePixel(centered.getComputedBorderRect().y));
    ASSERT_TRUE(isWholePixel(lifted.getComputedBorderRect().y));
    /* The 1pt border is 1.5px raw; snapped to 2px it moves the child to 1.333pt. */
    ASSERT_TRUE(tall.getComputedBorderRect().y * 1.5f == 2.0f);
    ASSERT_TRUE(box.getComputedBorderRect().height * 1.5f == 26.0f);

    /* A fractional wheel delta lands the child on a whole pixel. */
    window.onEvent.publish(MouseMoveWindowEvent(window, { 10.0f, 5.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -3.3f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(isWholePixel(tall.getComputedBorderRect().y));
    ASSERT_TRUE(tall.getComputedBorderRect().y < 0.0f);
}

/* render() smoke test: painting into a hidden window must not crash
   (drawable acquisition no-ops safely without a visible surface).
   Pixel output cannot be verified without a visible window. */
TEST(Document, RenderSmoke) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setBackground(Brush(ColorBrush{ .color = Vec4(1.0f, 0.0f, 0.0f, 1.0f) }));
    document.appendChild(a);

    document.update();
    document.render();
}

/* Every render is bracketed by exactly one window paint pass. */
TEST(Document, RenderWindowPassBracketsRender) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setBackground(ColorBrush{ .color = COLOR_WHITE });
    document.appendChild(a);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    InSequence seq;
    EXPECT_CALL(sink, beginPaint(TargetsWindow()));
    EXPECT_CALL(sink, paint(PaintsRect(Vec4{ 0.0f, 0.0f, 100.0f, 50.0f }), _, _));
    EXPECT_CALL(sink, endPaint());

    document.update();
    document.render();
}

/* Siblings paint in zIndex order, not tree order. */
TEST(Document, RenderZIndexOrdersPaints) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node(); /* first in the tree, but painted last (z 2) */
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setZIndex(2);
    a.setBackground(ColorBrush{ .color = COLOR_WHITE });
    document.appendChild(a);

    auto b = Node(); /* second in the tree, but painted first (z 1) */
    b.setWidth(200.0f);
    b.setHeight(50.0f);
    b.setZIndex(1);
    b.setBackground(ColorBrush{ .color = COLOR_WHITE });
    document.appendChild(b);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    InSequence seq;
    EXPECT_CALL(sink, paint(PaintsRect(Vec4{ 100.0f, 0.0f, 200.0f, 50.0f }), _, _)); /* b, z 1 */
    EXPECT_CALL(sink, paint(PaintsRect(Vec4{ 0.0f, 0.0f, 100.0f, 50.0f }), _, _));   /* a, z 2 */

    document.update();
    document.render();
}

/* A child overflowing a hidden-overflow parent paints with the parent's
   clip rect as its scissor. */
TEST(Document, RenderChildPaintScissoredToParentClip) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto parent = Node();
    parent.setWidth(100.0f);
    parent.setHeight(50.0f);
    parent.setOverflowX(NodeOverflow::Hidden);
    parent.setOverflowY(NodeOverflow::Hidden);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(200.0f);
    child.setHeight(200.0f);
    child.setBackground(ColorBrush{ .color = COLOR_WHITE });
    parent.appendChild(child);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    EXPECT_CALL(sink, paint(
        PaintsRect(Vec4{ 0.0f, 0.0f, 200.0f, 200.0f }),
        _,
        WithScissor(Vec4{ 0.0f, 0.0f, 100.0f, 50.0f })
    ));

    document.update();
    document.render();
}

/* Scrolling a scroll-overflow parent shifts where its child paints; the
   painted rect always matches the child's computed layout rect. */
TEST(Document, RenderScrollOffsetsChildPaints) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto parent = Node();
    parent.setWidth(100.0f);
    parent.setHeight(100.0f);
    parent.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(100.0f);
    child.setHeight(500.0f); /* taller than the parent: scrollable */
    child.setBackground(ColorBrush{ .color = COLOR_WHITE });
    parent.appendChild(child);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto childRects = std::vector<Vec4>();
    ON_CALL(sink, paint(_, _, _)).WillByDefault(Invoke(
        [&childRects](Shape const& shape, Brush const&, PaintOptions const&) {
            if (auto quad = shape.as<QuadShape>()) {
                if (quad->rect.width == 100.0f && quad->rect.height == 500.0f) {
                    childRects.push_back(quad->rect);
                }
            }
        }
    ));

    document.update();
    document.render();
    ASSERT_TRUE(childRects.size() == 1);

    /* hover the parent, then wheel-scroll it; window events are mapped into
       node space by (windowScale / documentScale), so publish the inverse */
    auto const windowPosition = (Vec2{ 50.0f, 50.0f } * (document.getScale() / window.getScale()));
    window.onEvent.publish(MouseMoveWindowEvent(window, windowPosition, KeyModifiers{}));
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -30.0f }, KeyModifiers{}));

    document.update();
    document.render();
    ASSERT_TRUE(childRects.size() == 2);

    /* the child moved on the y axis and still paints exactly at its computed rect */
    ASSERT_TRUE(childRects[1].y != childRects[0].y);
    ASSERT_TRUE(childRects[1] == child.getComputedBorderRect());
}

/* Border radii pass through to the painted background and border shapes,
   scaled by the document scale like the rects; unset corners stay unset
   (coalescing is the painter's job, not the renderer's). */
TEST(Document, RenderBorderRadiusScaledIntoShapes) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(2.0f);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setBackground(ColorBrush{ .color = COLOR_WHITE });
    a.setBorder(ColorBrush{ .color = COLOR_WHITE });
    a.setBorderWidth(1.0f);
    a.setBorderRadius(8.0f);
    a.setBorderTopLeftRadius(16.0f);
    document.appendChild(a);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto quads = std::vector<QuadShape>();
    auto outlines = std::vector<QuadOutlineShape>();
    ON_CALL(sink, paint(_, _, _)).WillByDefault(Invoke(
        [&quads, &outlines](Shape const& shape, Brush const&, PaintOptions const&) {
            if (auto quad = shape.as<QuadShape>()) {
                quads.push_back(*quad);
            }
            if (auto outline = shape.as<QuadOutlineShape>()) {
                outlines.push_back(*outline);
            }
        }
    ));

    document.update();
    document.render();

    /* the node's background quad, at 2x scale */
    ASSERT_TRUE(quads.size() == 1);
    ASSERT_TRUE(quads[0].rect == Vec4(0.0f, 0.0f, 200.0f, 100.0f));
    ASSERT_TRUE(quads[0].borderRadius.has_value());
    ASSERT_TRUE(quads[0].borderRadius.value() == 16.0f);
    ASSERT_TRUE(quads[0].borderTopLeftRadius.has_value());
    ASSERT_TRUE(quads[0].borderTopLeftRadius.value() == 32.0f);
    ASSERT_TRUE(!quads[0].borderTopRightRadius.has_value());
    ASSERT_TRUE(!quads[0].borderBottomLeftRadius.has_value());
    ASSERT_TRUE(!quads[0].borderBottomRightRadius.has_value());

    /* the node's border outline, same radii and 2x-scaled edge widths */
    ASSERT_TRUE(outlines.size() == 1);
    ASSERT_TRUE(outlines[0].rect == Vec4(0.0f, 0.0f, 200.0f, 100.0f));
    ASSERT_TRUE(outlines[0].borderRadius.has_value());
    ASSERT_TRUE(outlines[0].borderRadius.value() == 16.0f);
    ASSERT_TRUE(outlines[0].borderTopLeftRadius.has_value());
    ASSERT_TRUE(outlines[0].borderTopLeftRadius.value() == 32.0f);
    ASSERT_TRUE(!outlines[0].borderTopRightRadius.has_value());
    ASSERT_TRUE(!outlines[0].borderBottomLeftRadius.has_value());
    ASSERT_TRUE(!outlines[0].borderBottomRightRadius.has_value());
    ASSERT_TRUE(outlines[0].leftBorder.has_value());
    ASSERT_TRUE(outlines[0].leftBorder.value() == 2.0f);
    ASSERT_TRUE(outlines[0].topBorder.value() == 2.0f);
    ASSERT_TRUE(outlines[0].rightBorder.value() == 2.0f);
    ASSERT_TRUE(outlines[0].bottomBorder.value() == 2.0f);
}

/* An opacity < 1 subtree renders through an offscreen layer: a nested image
   pass bracketed inside the window pass, then composited back with the
   node's opacity. */
TEST(Document, RenderOpacityLayerLifecycle) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setOpacity(0.5f);
    a.setBackground(ColorBrush{ .color = COLOR_WHITE });
    document.appendChild(a);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    InSequence seq;
    EXPECT_CALL(sink, beginPaint(TargetsWindow()));
    EXPECT_CALL(sink, beginPaint(TargetsAnImage()));                    /* layer pass */
    EXPECT_CALL(sink, paint(_, _, _)).Times(AtLeast(1));                /* content into the layer */
    EXPECT_CALL(sink, endPaint());                                      /* layer pass ends */
    EXPECT_CALL(sink, paint(_, IsImageBrush(), WithOpacity(0.5f)));     /* composite */
    EXPECT_CALL(sink, endPaint());                                      /* window pass ends */

    document.update();
    document.render();
}

/* --------------------------- input scripts ------------------------------- */

// Internal helpers for synthesizing input scripts in Document tests: window
// events published through the public window.onEvent source, exactly as the
// platform layer would. Positions are window-space points; the document maps
// them into node space by (windowScale / documentScale), so with the default
// document scale they equal node points.

/** Moves the mouse to the given position. */
inline void _ScriptMove(Window& window, Vec2 const& position, KeyModifiers const& modifiers = {}) {
    window.onEvent.publish(MouseMoveWindowEvent(window, position, modifiers));
}

/** Clicks the left button at the given position (move, down, up). */
inline void _ScriptClick(Window& window, Vec2 const& position, KeyModifiers const& modifiers = {}) {
    window.onEvent.publish(MouseMoveWindowEvent(window, position, modifiers));
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, position, modifiers));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, position, modifiers));
}

/** Drags with the left button held from one position to another. */
inline void _ScriptDrag(Window& window, Vec2 const& from, Vec2 const& to, KeyModifiers const& modifiers = {}) {
    window.onEvent.publish(MouseMoveWindowEvent(window, from, modifiers));
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, from, modifiers));
    window.onEvent.publish(MouseMoveWindowEvent(window, to, modifiers));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, to, modifiers));
}

/** Presses and releases a key; like the OS, it commits its text as input unless it is a Command or Control shortcut. */
inline void _ScriptKey(Window& window, Rocket::Scancode const& key, KeyModifiers const& modifiers = {}, std::string const& input = "") {
    window.onEvent.publish(KeyDownWindowEvent(window, key, GetDefaultKeycode(key), modifiers));
    if (input.empty() == false && modifiers.meta == false && modifiers.control == false) {
        window.onEvent.publish(InputWindowEvent(window, input));
    }
    window.onEvent.publish(KeyUpWindowEvent(window, key, GetDefaultKeycode(key), modifiers));
}

/** Types a string as a single text-producing keypress. */
inline void _ScriptType(Window& window, std::string const& input) {
    _ScriptKey(window, Rocket::Scancode::Unknown, {}, input);
}

/* ------------------------------ layout seams ------------------------------
   Targeted at the integration seams around Yoga (text measurement, scale
   rounding, scroll/position interplay) — not at flexbox arithmetic, which is
   Yoga's own responsibility. Where behavior could diverge from the web, the
   expectations below assert the web semantics on purpose. */

static auto const _longText = std::string(
    "An attention-drawing message box with a themed background and an "
    "optional leading icon that makes this string comfortably longer than "
    "any slot it is measured into.");

/* Text in a flex row with fixed siblings wraps to its reduced slot instead of
   overflowing the row (the Callout overflow bug class). */
TEST(Document, LayoutTextInFlexRowStaysInSlot) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto row = Node();
    row.setWidth(360.0f);
    row.setDirection(NodeDirection::Horizontal);
    row.setPaddingLeft(12.0f);
    row.setPaddingRight(12.0f);
    document.appendChild(row);

    auto icon = Node();
    icon.setWidth(16.0f);
    icon.setHeight(16.0f);
    row.appendChild(icon);

    auto wrapper = Node(); /* the flex Column wrapper pattern from Callout */
    wrapper.setDirection(NodeDirection::Vertical);
    wrapper.setFlex(true);
    row.appendChild(wrapper);

    auto text = Node();
    text.setDisplay(NodeDisplay::Text);
    text.setContent(_longText);
    wrapper.appendChild(text);

    /* a second, very wide box with the same content for a wrap reference */
    auto wide = Node();
    wide.setWidth(4000.0f);
    document.appendChild(wide);

    auto wideText = Node();
    wideText.setDisplay(NodeDisplay::Text);
    wideText.setContent(_longText);
    wide.appendChild(wideText);

    document.update();

    auto const contentRight = (row.getComputedBorderRect().getMaxX() - 12.0f);
    ASSERT_TRUE(wrapper.getComputedBorderRect().getMaxX() <= contentRight);
    ASSERT_TRUE(text.getComputedBorderRect().getMaxX() <= contentRight);

    /* the narrow slot forced a wrap: same content is taller than in the wide box */
    ASSERT_TRUE(text.getComputedBorderRect().height > wideText.getComputedBorderRect().height);
}

/* Text measurement rounds at the physical-pixel boundary only: the measured
   logical width differs by less than one point across document scales. */
TEST(Document, LayoutTextMeasureScaleRounding) {
    auto measure = [](float scale) -> Vec2 {
        auto window = Window();
        window.setSize({ 640.0f, 480.0f });

        auto document = Document(window);
        document.setScale(scale);

        auto text = Node();
        text.setDisplay(NodeDisplay::Text);
        text.setContent("The quick brown fox");
        document.appendChild(text);

        document.update();
        return { text.getComputedBorderRect().width, text.getComputedBorderRect().height };
    };

    auto const at1 = measure(1.0f);
    auto const at2 = measure(2.0f);
    auto const at15 = measure(1.5f);

    ASSERT_TRUE(std::fabs(at2.x - at1.x) <= 1.0f);
    ASSERT_TRUE(std::fabs(at15.x - at1.x) <= 1.0f);
    ASSERT_TRUE(std::fabs(at2.y - at1.y) <= 1.0f);
    ASSERT_TRUE(std::fabs(at15.y - at1.y) <= 1.0f);
}

/* Percent widths resolve against the parent's content box (web semantics:
   padding is not part of the containing block for percentages). */
TEST(Document, LayoutPercentResolvesAgainstContentBox) {
    struct Row {
        float parentWidth;
        float parentPadding;
        float childPercent;
        float expectedChildWidth;
    };

    auto const rows = std::vector<Row>{
        { 640.0f,  0.0f, 50.0f, 320.0f },
        { 640.0f, 20.0f, 50.0f, 300.0f }, /* (640 - 2*20) * 50% */
        { 400.0f, 50.0f, 25.0f,  75.0f }, /* (400 - 2*50) * 25% */
    };

    for (auto const& row : rows) {
        SCOPED_TRACE(testing::Message()
            << "parent " << row.parentWidth << " padding " << row.parentPadding
            << " percent " << row.childPercent);

        auto window = Window();
        window.setSize({ 800.0f, 480.0f });

        auto document = Document(window);

        auto parent = Node();
        parent.setWidth(row.parentWidth);
        parent.setHeight(100.0f);
        parent.setPaddingLeft(row.parentPadding);
        parent.setPaddingRight(row.parentPadding);
        document.appendChild(parent);

        auto child = Node();
        child.setWidth(NodeValue(PercentValue{ row.childPercent }));
        child.setHeight(50.0f);
        parent.appendChild(child);

        document.update();
        ASSERT_TRUE(child.getComputedBorderRect().width == row.expectedChildWidth);
    }
}

/* The padding shorthand applies to all edges; a per-edge value overrides it on
   that edge, clearing the per-edge value falls back to the shorthand, and
   clearing the shorthand with no per-edge values leaves the edges unset. */
TEST(Document, LayoutPaddingShorthandCoalescesWithPerEdge) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(100.0f);
    parent.setPadding(10.0f);
    parent.setPaddingLeft(20.0f);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(50.0f);
    child.setHeight(50.0f);
    parent.appendChild(child);

    document.update();
    ASSERT_TRUE(child.getComputedBorderRect().x == 20.0f); /* per-edge override wins */
    ASSERT_TRUE(child.getComputedBorderRect().y == 10.0f); /* shorthand fills the unset edge */

    parent.setPaddingLeft(std::nullopt);
    document.update();
    ASSERT_TRUE(child.getComputedBorderRect().x == 10.0f); /* falls back to the shorthand */

    parent.setPadding(std::nullopt);
    document.update();
    ASSERT_TRUE(child.getComputedBorderRect().x == 0.0f);
    ASSERT_TRUE(child.getComputedBorderRect().y == 0.0f);
}

/* The gap shorthand applies to both gutters; gapX (the column gutter, i.e. the
   horizontal gap in a Row) overrides it, and clearing coalesces back. */
TEST(Document, LayoutGapShorthandCoalescesWithPerAxis) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setDirection(NodeDirection::Horizontal);
    parent.setWidth(200.0f);
    parent.setHeight(100.0f);
    parent.setGap(8.0f);
    document.appendChild(parent);

    auto first = Node();
    first.setWidth(50.0f);
    first.setHeight(50.0f);
    parent.appendChild(first);

    auto second = Node();
    second.setWidth(50.0f);
    second.setHeight(50.0f);
    parent.appendChild(second);

    document.update();
    ASSERT_TRUE(second.getComputedBorderRect().x == 58.0f); /* 50 + gap 8 */

    parent.setGapX(4.0f);
    document.update();
    ASSERT_TRUE(second.getComputedBorderRect().x == 54.0f); /* per-axis override wins */

    parent.setGapX(std::nullopt);
    document.update();
    ASSERT_TRUE(second.getComputedBorderRect().x == 58.0f); /* falls back to the shorthand */

    parent.setGap(std::nullopt);
    document.update();
    ASSERT_TRUE(second.getComputedBorderRect().x == 50.0f);
}

/* The border width shorthand applies to all edges; a per-edge value overrides
   it on that edge, clearing the per-edge value falls back to the shorthand, and
   clearing the shorthand with no per-edge values leaves the edges unset. */
TEST(Document, LayoutBorderWidthShorthandCoalescesWithPerEdge) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(100.0f);
    parent.setBorderWidth(5.0f);
    parent.setBorderLeftWidth(10.0f);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(50.0f);
    child.setHeight(50.0f);
    parent.appendChild(child);

    document.update();
    ASSERT_TRUE(child.getComputedBorderRect().x == 10.0f); /* per-edge override wins */
    ASSERT_TRUE(child.getComputedBorderRect().y == 5.0f); /* shorthand fills the unset edge */

    parent.setBorderLeftWidth(std::nullopt);
    document.update();
    ASSERT_TRUE(child.getComputedBorderRect().x == 5.0f); /* falls back to the shorthand */

    parent.setBorderWidth(std::nullopt);
    document.update();
    ASSERT_TRUE(child.getComputedBorderRect().x == 0.0f);
    ASSERT_TRUE(child.getComputedBorderRect().y == 0.0f);
}

/* Min/max constraints clamp text-measured sizes; clamping the width makes the
   same content wrap taller. */
TEST(Document, LayoutMinMaxClampWithText) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto clamped = Node();
    clamped.setMaxWidth(100.0f);
    document.appendChild(clamped);

    auto clampedText = Node();
    clampedText.setDisplay(NodeDisplay::Text);
    clampedText.setContent(_longText);
    clamped.appendChild(clampedText);

    auto wide = Node();
    wide.setWidth(4000.0f);
    document.appendChild(wide);

    auto wideText = Node();
    wideText.setDisplay(NodeDisplay::Text);
    wideText.setContent(_longText);
    wide.appendChild(wideText);

    auto minned = Node();
    minned.setMinWidth(200.0f);
    minned.setHeight(20.0f);
    document.appendChild(minned);

    auto minnedText = Node();
    minnedText.setDisplay(NodeDisplay::Text);
    minnedText.setContent("x");
    minned.appendChild(minnedText);

    document.update();

    ASSERT_TRUE(clamped.getComputedBorderRect().width <= 100.0f);
    ASSERT_TRUE(clampedText.getComputedBorderRect().height > wideText.getComputedBorderRect().height);
    ASSERT_TRUE(minned.getComputedBorderRect().width >= 200.0f);
}

/* Scrolling shifts relative and absolute children together (web: both scroll
   with the container); a fixed child stays window-anchored. */
TEST(Document, LayoutScrollShiftsRelativeAndAbsoluteNotFixed) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(100.0f);
    parent.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(parent);

    auto relative = Node();
    relative.setWidth(200.0f);
    relative.setHeight(500.0f); /* forces scrollable overflow */
    parent.appendChild(relative);

    auto absolute = Node();
    absolute.setPosition(NodePosition::Absolute);
    absolute.setTop(10.0f);
    absolute.setLeft(10.0f);
    absolute.setWidth(20.0f);
    absolute.setHeight(20.0f);
    parent.appendChild(absolute);

    auto fixed = Node();
    fixed.setPosition(NodePosition::Fixed);
    fixed.setTop(30.0f);
    fixed.setLeft(30.0f);
    fixed.setWidth(20.0f);
    fixed.setHeight(20.0f);
    parent.appendChild(fixed);

    document.update();

    auto const relativeBefore = relative.getComputedBorderRect();
    auto const absoluteBefore = absolute.getComputedBorderRect();
    auto const fixedBefore = fixed.getComputedBorderRect();

    _ScriptMove(window, { 50.0f, 50.0f });
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -30.0f }, KeyModifiers{}));
    document.update();

    auto const scrolled = (relativeBefore.y - relative.getComputedBorderRect().y);
    ASSERT_TRUE(scrolled > 0.0f);
    ASSERT_TRUE(absolute.getComputedBorderRect().y == (absoluteBefore.y - scrolled));
    ASSERT_TRUE(fixed.getComputedBorderRect().y == fixedBefore.y);
}

/* skip removes a node from layout entirely (display:none); visible false
   keeps its space (visibility:hidden). */
TEST(Document, LayoutSkipRemovesSpaceInvisibleKeepsIt) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    document.appendChild(b);

    auto c = Node();
    c.setWidth(100.0f);
    c.setHeight(50.0f);
    document.appendChild(c);

    document.update();
    ASSERT_TRUE(c.getComputedBorderRect().x == 200.0f);

    b.setSkip(true);
    document.update();
    ASSERT_TRUE(c.getComputedBorderRect().x == 100.0f); /* b's space is gone */

    b.setSkip(false);
    b.setVisible(false);
    document.update();
    ASSERT_TRUE(c.getComputedBorderRect().x == 200.0f); /* b's space is back */
}

/* Changing the document scale keeps logical rects stable: fixed-size nodes
   exactly, text-measured nodes within the one-point rounding bound. */
TEST(Document, LayoutSetScaleKeepsLogicalRectsStable) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto fixed = Node();
    fixed.setWidth(100.0f);
    fixed.setHeight(50.0f);
    document.appendChild(fixed);

    auto text = Node();
    text.setDisplay(NodeDisplay::Text);
    text.setContent("The quick brown fox");
    document.appendChild(text);

    document.setScale(1.0f);
    document.update();
    auto const fixedAt1 = fixed.getComputedBorderRect();
    auto const textAt1 = text.getComputedBorderRect();

    document.setScale(2.0f);
    document.update();

    ASSERT_TRUE(fixed.getComputedBorderRect() == fixedAt1);
    ASSERT_TRUE(std::fabs(text.getComputedBorderRect().width - textAt1.width) <= 1.0f);
    ASSERT_TRUE(std::fabs(text.getComputedBorderRect().height - textAt1.height) <= 1.0f);
}

/* --------------------- hit-testing / focus scenarios ---------------------- */

/* Overlapping siblings: the higher zIndex receives the pointer, regardless of
   tree order. */
TEST(Document, HitZIndexTopmostWins) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto high = Node(); /* first in the tree, but on top (z 2) */
    high.setPosition(NodePosition::Absolute);
    high.setTop(0.0f);
    high.setLeft(0.0f);
    high.setWidth(100.0f);
    high.setHeight(100.0f);
    high.setZIndex(2);
    document.appendChild(high);

    auto low = Node(); /* later in the tree, but underneath (z 1) */
    low.setPosition(NodePosition::Absolute);
    low.setTop(0.0f);
    low.setLeft(0.0f);
    low.setWidth(100.0f);
    low.setHeight(100.0f);
    low.setZIndex(1);
    document.appendChild(low);

    document.update();

    _ScriptMove(window, { 50.0f, 50.0f });
    ASSERT_TRUE(high.isHover() == true);
    ASSERT_TRUE(low.isHover() == false);
}

/* A child's area outside its parent's hidden-overflow clip is not hittable. */
TEST(Document, HitClippedAreaMisses) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(100.0f);
    parent.setHeight(50.0f);
    parent.setOverflowX(NodeOverflow::Hidden);
    parent.setOverflowY(NodeOverflow::Hidden);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(200.0f);
    child.setHeight(50.0f);
    parent.appendChild(child);

    document.update();

    _ScriptMove(window, { 50.0f, 25.0f }); /* inside the clip */
    ASSERT_TRUE(child.isHover() == true);

    _ScriptMove(window, { 150.0f, 25.0f }); /* inside the child, outside the clip */
    ASSERT_TRUE(child.isHover() == false);
    ASSERT_TRUE(parent.isHover() == false);
}

/* mouseEvents=false and visible=false make nodes transparent to hit-testing;
   the pointer falls through to what is underneath. */
TEST(Document, HitTransparencyFallsThrough) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(100.0f);
    parent.setHeight(100.0f);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(100.0f);
    child.setHeight(100.0f);
    parent.appendChild(child);

    document.update();

    _ScriptMove(window, { 50.0f, 50.0f });
    ASSERT_TRUE(child.isHover() == true);

    _ScriptMove(window, { 300.0f, 300.0f }); /* reset hover */

    child.setMouseEvents(false);
    _ScriptMove(window, { 50.0f, 50.0f });
    ASSERT_TRUE(child.isHover() == false);
    ASSERT_TRUE(parent.isHover() == true); /* falls through to the parent */

    _ScriptMove(window, { 300.0f, 300.0f });

    child.setMouseEvents(true);
    child.setVisible(false);
    document.update();
    _ScriptMove(window, { 50.0f, 50.0f });
    ASSERT_TRUE(child.isHover() == false);
    ASSERT_TRUE(parent.isHover() == true);
}

/* Moving between siblings exits one, enters the other, and leaves the common
   ancestor's hover untouched (no spurious exit/enter pair on it). */
TEST(Document, HoverEnterExitDivergingPaths) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(50.0f);
    parent.setDirection(NodeDirection::Horizontal);
    document.appendChild(parent);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    parent.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    parent.appendChild(b);

    document.update();

    /* events propagate through the ancestor chain, so record only the ones
       that target each node */
    auto parentEvents = std::vector<std::string>();
    auto aEvents = std::vector<std::string>();
    auto bEvents = std::vector<std::string>();
    auto parentSub = Sub<NodeEvent const&>(parent.onEvent, [&](NodeEvent const& e) { if (&e.getNode() == &parent) parentEvents.push_back(eventName(e)); });
    auto aSub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& e) { if (&e.getNode() == &a) aEvents.push_back(eventName(e)); });
    auto bSub = Sub<NodeEvent const&>(b.onEvent, [&](NodeEvent const& e) { if (&e.getNode() == &b) bEvents.push_back(eventName(e)); });

    _ScriptMove(window, { 50.0f, 25.0f }); /* over a */
    ASSERT_TRUE(aEvents == std::vector<std::string>({ "enter", "move" }));
    ASSERT_TRUE(parentEvents == std::vector<std::string>({ "enter" }));

    parentEvents.clear();
    aEvents.clear();

    _ScriptMove(window, { 150.0f, 25.0f }); /* over b */
    ASSERT_TRUE(aEvents == std::vector<std::string>({ "exit" }));
    ASSERT_TRUE(bEvents == std::vector<std::string>({ "enter", "move" }));
    ASSERT_TRUE(parent.isHover() == true);
    ASSERT_TRUE(parentEvents.empty()); /* the common ancestor saw no exit/enter */
}

/* While a drag is in progress, hover does not retarget: the node under the
   cursor gets no enter until the button is released. */
TEST(Document, HoverSuppressedWhileDragging) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    document.appendChild(b);

    document.update();

    auto bEvents = std::vector<std::string>();
    auto bSub = Sub<NodeEvent const&>(b.onEvent, [&](NodeEvent const& e) { bEvents.push_back(eventName(e)); });

    _ScriptMove(window, { 50.0f, 25.0f });
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseMoveWindowEvent(window, { 150.0f, 25.0f }, KeyModifiers{})); /* drag over b */
    ASSERT_TRUE(b.isHover() == false);
    ASSERT_TRUE(bEvents.empty());

    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 150.0f, 25.0f }, KeyModifiers{}));
    ASSERT_TRUE(b.isHover() == true); /* the post-drag synthetic move retargets */
    ASSERT_TRUE(bEvents.front() == "enter");
}

/* A press and release on different nodes clicks their deepest common
   ancestor (here: the shared parent), not either child. */
TEST(Document, ClickFiresOnCommonAncestor) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(50.0f);
    parent.setDirection(NodeDirection::Horizontal);
    document.appendChild(parent);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    parent.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    parent.appendChild(b);

    document.update();

    auto parentClicks = 0;
    auto childClicks = 0;
    auto parentSub = Sub<NodeEvent const&>(parent.onEvent, [&](NodeEvent const& e) {
        if (e.is<MouseClickNodeEvent>() && (&e.getNode() == &parent)) parentClicks += 1;
    });
    auto aSub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& e) {
        if (e.is<MouseClickNodeEvent>() && (&e.getNode() == &a)) childClicks += 1;
    });
    auto bSub = Sub<NodeEvent const&>(b.onEvent, [&](NodeEvent const& e) {
        if (e.is<MouseClickNodeEvent>() && (&e.getNode() == &b)) childClicks += 1;
    });

    /* press on a, release on b, no move in between (stays below the drag
       threshold, so the gesture is a click) */
    _ScriptMove(window, { 50.0f, 25.0f });
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 150.0f, 25.0f }, KeyModifiers{}));

    ASSERT_TRUE(parentClicks == 1);
    ASSERT_TRUE(childClicks == 0);
}

/* Focus marks the node and its ancestor chain; re-clicking inside keeps it;
   clicking empty space clears the whole chain. */
TEST(Document, FocusWithinChainAndRetention) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(100.0f);
    document.appendChild(parent);

    auto focusable = Node();
    focusable.setWidth(100.0f);
    focusable.setHeight(50.0f);
    focusable.setTabIndex(1);
    parent.appendChild(focusable);

    document.update();

    _ScriptClick(window, { 50.0f, 25.0f });
    ASSERT_TRUE(focusable.isFocused() == true);
    ASSERT_TRUE(focusable.isFocusedWithin() == true);
    ASSERT_TRUE(parent.isFocusedWithin() == true);
    ASSERT_TRUE(document.isFocusedWithin() == true);
    ASSERT_TRUE(parent.isFocused() == false);

    _ScriptClick(window, { 60.0f, 30.0f }); /* re-click inside: retained */
    ASSERT_TRUE(focusable.isFocused() == true);

    _ScriptClick(window, { 500.0f, 400.0f }); /* empty space: cleared */
    ASSERT_TRUE(focusable.isFocused() == false);
    ASSERT_TRUE(focusable.isFocusedWithin() == false);
    ASSERT_TRUE(parent.isFocusedWithin() == false);
}

/* Builds a focused, editable input surface: a focusable box whose first
   child is a text-display node holding the content. update() must run before
   keys are sent so the lazy Text object exists. */
static void _ScriptEditableSetup(Document& document, Node& box, Node& text, std::string const& content) {
    box.setWidth(200.0f);
    box.setHeight(30.0f);
    box.setTabIndex(1);
    box.setContentEditable(true);
    document.appendChild(box);

    text.setDisplay(NodeDisplay::Text);
    text.setContent(content);
    box.appendChild(text);

    document.update();
    document.focusNode(&box);
}

/* Editing surfaces are single-line by default: Enter inserts nothing (but
   still dispatches its KeyDownNodeEvent as the submit hook) and Tab is not
   consumed, while typing continues to work. */
TEST(Document, EditingSingleLineByDefaultEnterAndTabInsertNothing) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, "ab");

    auto enterKeyDowns = 0;
    auto sub = Sub<NodeEvent const&>();
    sub.on(box.onEvent, [&enterKeyDowns](NodeEvent const& event) {
        if (auto e = event.as<KeyDownNodeEvent>()) {
            if (e->getScancode() == Rocket::Scancode::Enter) enterKeyDowns += 1;
        }
    });

    _ScriptKey(window, Rocket::Scancode::End);
    _ScriptKey(window, Rocket::Scancode::Enter);
    ASSERT_TRUE(text.getContent() == "ab");
    ASSERT_TRUE(enterKeyDowns == 1);

    _ScriptKey(window, Rocket::Scancode::NumpadEnter);
    ASSERT_TRUE(text.getContent() == "ab");

    _ScriptKey(window, Rocket::Scancode::Tab);
    ASSERT_TRUE(text.getContent() == "ab");

    _ScriptKey(window, Rocket::Scancode::KeyC, {}, "c");
    ASSERT_TRUE(text.getContent() == "abc");
}

/* With multi-line mode enabled, Enter inserts a line break and Tab inserts
   spaces, restoring the full editing behaviour. */
TEST(Document, EditingMultiLineEnterInsertsLineBreak) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, "ab");
    box.setContentMultiLine(true);

    _ScriptKey(window, Rocket::Scancode::End);
    _ScriptKey(window, Rocket::Scancode::Enter);
    ASSERT_TRUE(text.getContent() == "ab\n");

    _ScriptKey(window, Rocket::Scancode::Tab);
    ASSERT_TRUE(text.getContent() == "ab\n    ");
}

/* Single-line paste replaces line breaks (\n, \r\n, \r) with single spaces;
   multi-line paste keeps them, normalised to \n. */
TEST(Document, EditingSingleLinePasteStripsLineBreaks) {
    /* Preserve whatever text the user currently has on the clipboard. */
    auto original = GetClipboardString();

    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, "");

    SetClipboardString("a\nb\r\nc\rd");
    _ScriptKey(window, Rocket::Scancode::KeyV, KeyModifiers{ .meta = true }, "v");
    ASSERT_TRUE(text.getContent() == "a b c d");

    box.setContentMultiLine(true);
    _ScriptKey(window, Rocket::Scancode::KeyA, KeyModifiers{ .meta = true }, "a");
    _ScriptKey(window, Rocket::Scancode::KeyV, KeyModifiers{ .meta = true }, "v");
    ASSERT_TRUE(text.getContent() == "a\nb\nc\nd");

    SetClipboardString(original);
}

/* Single-line typed input is sanitized too: line breaks arriving through the
   text-input path become spaces. */
TEST(Document, EditingSingleLineTypedLineBreaksBecomeSpaces) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, "");

    _ScriptKey(window, Rocket::Scancode::KeyX, {}, "x\ny");
    ASSERT_TRUE(text.getContent() == "x y");
}

/* In single-line mode ArrowUp/ArrowDown move the caret to the line start and
   end instead of switching lines. */
TEST(Document, EditingSingleLineArrowsMoveToLineEdges) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, "bc");

    _ScriptKey(window, Rocket::Scancode::ArrowUp);
    _ScriptKey(window, Rocket::Scancode::KeyA, {}, "a");
    ASSERT_TRUE(text.getContent() == "abc");

    _ScriptKey(window, Rocket::Scancode::ArrowDown);
    _ScriptKey(window, Rocket::Scancode::KeyD, {}, "d");
    ASSERT_TRUE(text.getContent() == "abcd");
}

/* Wheel scrolling clamps at both content edges, delivers wheel events to the
   hovered node, and hit-testing tracks the scrolled positions. */
TEST(Document, ScrollClampsAndRetargetsHits) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(100.0f);
    parent.setHeight(100.0f);
    parent.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(100.0f);
    child.setHeight(500.0f);
    parent.appendChild(child);

    auto top = Node(); /* marker at the top of the content */
    top.setPosition(NodePosition::Absolute);
    top.setTop(0.0f);
    top.setLeft(0.0f);
    top.setWidth(100.0f);
    top.setHeight(50.0f);
    child.appendChild(top);

    auto bottom = Node(); /* marker 400pt down: scrolled into view at max */
    bottom.setPosition(NodePosition::Absolute);
    bottom.setTop(400.0f);
    bottom.setLeft(0.0f);
    bottom.setWidth(100.0f);
    bottom.setHeight(50.0f);
    child.appendChild(bottom);

    document.update();

    auto wheels = 0;
    auto topSub = Sub<NodeEvent const&>(top.onEvent, [&](NodeEvent const& e) {
        if (e.is<MouseWheelNodeEvent>()) wheels += 1;
    });

    auto const parentRect = parent.getComputedBorderRect();

    _ScriptMove(window, { 50.0f, 25.0f });
    ASSERT_TRUE(top.isHover() == true);

    /* a huge scroll clamps at the far edge: content bottom == parent bottom */
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -10000.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(wheels == 1); /* delivered to the hovered marker */
    ASSERT_TRUE(child.getComputedBorderRect().getMaxY() == parentRect.getMaxY());

    /* the same point now hits the marker that scrolled into view */
    _ScriptMove(window, { 50.0f, 25.0f });
    ASSERT_TRUE(top.isHover() == false);
    ASSERT_TRUE(bottom.isHover() == true);

    /* a huge reverse scroll clamps back at zero */
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, 10000.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(child.getComputedBorderRect().y == parentRect.y);
}

/* A shadowed node bakes its shadow into an offscreen image once; repaints
   reuse the cached bake until the shadow parameters change. */
TEST(Document, RenderShadowRebakesOnlyOnChange) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setBackground(ColorBrush{ .color = COLOR_WHITE });
    a.setShadow(Shadow{ .blur = 4.0f });
    document.appendChild(a);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto imagePasses = 0;
    ON_CALL(sink, beginPaint(_)).WillByDefault(Invoke([&imagePasses](PaintTarget const& target) {
        if (target.as<ImagePaintTarget>() != nullptr) imagePasses += 1;
    }));

    /* first render: the layer pass plus the shadow bake pass */
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 2);

    /* an unrelated invalidation repaints the layer but reuses the bake */
    a.setBackground(ColorBrush{ .color = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } });
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 3);

    /* changing the shadow rebakes */
    a.setShadow(Shadow{ .blur = 8.0f });
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 5);
}

/* Changing the effective corner radii rebakes the cached shadow; a change
   that leaves the effective per-corner radii identical does not. */
TEST(Document, RenderShadowRebakesOnBorderRadiusChange) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setBackground(ColorBrush{ .color = COLOR_WHITE });
    a.setShadow(Shadow{ .blur = 4.0f });
    document.appendChild(a);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto imagePasses = 0;
    ON_CALL(sink, beginPaint(_)).WillByDefault(Invoke([&imagePasses](PaintTarget const& target) {
        if (target.as<ImagePaintTarget>() != nullptr) imagePasses += 1;
    }));

    /* first render: the layer pass plus the shadow bake pass */
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 2);

    /* nothing changed: render early-outs, nothing repaints */
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 2);

    /* an unrelated invalidation repaints the layer but reuses the bake */
    a.setBackground(ColorBrush{ .color = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } });
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 3);

    /* changing the uniform radius rebakes */
    a.setBorderRadius(12.0f);
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 5);

    /* switching uniform 12 to explicit per-corner 12/12/12/12 leaves the
       effective radii unchanged: no rebake */
    a.setBorderRadius(std::nullopt);
    a.setBorderTopLeftRadius(12.0f);
    a.setBorderTopRightRadius(12.0f);
    a.setBorderBottomLeftRadius(12.0f);
    a.setBorderBottomRightRadius(12.0f);
    document.update();
    document.render();
    ASSERT_TRUE(imagePasses == 6);
}

/* The layer and shadow composites of a shadowed node inside a scroll viewport
   carry the ancestor clip as their scissor: a shadow scrolled out of the
   viewport cannot ghost onto the window outside it. The composites are the
   image-brush draws issued directly into the window pass (depth 1); in-layer
   paints happen inside nested image passes (depth 2). */
TEST(Document, RenderShadowCompositeScissoredToScrollViewport) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);
    document.setDirection(NodeDirection::Horizontal);

    /* pushes the viewport off the window origin, so a chip scrolled out to
       the left lands on visible window area where a ghost would show */
    auto lead = Node();
    lead.setWidth(200.0f);
    lead.setHeight(100.0f);
    document.appendChild(lead);

    auto container = Node(); /* the scroll viewport */
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setOverflowX(NodeOverflow::Scroll);
    container.setOverflowY(NodeOverflow::Hidden);
    document.appendChild(container);

    auto row = Node();
    row.setDirection(NodeDirection::Horizontal);
    row.setWidth(400.0f); /* wider than the viewport: scrollable */
    row.setHeight(50.0f);
    container.appendChild(row);

    auto chip = Node(); /* shadowed; starts fully inside the viewport */
    chip.setWidth(100.0f);
    chip.setHeight(50.0f);
    chip.setBackground(ColorBrush{ .color = COLOR_WHITE });
    chip.setShadow(Shadow{ .blur = 4.0f });
    row.appendChild(chip);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    struct Composite {
        Vec4 rect;
        std::optional<Vec4> scissor;
    };

    auto depth = 0;
    auto composites = std::vector<Composite>();
    ON_CALL(sink, beginPaint(_)).WillByDefault(Invoke([&depth](PaintTarget const&) { depth += 1; }));
    ON_CALL(sink, endPaint()).WillByDefault(Invoke([&depth]() { depth -= 1; }));
    ON_CALL(sink, paint(_, _, _)).WillByDefault(Invoke(
        [&](Shape const& shape, Brush const& brush, PaintOptions const& options) {
            auto const quad = shape.as<QuadShape>();
            if ((depth == 1) && (quad != nullptr) && (brush.as<ImageBrush>() != nullptr)) {
                composites.push_back({ quad->rect, options.scissor });
            }
        }
    ));

    document.update();
    document.render();

    auto const viewport = container.getComputedBorderRect();
    ASSERT_TRUE(viewport == Vec4(200.0f, 0.0f, 100.0f, 100.0f));

    /* the shadow image and the layer image composite onto the window, both
       scissored to the viewport (not to the chip's own rect: the shadow blur
       must still be able to paint outside the chip's border rect) */
    ASSERT_TRUE(composites.size() == 2);
    for (auto const& composite : composites) {
        ASSERT_TRUE(composite.scissor.has_value());
        ASSERT_TRUE(composite.scissor.value() == viewport);
    }

    /* scroll the chip fully out of the viewport (clamped at the overflow);
       window events are mapped into node space by (windowScale / documentScale),
       so publish the inverse */
    auto const windowPosition = (Vec2{ 250.0f, 75.0f } * (document.getScale() / window.getScale()));
    window.onEvent.publish(MouseMoveWindowEvent(window, windowPosition, KeyModifiers{}));
    window.onEvent.publish(MouseWheelWindowEvent(window, { -10000.0f, 0.0f }, KeyModifiers{}));

    composites.clear();
    document.update();
    document.render();
    ASSERT_TRUE(chip.getComputedBorderRect().getMaxX() <= viewport.x);

    /* everything composited for the chip now lies left of the viewport, on
       otherwise-visible window area — and every composite is scissored to the
       viewport, so no ghost shadow can appear */
    ASSERT_TRUE(composites.size() == 2);
    for (auto const& composite : composites) {
        ASSERT_TRUE(composite.rect.getMaxX() <= viewport.x);
        ASSERT_TRUE(composite.scissor.has_value());
        ASSERT_TRUE(composite.scissor.value() == viewport);
    }
}

/* The baked shadow is keyed on the clip actually covering the layer: scrolling
   a shadowed node partially out of its viewport rebakes the truncated
   silhouette, and scrolling it back in rebakes the full one — a stale
   part-clipped shadow must not survive on a fully visible node. Bakes are the
   ShadowFilter paints; the layer image repainting on every scrolled frame is
   separate and deliberately not counted. */
TEST(Document, RenderShadowRebakesOnViewportClipChange) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto container = Node(); /* the scroll viewport */
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(container);

    auto chip = Node(); /* shadowed; starts fully inside the viewport */
    chip.setWidth(100.0f);
    chip.setHeight(50.0f);
    chip.setBackground(ColorBrush{ .color = COLOR_WHITE });
    chip.setShadow(Shadow{ .blur = 4.0f });
    container.appendChild(chip);

    auto tall = Node(); /* forces scrollable overflow */
    tall.setWidth(100.0f);
    tall.setHeight(400.0f);
    container.appendChild(tall);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto bakes = 0;
    ON_CALL(sink, paint(_, _, _)).WillByDefault(Invoke(
        [&bakes](Shape const&, Brush const&, PaintOptions const& options) {
            if (options.filter.has_value() && (options.filter->as<ShadowFilter>() != nullptr)) {
                bakes += 1;
            }
        }
    ));

    /* fully visible: one bake of the full silhouette */
    document.update();
    document.render();
    ASSERT_TRUE(bakes == 1);

    /* scroll the chip partially out of the viewport: the layer content is
       clipped differently now, so the shadow rebakes. Wheel deltas scale by
       (windowScale / documentScale), so keep the delta small enough that the
       chip stays partially visible at any window scale. */
    auto const windowPosition = (Vec2{ 50.0f, 75.0f } * (document.getScale() / window.getScale()));
    window.onEvent.publish(MouseMoveWindowEvent(window, windowPosition, KeyModifiers{}));
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -10.0f }, KeyModifiers{}));
    document.update();
    document.render();
    ASSERT_TRUE(chip.getComputedBorderRect().y < 0.0f);          /* partially out */
    ASSERT_TRUE(chip.getComputedBorderRect().getMaxY() > 0.0f);  /* still visible */
    ASSERT_TRUE(bakes == 2);

    /* scroll back to fully visible: the full silhouette is rebaked — the
       stale truncated shadow must not be reused */
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, 1000.0f }, KeyModifiers{}));
    document.update();
    document.render();
    ASSERT_TRUE(chip.getComputedBorderRect().y == 0.0f);
    ASSERT_TRUE(bakes == 3);

    /* steady state: nothing changed, no further bakes */
    document.update();
    document.render();
    ASSERT_TRUE(bakes == 3);
}

/* A shadowed node with no scrolling ancestor composites with the document-
   sized clip as its scissor — wide enough for the shadow blur to paint outside
   the node's own border rect (shadows must not be over-clipped to it). */
TEST(Document, RenderShadowCompositeNotClippedToOwnRect) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setBackground(ColorBrush{ .color = COLOR_WHITE });
    a.setShadow(Shadow{ .blur = 4.0f });
    document.appendChild(a);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    struct Composite {
        Vec4 rect;
        std::optional<Vec4> scissor;
    };

    auto depth = 0;
    auto composites = std::vector<Composite>();
    ON_CALL(sink, beginPaint(_)).WillByDefault(Invoke([&depth](PaintTarget const&) { depth += 1; }));
    ON_CALL(sink, endPaint()).WillByDefault(Invoke([&depth]() { depth -= 1; }));
    ON_CALL(sink, paint(_, _, _)).WillByDefault(Invoke(
        [&](Shape const& shape, Brush const& brush, PaintOptions const& options) {
            auto const quad = shape.as<QuadShape>();
            if ((depth == 1) && (quad != nullptr) && (brush.as<ImageBrush>() != nullptr)) {
                composites.push_back({ quad->rect, options.scissor });
            }
        }
    ));

    document.update();
    document.render();

    /* the shadow image composites first, padded by the blur beyond the node's
       border rect on every side; the layer image follows at the node's rect */
    ASSERT_TRUE(composites.size() == 2);
    ASSERT_TRUE(composites[0].rect == Vec4(-4.0f, -4.0f, 108.0f, 58.0f));
    ASSERT_TRUE(composites[1].rect == Vec4(0.0f, 0.0f, 100.0f, 50.0f));

    /* both scissors are the document-sized clip, which contains the whole
       shadow rect within the window — not the node's own 100x50 rect */
    auto const documentClip = Vec4{ Vec2{}, document.getSize() };
    for (auto const& composite : composites) {
        ASSERT_TRUE(composite.scissor.has_value());
        ASSERT_TRUE(composite.scissor.value() == documentClip);
    }
}

/* --- invalidation wake and idempotency (regression tests for the flag-based
   invalidation pipeline: setters only flag their own node, so the update pass
   must discover the flags and wake itself) --- */

/* A property change made while the document is idle wakes the next update. */
TEST(Document, UpdateWakesOnPropChangeWhileIdle) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    document.appendChild(box);

    document.update(); /* now idle */

    box.setWidth(200.0f);
    document.update();
    ASSERT_TRUE(box.getComputedBorderRect().width == 200.0f);
}

/* Removing a child wakes the document and relayouts the remaining siblings. */
TEST(Document, RemoveChildRelayoutsParent) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto first = Node();
    first.setWidth(100.0f);
    first.setHeight(50.0f);
    document.appendChild(first);

    auto second = Node();
    second.setWidth(100.0f);
    second.setHeight(50.0f);
    document.appendChild(second);

    document.update();
    ASSERT_TRUE(second.getComputedBorderRect().x == 100.0f);

    document.removeChild(first);
    document.update();
    ASSERT_TRUE(second.getComputedBorderRect().x == 0.0f);
}

/* With nothing changed, update() and render() are no-ops: no repaint happens
   and computed geometry stays identical. */
TEST(Document, UpdateAndRenderAreIdempotentWhenNothingChanged) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(1.0f);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    box.setBackground(ColorBrush{ .color = COLOR_WHITE });
    document.appendChild(box);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto paintPasses = 0;
    ON_CALL(sink, beginPaint(_)).WillByDefault(Invoke([&paintPasses](PaintTarget const&) {
        paintPasses += 1;
    }));

    document.update();
    document.render();
    auto const passesAfterFirst = paintPasses;
    auto const rectAfterFirst = box.getComputedBorderRect();

    document.update();
    document.render();
    ASSERT_TRUE(paintPasses == passesAfterFirst);
    ASSERT_TRUE(box.getComputedBorderRect() == rectAfterFirst);
}

/* --- detach/reattach semantics: detach drops all derived state --- */

/* Detaching a node resets its computed geometry to zeros. */
TEST(Document, DetachResetsComputedState) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    document.appendChild(box);

    document.update();
    ASSERT_TRUE(box.getComputedBorderRect().width == 100.0f);

    document.removeChild(box);
    ASSERT_TRUE(box.getComputedBorderRect() == Vec4{});
}

/* Detaching a scrolled node resets its scroll position: after reattach the
   content starts at the top again. */
TEST(Document, DetachResetsScrollPosition) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto viewport = Node();
    viewport.setWidth(100.0f);
    viewport.setHeight(100.0f);
    viewport.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(viewport);

    auto content = Node();
    content.setWidth(100.0f);
    content.setHeight(300.0f);
    viewport.appendChild(content);

    document.update();
    document.scrollNode(content, { 0.0f, -50.0f });
    document.update();
    ASSERT_TRUE(content.getComputedBorderRect().y == -50.0f);

    document.removeChild(viewport);
    document.appendChild(viewport);
    document.update();
    ASSERT_TRUE(content.getComputedBorderRect().y == 0.0f);
}

/* A text node detached and reattached measures again: its Text object is
   destroyed on detach and lazily rebuilt by the next layout. */
TEST(Document, ReattachedTextNodeRemeasures) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    document.appendChild(box);

    auto text = Node();
    text.setDisplay(NodeDisplay::Text);
    text.setContent("hello");
    box.appendChild(text);

    document.update();
    auto const measured = text.getComputedBorderRect().size;
    ASSERT_TRUE(measured.width > 0.0f);
    ASSERT_TRUE(measured.height > 0.0f);

    document.removeChild(box);
    document.appendChild(box);
    document.update();
    ASSERT_TRUE(text.getComputedBorderRect().size == measured);
}

/* --- event payloads: positions are document coordinates, modifiers, keys and
   deltas arrive as sent --- */

/* Mouse down/up/click deliver the document-space position, the button, and
   the modifiers they were synthesized with. */
TEST(Document, MouseEventPayloadsArriveAsSent) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(200.0f);
    box.setHeight(100.0f);
    document.appendChild(box);

    document.update();

    auto downPosition = Vec2{};
    auto downMouse = Mouse::RightButton; /* any non-left value */
    auto downShift = false;
    auto clickPosition = Vec2{};
    auto movePosition = Vec2{};
    auto wheelDelta = Vec2{};

    auto sub = Sub<NodeEvent const&>();
    sub.on(box.onEvent, [&](NodeEvent const& event) {
        if (auto e = event.as<MouseDownNodeEvent>()) {
            downPosition = e->getPosition();
            downMouse = e->getMouse();
            downShift = e->getModifiers().shift;
        } else if (auto e = event.as<MouseClickNodeEvent>()) {
            clickPosition = e->getPosition();
        } else if (auto e = event.as<MouseMoveNodeEvent>()) {
            movePosition = e->getPosition();
        } else if (auto e = event.as<MouseWheelNodeEvent>()) {
            wheelDelta = e->getWheel();
        }
    });

    window.onEvent.publish(MouseMoveWindowEvent(window, { 50.0f, 25.0f }, KeyModifiers{}));
    ASSERT_TRUE(movePosition == Vec2(50.0f, 25.0f));

    window.onEvent.publish(MouseWheelWindowEvent(window, { 3.0f, -7.0f }, KeyModifiers{}));
    ASSERT_TRUE(wheelDelta == Vec2(3.0f, -7.0f));

    _ScriptClick(window, { 60.0f, 30.0f }, KeyModifiers{ .shift = true });
    ASSERT_TRUE(downPosition == Vec2(60.0f, 30.0f));
    ASSERT_TRUE(downMouse == Mouse::LeftButton);
    ASSERT_TRUE(downShift == true);
    ASSERT_TRUE(clickPosition == Vec2(60.0f, 30.0f));
}

/* Key events deliver the key, the modifiers, and the produced input string. */
TEST(Document, KeyEventPayloadsArriveAsSent) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    box.setTabIndex(1);
    document.appendChild(box);

    document.update();
    document.focusNode(&box);

    auto key = Rocket::Scancode::Unknown;
    auto keycode = std::string();
    auto meta = false;
    auto upKey = Rocket::Scancode::Unknown;

    auto sub = Sub<NodeEvent const&>();
    sub.on(box.onEvent, [&](NodeEvent const& event) {
        if (auto e = event.as<KeyDownNodeEvent>()) {
            key = e->getScancode();
            keycode = e->getKeycode();
            meta = e->getModifiers().meta;
        } else if (auto e = event.as<KeyUpNodeEvent>()) {
            upKey = e->getScancode();
        }
    });

    _ScriptKey(window, Rocket::Scancode::KeyQ, KeyModifiers{ .meta = true }, "q");
    ASSERT_TRUE(key == Rocket::Scancode::KeyQ);
    ASSERT_TRUE(keycode == Keycode::Q);
    ASSERT_TRUE(meta == true);
    ASSERT_TRUE(upKey == Rocket::Scancode::KeyQ);
}

/* A focused node with key events disabled emits no key events and does not
   edit its text; the key events go to the document instead. */
TEST(Document, KeyEventsDisabled) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    box.setTabIndex(1);
    box.setKeyEvents(false);
    document.appendChild(box);

    document.update();
    document.focusNode(&box);

    auto boxEvents = std::vector<std::string>();
    auto documentEvents = std::vector<std::string>();
    auto boxSub = Sub<NodeEvent const&>(box.onEvent, [&](NodeEvent const& event) { boxEvents.push_back(eventName(event)); });
    auto documentSub = Sub<NodeEvent const&>(document.onEvent, [&](NodeEvent const& event) { documentEvents.push_back(eventName(event)); });

    _ScriptKey(window, Rocket::Scancode::KeyA, KeyModifiers{}, "a");
    ASSERT_TRUE(box.isFocused() == true);
    ASSERT_TRUE(boxEvents.empty());
    ASSERT_TRUE(documentEvents == std::vector<std::string>({ "keydown", "keyup" }));

    box.setKeyEvents(true);
    _ScriptKey(window, Rocket::Scancode::KeyA, KeyModifiers{}, "a");
    ASSERT_TRUE(boxEvents == std::vector<std::string>({ "keydown", "keyup" }));
}

/* Removing a text span re-measures the text node that contained it. */
TEST(Document, RemoveChildRemeasuresText) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto text = Node();
    text.setDisplay(NodeDisplay::Text);
    text.setContent("hello ");
    document.appendChild(text);

    auto span = Node();
    span.setDisplay(NodeDisplay::Text);
    span.setContent("world, a much longer span");
    text.appendChild(span);

    document.update();
    auto const withSpan = text.getComputedBorderRect().width;

    text.removeChild(span);
    document.update();
    ASSERT_TRUE(text.getComputedBorderRect().width < withSpan);
}

/* scrollNodeIntoView scrolls the nearest scroll container by the least
   amount that shows the node in full; a node taller than the container is
   aligned to its top; a tree without a scroll container is left alone. */
TEST(Document, ScrollNodeIntoView) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto container = Node();
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setDirection(NodeDirection::Vertical);
    container.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(container);

    auto a = Node();
    auto b = Node();
    auto c = Node();
    auto d = Node();
    for (auto node : { &a, &b, &c }) {
        node->setWidth(100.0f);
        node->setHeight(80.0f);
        container.appendChild(*node);
    }
    d.setWidth(100.0f);
    d.setHeight(300.0f);
    container.appendChild(d);

    auto inner = Node();
    inner.setWidth(20.0f);
    inner.setHeight(20.0f);
    c.appendChild(inner);

    document.update();
    ASSERT_TRUE(c.getComputedBorderRect().y == 160.0f);

    /* c ends at 240: scroll down by 140 so its bottom meets the container's. */
    document.scrollNodeIntoView(c);
    document.update();
    ASSERT_TRUE(c.getComputedBorderRect().y == 20.0f);

    /* Already fully visible: nothing moves. */
    document.scrollNodeIntoView(c);
    document.update();
    ASSERT_TRUE(c.getComputedBorderRect().y == 20.0f);

    /* a starts above the viewport: scroll back up by 140. */
    document.scrollNodeIntoView(a);
    document.update();
    ASSERT_TRUE(a.getComputedBorderRect().y == 0.0f);

    /* d is taller than the container: align its top. */
    document.scrollNodeIntoView(d);
    document.update();
    ASSERT_TRUE(d.getComputedBorderRect().y == 0.0f);

    /* A nested node scrolls the first scrollable ancestor, not its parent. */
    document.scrollNodeIntoView(inner);
    document.update();
    ASSERT_TRUE(inner.getComputedBorderRect().y == 0.0f);
    ASSERT_TRUE(c.getComputedBorderRect().y == 0.0f);

    /* No scroll container above: a no-op. */
    auto loose = Node();
    loose.setWidth(10.0f);
    loose.setHeight(10.0f);
    document.appendChild(loose);
    document.scrollNodeIntoView(loose);
    document.update();
    ASSERT_TRUE(c.getComputedBorderRect().y == 0.0f);
}

/* ------------------------- interaction state machine -------------------------
   Pins the input state machine's guarantees: listeners may update or change
   the tree while events are dispatched, the pointer can leave the window,
   nodes can be removed mid-gesture, and layout can move nodes under a still
   pointer. */

namespace {

std::string _EventNameWithFocus(NodeEvent const& event) {
    if (event.is<FocusNodeEvent>()) return "focus";
    if (event.is<BlurNodeEvent>())  return "blur";
    return eventName(event);
}

} /* namespace */

/* An enter or exit listener that forces an update sees consistent state, and
   every hover event still fires exactly once. */
TEST(Document, UpdateFromHoverListenerIsSafe) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    document.appendChild(b);

    document.update();

    auto aEvents = std::vector<std::string>();
    auto bEvents = std::vector<std::string>();
    auto toggle = false;

    auto const listen = [&](Node& node, std::vector<std::string>& events) {
        return Sub<NodeEvent const&>(node.onEvent, [&](NodeEvent const& event) {
            if (&event.getNode() != &node) return;
            events.push_back(eventName(event));
            if (event.is<MouseEnterNodeEvent>() || event.is<MouseExitNodeEvent>()) {
                toggle = !toggle;
                node.setHeight(toggle ? 51.0f : 50.0f);
                document.update();
            }
        });
    };

    auto aSub = listen(a, aEvents);
    auto bSub = listen(b, bEvents);

    _ScriptMove(window, { 50.0f, 25.0f });
    _ScriptMove(window, { 150.0f, 25.0f });
    _ScriptMove(window, { 500.0f, 400.0f });

    ASSERT_TRUE(aEvents == std::vector<std::string>({ "enter", "move", "exit" }));
    ASSERT_TRUE(bEvents == std::vector<std::string>({ "enter", "move", "exit" }));
    ASSERT_TRUE(a.isHover() == false);
    ASSERT_TRUE(b.isHover() == false);
    ASSERT_TRUE(document.isHover() == false);
}

/* An enter listener that hides its node and updates moves the pointer off it:
   events already queued for the node (its move) still arrive in order, then
   its exit, and nothing after it. */
TEST(Document, EnterListenerHidingNodeEndsHover) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() != &a) return;
        events.push_back(eventName(event));
        if (event.is<MouseEnterNodeEvent>()) {
            a.setVisible(false);
            document.update();
        }
    });

    _ScriptMove(window, { 50.0f, 25.0f });

    ASSERT_TRUE(events == std::vector<std::string>({ "enter", "move", "exit" }));
    ASSERT_TRUE(a.isHover() == false);
}

/* A child's exit listener that destroys the child's parent: the parent is
   dropped from the pending exits instead of being dispatched to. */
TEST(Document, ExitListenerDestroyingAncestorIsSafe) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = std::make_unique<Node>();
    parent->setWidth(200.0f);
    parent->setHeight(100.0f);
    document.appendChild(*parent);

    auto child = Node();
    child.setWidth(100.0f);
    child.setHeight(50.0f);
    parent->appendChild(child);

    document.update();

    _ScriptMove(window, { 50.0f, 25.0f });
    ASSERT_TRUE(child.isHover() == true);

    auto sub = Sub<NodeEvent const&>(child.onEvent, [&](NodeEvent const& event) {
        if (event.is<MouseExitNodeEvent>() && (&event.getNode() == &child)) {
            parent->removeChild(child);
            parent = nullptr;
        }
    });

    _ScriptMove(window, { 500.0f, 400.0f });

    ASSERT_TRUE(parent == nullptr);
    ASSERT_TRUE(child.isHover() == false);
    ASSERT_TRUE(document.isHover() == false);
}

/* A begin-drag listener that removes the dragged node ends the drag: no
   further drag events reach it and nothing dereferences the removed node. */
TEST(Document, BeginDragListenerRemovingNodeIsSafe) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() != &a) return;
        events.push_back(eventName(event));
        if (event.is<MouseBeginDragNodeEvent>()) {
            a.removeFromParent();
        }
    });

    _ScriptDrag(window, { 10.0f, 10.0f }, { 60.0f, 10.0f });
    _ScriptMove(window, { 80.0f, 10.0f });

    ASSERT_TRUE(events == std::vector<std::string>({ "enter", "move", "down", "begindrag" }));
    ASSERT_TRUE(document.isActive() == false);
}

/* A click listener that removes the clicked node leaves no stale state. */
TEST(Document, ClickListenerRemovingNodeIsSafe) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setTabIndex(1);
    document.appendChild(a);

    document.update();

    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (event.is<MouseClickNodeEvent>()) {
            a.removeFromParent();
            document.update();
        }
    });

    _ScriptClick(window, { 50.0f, 25.0f });

    ASSERT_TRUE(a.getParent() == nullptr);
    ASSERT_TRUE(a.isHover() == false);
    ASSERT_TRUE(a.isActive() == false);
    ASSERT_TRUE(a.isFocused() == false);
    ASSERT_TRUE(document.isActive() == false);
    ASSERT_TRUE(document.isFocusedWithin() == false);

    _ScriptClick(window, { 50.0f, 25.0f });
    ASSERT_TRUE(document.isActive() == false);
}

/* A blur listener that moves focus elsewhere wins: the last focus event names
   the node that ends up focused. */
TEST(Document, BlurListenerRedirectingFocusWins) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    auto b = Node();
    auto c = Node();
    for (auto node : { &a, &b, &c }) {
        node->setWidth(100.0f);
        node->setHeight(50.0f);
        node->setTabIndex(1);
        document.appendChild(*node);
    }

    document.update();
    _ScriptClick(window, { 50.0f, 25.0f });
    ASSERT_TRUE(a.isFocused() == true);

    auto lastFocus = static_cast<Node*>(nullptr);
    auto documentSub = Sub<NodeEvent const&>(document.onEvent, [&](NodeEvent const& event) {
        if (event.is<FocusNodeEvent>()) lastFocus = &event.getNode();
    });
    auto aSub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (event.is<BlurNodeEvent>() && (&event.getNode() == &a)) document.focusNode(&c);
    });

    _ScriptClick(window, { 150.0f, 25.0f });

    ASSERT_TRUE(c.isFocused() == true);
    ASSERT_TRUE(b.isFocused() == false);
    ASSERT_TRUE(lastFocus == &c);
}

/* Leaving the window clears hover and resets the cursor; entering restores
   both. */
TEST(Document, MouseExitAndEnterWindow) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setCursor(Cursor::Pointer);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() == &a) events.push_back(eventName(event));
    });

    _ScriptMove(window, { 50.0f, 25.0f });
    ASSERT_TRUE(a.isHover() == true);
    ASSERT_TRUE(window.getCursor() == Cursor::Pointer);

    window.onEvent.publish(MouseExitWindowEvent(window, { 50.0f, 25.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isHover() == false);
    ASSERT_TRUE(document.isHover() == false);
    ASSERT_TRUE(window.getCursor() == Cursor::Default);

    /* A relayout while outside does not bring hover back. */
    a.setHeight(60.0f);
    document.update();
    ASSERT_TRUE(a.isHover() == false);

    window.onEvent.publish(MouseEnterWindowEvent(window, { 50.0f, 25.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isHover() == true);
    ASSERT_TRUE(window.getCursor() == Cursor::Pointer);

    ASSERT_TRUE(events == std::vector<std::string>({ "enter", "move", "exit", "enter", "move" }));
}

/* Nothing is hovered until the first mouse event, even a node at the origin. */
TEST(Document, NothingHoveredBeforeFirstMouseEvent) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();
    ASSERT_TRUE(a.isHover() == false);
    ASSERT_TRUE(document.isHover() == false);

    a.setWidth(120.0f);
    document.update();
    ASSERT_TRUE(a.isHover() == false);
}

/* Leaving the window mid-drag keeps the drag; the release outside ends it and
   then clears hover. */
TEST(Document, MouseExitWindowDuringDrag) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() == &a) events.push_back(eventName(event));
    });

    _ScriptMove(window, { 10.0f, 10.0f });
    events.clear();

    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 10.0f, 10.0f }, KeyModifiers{}));
    _ScriptMove(window, { 50.0f, 10.0f });
    window.onEvent.publish(MouseExitWindowEvent(window, { 700.0f, 10.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isActive() == true);

    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 700.0f, 10.0f }, KeyModifiers{}));

    ASSERT_TRUE(events == std::vector<std::string>({ "down", "begindrag", "drag", "drag", "up", "enddrag", "exit" }));
    ASSERT_TRUE(a.isActive() == false);
    ASSERT_TRUE(a.isHover() == false);
}

/* Removing the hovered node clears its hover; ancestors still under the
   pointer stay hovered and get no exit. */
TEST(Document, RemovingHoveredNodeKeepsAncestorHover) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(100.0f);
    document.appendChild(parent);

    auto child = Node();
    child.setWidth(100.0f);
    child.setHeight(50.0f);
    parent.appendChild(child);

    document.update();

    _ScriptMove(window, { 50.0f, 25.0f });
    ASSERT_TRUE(child.isHover() == true);

    auto parentEvents = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(parent.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() == &parent) parentEvents.push_back(eventName(event));
    });

    parent.removeChild(child);
    document.update();

    ASSERT_TRUE(child.isHover() == false);
    ASSERT_TRUE(parent.isHover() == true);
    ASSERT_TRUE(document.isHover() == true);
    ASSERT_TRUE(parentEvents == std::vector<std::string>({ "move" }));

    /* The parent is now the press target. */
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));
    ASSERT_TRUE(parent.isActive() == true);
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));
}

/* Removing the pressed node mid-press: the release goes to the document, no
   click fires, and the active chain is cleared. */
TEST(Document, RemovingActiveNodeMidPress) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto upTargets = std::vector<Node*>();
    auto clicks = 0;
    auto sub = Sub<NodeEvent const&>(document.onEvent, [&](NodeEvent const& event) {
        if (event.is<MouseUpNodeEvent>()) upTargets.push_back(&event.getNode());
        if (event.is<MouseClickNodeEvent>()) clicks += 1;
    });

    _ScriptMove(window, { 50.0f, 25.0f });
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isActive() == true);

    document.removeChild(a);
    ASSERT_TRUE(a.isActive() == false);

    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));

    ASSERT_TRUE(upTargets == std::vector<Node*>({ &document }));
    ASSERT_TRUE(clicks == 0);
    ASSERT_TRUE(document.isActive() == false);
}

/* Removing the dragged node mid-drag stops drag events without ending the
   gesture on a removed node. */
TEST(Document, RemovingDragNodeMidDrag) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() == &a) events.push_back(eventName(event));
    });

    _ScriptMove(window, { 10.0f, 10.0f });
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 10.0f, 10.0f }, KeyModifiers{}));
    _ScriptMove(window, { 50.0f, 10.0f });
    events.clear();

    document.removeChild(a);

    _ScriptMove(window, { 70.0f, 10.0f });
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 70.0f, 10.0f }, KeyModifiers{}));

    ASSERT_TRUE(events.empty());
    ASSERT_TRUE(document.isActive() == false);
}

/* Removing the focused node clears focus-within on its former ancestors. */
TEST(Document, RemovingFocusedNodeClearsFocusWithin) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto parent = Node();
    parent.setWidth(200.0f);
    parent.setHeight(100.0f);
    document.appendChild(parent);

    auto focusable = Node();
    focusable.setWidth(100.0f);
    focusable.setHeight(50.0f);
    focusable.setTabIndex(1);
    parent.appendChild(focusable);

    document.update();

    _ScriptClick(window, { 50.0f, 25.0f });
    ASSERT_TRUE(focusable.isFocused() == true);
    ASSERT_TRUE(parent.isFocusedWithin() == true);

    parent.removeChild(focusable);

    ASSERT_TRUE(focusable.isFocused() == false);
    ASSERT_TRUE(focusable.isFocusedWithin() == false);
    ASSERT_TRUE(parent.isFocusedWithin() == false);
    ASSERT_TRUE(document.isFocusedWithin() == false);

    /* Re-attaching does not bring focus back; focusing again works. */
    parent.appendChild(focusable);
    document.update();
    ASSERT_TRUE(focusable.isFocused() == false);

    document.focusNode(&focusable);
    ASSERT_TRUE(focusable.isFocused() == true);
    ASSERT_TRUE(parent.isFocusedWithin() == true);
}

/* Every press gets exactly one release on the same target: the document when
   the press hit nothing, otherwise the pressed node wherever the release
   lands, for either button. */
TEST(Document, MouseUpPairsWithMouseDownTarget) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    document.appendChild(b);

    document.update();

    auto downTargets = std::vector<Node*>();
    auto upTargets = std::vector<Node*>();
    auto sub = Sub<NodeEvent const&>(document.onEvent, [&](NodeEvent const& event) {
        if (event.is<MouseDownNodeEvent>()) downTargets.push_back(&event.getNode());
        if (event.is<MouseUpNodeEvent>()) upTargets.push_back(&event.getNode());
    });

    /* Press on empty space, release over a. */
    _ScriptMove(window, { 500.0f, 400.0f });
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 500.0f, 400.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));

    /* Press on a, release over b. */
    _ScriptMove(window, { 50.0f, 25.0f });
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 150.0f, 25.0f }, KeyModifiers{}));

    /* Same with the right button. */
    _ScriptMove(window, { 50.0f, 25.0f });
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::RightButton, { 50.0f, 25.0f }, KeyModifiers{}));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::RightButton, { 150.0f, 25.0f }, KeyModifiers{}));

    ASSERT_TRUE(downTargets == std::vector<Node*>({ &document, &a, &a }));
    ASSERT_TRUE(upTargets == std::vector<Node*>({ &document, &a, &a }));
}

/* A layout change that moves a different node under a still pointer moves
   hover to it: exit on the old node, enter and exactly one move on the new. */
TEST(Document, LayoutChangeUnderStillPointerRetargetsHover) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    document.appendChild(b);

    document.update();

    _ScriptMove(window, { 150.0f, 25.0f });
    ASSERT_TRUE(b.isHover() == true);

    auto aEvents = std::vector<std::string>();
    auto bEvents = std::vector<std::string>();
    auto aSub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() == &a) aEvents.push_back(eventName(event));
    });
    auto bSub = Sub<NodeEvent const&>(b.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() == &b) bEvents.push_back(eventName(event));
    });

    auto spacer = Node();
    spacer.setWidth(100.0f);
    spacer.setHeight(50.0f);
    document.insertChild(spacer, 0);
    document.update();

    ASSERT_TRUE(a.isHover() == true);
    ASSERT_TRUE(b.isHover() == false);
    ASSERT_TRUE(aEvents == std::vector<std::string>({ "enter", "move" }));
    ASSERT_TRUE(bEvents == std::vector<std::string>({ "exit" }));
}

/* A relayout that leaves the same node under a still pointer dispatches
   nothing. */
TEST(Document, RelayoutWithoutHoverChangeDispatchesNothing) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();
    _ScriptMove(window, { 50.0f, 25.0f });

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(a.onEvent, [&](NodeEvent const& event) {
        events.push_back(eventName(event));
    });

    a.setHeight(60.0f);
    document.update();
    a.setWidth(120.0f);
    document.update();

    ASSERT_TRUE(events.empty());
    ASSERT_TRUE(a.isHover() == true);
}

/* The cursor follows hover changes caused by layout and cursor changes on
   the hovered node, without any mouse movement. */
TEST(Document, CursorFollowsLayoutAndCursorChanges) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setCursor(Cursor::Pointer);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    document.appendChild(b);

    document.update();
    _ScriptMove(window, { 150.0f, 25.0f });
    ASSERT_TRUE(window.getCursor() == Cursor::Default);

    auto spacer = Node();
    spacer.setWidth(100.0f);
    spacer.setHeight(50.0f);
    document.insertChild(spacer, 0);
    document.update();
    ASSERT_TRUE(window.getCursor() == Cursor::Pointer);

    a.setCursor(Cursor::Text);
    document.update();
    ASSERT_TRUE(window.getCursor() == Cursor::Text);
}

/* A press dispatches the mouse down before the blur and focus it causes. */
TEST(Document, PressDispatchesDownBeforeFocusChange) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setTabIndex(1);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    b.setTabIndex(1);
    document.appendChild(b);

    document.update();
    _ScriptClick(window, { 50.0f, 25.0f });

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(document.onEvent, [&](NodeEvent const& event) {
        auto const name = _EventNameWithFocus(event);
        auto const target = (&event.getNode() == &a) ? "a" : (&event.getNode() == &b) ? "b" : "?";
        if ((name != "enter") && (name != "exit") && (name != "move")) {
            events.push_back(name + ":" + target);
        }
    });

    _ScriptClick(window, { 150.0f, 25.0f });

    ASSERT_TRUE(events == std::vector<std::string>({ "down:b", "blur:a", "focus:b", "up:b", "click:b" }));
}

/* Focus walks up to the nearest focusable ancestor, for clicks and for
   focusNode() alike; with none, focus is cleared. */
TEST(Document, FocusWalksUpToFocusableAncestor) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto focusable = Node();
    focusable.setWidth(200.0f);
    focusable.setHeight(100.0f);
    focusable.setTabIndex(1);
    document.appendChild(focusable);

    auto child = Node();
    child.setWidth(100.0f);
    child.setHeight(50.0f);
    focusable.appendChild(child);

    auto plain = Node();
    plain.setWidth(100.0f);
    plain.setHeight(50.0f);
    document.appendChild(plain);

    document.update();

    _ScriptClick(window, { 50.0f, 25.0f });
    ASSERT_TRUE(focusable.isFocused() == true);
    ASSERT_TRUE(child.isFocused() == false);

    document.focusNode(nullptr);
    ASSERT_TRUE(focusable.isFocused() == false);

    document.focusNode(&child);
    ASSERT_TRUE(focusable.isFocused() == true);
    ASSERT_TRUE(child.isFocused() == false);

    document.focusNode(&plain);
    ASSERT_TRUE(focusable.isFocused() == false);
    ASSERT_TRUE(plain.isFocused() == false);
    ASSERT_TRUE(document.isFocusedWithin() == false);
}

/* Resize and DPI change window events relayout without an explicit update(). */
TEST(Document, ResizeAndDPIChangeRelayout) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(NodeValue(PercentValue{ 50.0f }));
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();
    ASSERT_TRUE(a.getComputedBorderRect().width == 320.0f);

    window.setSize({ 800.0f, 480.0f });
    window.onEvent.publish(ResizeWindowEvent(window));
    ASSERT_TRUE(a.getComputedBorderRect().width == 400.0f);

    window.setSize({ 1000.0f, 480.0f });
    window.onEvent.publish(DPIChangeWindowEvent(window));
    ASSERT_TRUE(a.getComputedBorderRect().width == 500.0f);
}

/* Wheel deltas arrive in window points and map into document units, both for
   scrolling and for the MouseWheelNodeEvent payload. */
TEST(Document, WheelDeltaMapsUnderDocumentScale) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setScale(window.getScale() * 2.0f);

    auto container = Node();
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(container);

    auto tall = Node();
    tall.setWidth(100.0f);
    tall.setHeight(300.0f);
    container.appendChild(tall);

    document.update();

    auto const factor = (window.getScale() / document.getScale());
    auto wheel = Vec2();
    auto sub = Sub<NodeEvent const&>(container.onEvent, [&](NodeEvent const& event) {
        if (auto e = event.as<MouseWheelNodeEvent>()) wheel = e->getWheel();
    });

    _ScriptMove(window, (Vec2(10.0f, 10.0f) / factor));
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -30.0f }, KeyModifiers{}));
    document.update();

    ASSERT_TRUE(wheel == Vec2(0.0f, (-30.0f * factor)));
    ASSERT_TRUE(tall.getComputedBorderRect().y == (-30.0f * factor));
}

/* Like the web's :active, only the primary button activates; other buttons
   still pair their down and up on the pressed node. */
TEST(Document, OnlyLeftButtonActivates) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    document.appendChild(a);

    document.update();
    _ScriptMove(window, { 50.0f, 25.0f });

    auto upTargets = std::vector<Node*>();
    auto sub = Sub<NodeEvent const&>(document.onEvent, [&](NodeEvent const& event) {
        if (event.is<MouseUpNodeEvent>()) upTargets.push_back(&event.getNode());
    });

    for (auto const mouse : { Mouse::RightButton, Mouse::MiddleButton }) {
        window.onEvent.publish(MouseDownWindowEvent(window, mouse, { 50.0f, 25.0f }, KeyModifiers{}));
        ASSERT_TRUE(a.isActive() == false);
        ASSERT_TRUE(document.isActive() == false);
        window.onEvent.publish(MouseUpWindowEvent(window, mouse, { 50.0f, 25.0f }, KeyModifiers{}));
    }

    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));
    ASSERT_TRUE(a.isActive() == true);
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, { 50.0f, 25.0f }, KeyModifiers{}));

    ASSERT_TRUE(upTargets == std::vector<Node*>({ &a, &a, &a }));
}

/* Like the web, the left and right buttons move focus; other buttons do not. */
TEST(Document, LeftAndRightButtonsFocus) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setTabIndex(1);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    b.setTabIndex(1);
    document.appendChild(b);

    document.update();

    auto press = [&](Mouse mouse, Vec2 const& position) {
        _ScriptMove(window, position);
        window.onEvent.publish(MouseDownWindowEvent(window, mouse, position, KeyModifiers{}));
        window.onEvent.publish(MouseUpWindowEvent(window, mouse, position, KeyModifiers{}));
    };

    press(Mouse::RightButton, { 50.0f, 25.0f });
    ASSERT_TRUE(a.isFocused() == true);

    press(Mouse::MiddleButton, { 150.0f, 25.0f });
    ASSERT_TRUE(a.isFocused() == true);
    ASSERT_TRUE(b.isFocused() == false);

    press(Mouse::LeftButton, { 150.0f, 25.0f });
    ASSERT_TRUE(b.isFocused() == true);
}

/* Like the web, focusing is the default action of a press: preventDefault()
   on the mouse down keeps focus where it was, while the press, release and
   click still arrive. */
TEST(Document, PreventDefaultOnMouseDownKeepsFocus) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    document.setDirection(NodeDirection::Horizontal);

    auto a = Node();
    a.setWidth(100.0f);
    a.setHeight(50.0f);
    a.setTabIndex(1);
    document.appendChild(a);

    auto b = Node();
    b.setWidth(100.0f);
    b.setHeight(50.0f);
    b.setTabIndex(1);
    document.appendChild(b);

    document.update();
    _ScriptClick(window, { 50.0f, 25.0f });
    ASSERT_TRUE(a.isFocused() == true);

    auto events = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(b.onEvent, [&](NodeEvent const& event) {
        if (&event.getNode() != &b) return;
        events.push_back(_EventNameWithFocus(event));
        if (event.is<MouseDownNodeEvent>()) event.preventDefault();
    });

    _ScriptClick(window, { 150.0f, 25.0f });

    ASSERT_TRUE(a.isFocused() == true);
    ASSERT_TRUE(b.isFocused() == false);
    ASSERT_TRUE(events == std::vector<std::string>({ "enter", "move", "down", "up", "click" }));
}

/* Like the web, preventDefault() on a wheel event cancels the scroll it would
   cause. */
TEST(Document, PreventDefaultOnWheelCancelsScroll) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto container = Node();
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(container);

    auto tall = Node();
    tall.setWidth(100.0f);
    tall.setHeight(300.0f);
    container.appendChild(tall);

    document.update();
    _ScriptMove(window, { 10.0f, 10.0f });

    auto prevent = true;
    auto sub = Sub<NodeEvent const&>(container.onEvent, [&](NodeEvent const& event) {
        if (event.is<MouseWheelNodeEvent>() && prevent) event.preventDefault();
    });

    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -30.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(tall.getComputedBorderRect().y == 0.0f);

    prevent = false;
    window.onEvent.publish(MouseWheelWindowEvent(window, { 0.0f, -30.0f }, KeyModifiers{}));
    document.update();
    ASSERT_TRUE(tall.getComputedBorderRect().y == -30.0f);
}

/* needsUpdate() makes the next update() run a layout pass although nothing in
   the tree was invalidated; a layout pass always schedules a repaint. */
TEST(Document, NeedsUpdateForcesLayoutPass) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    document.appendChild(box);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto paintPasses = 0;
    ON_CALL(sink, beginPaint(_)).WillByDefault(Invoke([&paintPasses](PaintTarget const&) {
        paintPasses += 1;
    }));

    document.update();
    document.render();
    auto const passes = paintPasses;

    document.update();
    document.render();
    ASSERT_TRUE(paintPasses == passes);

    document.needsUpdate();
    document.update();
    document.render();
    ASSERT_TRUE(paintPasses == (passes + 1));
}

/* needsRender() makes the next render() repaint although nothing changed. */
TEST(Document, NeedsRenderForcesRepaint) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    document.appendChild(box);

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto paintPasses = 0;
    ON_CALL(sink, beginPaint(_)).WillByDefault(Invoke([&paintPasses](PaintTarget const&) {
        paintPasses += 1;
    }));

    document.update();
    document.render();
    auto const passes = paintPasses;

    document.render();
    ASSERT_TRUE(paintPasses == passes);

    document.needsRender();
    document.render();
    ASSERT_TRUE(paintPasses == (passes + 1));
}

/* onPaint fires once per render on the node it is subscribed to, with the
   node's border box in paint-target pixels, after the node's background and
   before its children. */
TEST(Document, RenderPublishesOnPaintBeforeChildren) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    box.setBackground(ColorBrush{ .color = COLOR_WHITE });
    document.appendChild(box);

    auto child = Node();
    child.setWidth(20.0f);
    child.setHeight(20.0f);
    child.setBackground(ColorBrush{ .color = COLOR_BLACK });
    box.appendChild(child);

    document.update();

    auto sink = NiceMock<PainterSink>();
    auto scope = SinkScope(sink);

    auto order = std::vector<std::string>();
    ON_CALL(sink, paint(_, _, _)).WillByDefault(Invoke([&order](Shape const&, Brush const& brush, PaintOptions const&) {
        if (auto color = brush.as<ColorBrush>()) {
            order.push_back((color->color == COLOR_WHITE) ? "background" : (color->color == COLOR_BLACK) ? "child" : "custom");
        }
    }));

    auto paints = 0;
    auto paintRect = Vec4();
    auto sub = Sub<Painter const&, Vec4 const&>(box.onPaint, [&](Painter const& painter, Vec4 const& rect) {
        paints += 1;
        paintRect = rect;
        painter.paint(QuadShape{ .rect = rect }, ColorBrush{ .color = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } });
    });

    document.render();

    ASSERT_TRUE(paints == 1);
    ASSERT_TRUE(paintRect == (box.getComputedBorderRect() * document.getScale()));
    ASSERT_TRUE(order == std::vector<std::string>({ "background", "custom", "child" }));
}

/* Changing the tree from a paint listener throws and leaves the tree as it
   was; once rendering is over the tree can change again, also after a
   listener threw out of render(). */
TEST(Document, ChangingTreeWhileRenderingThrows) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto box = Node();
    box.setWidth(100.0f);
    box.setHeight(50.0f);
    document.appendChild(box);

    auto extra = Node();
    auto throws = 0;
    auto fail = false;

    auto sub = Sub<Painter const&, Vec4 const&>(box.onPaint, [&](Painter const&, Vec4 const&) {
        try { document.appendChild(extra); } catch (std::runtime_error const&) { throws += 1; }
        try { document.removeChild(box); } catch (std::runtime_error const&) { throws += 1; }
        if (fail) throw std::logic_error("listener failure");
    });

    document.update();
    document.render();

    ASSERT_TRUE(throws == 2);
    ASSERT_TRUE(extra.getParent() == nullptr);
    ASSERT_TRUE(box.getParent() == &document);

    fail = true;
    document.needsRender();
    ASSERT_THROW(document.render(), std::logic_error);

    document.appendChild(extra);
    ASSERT_TRUE(extra.getParent() == &document);
}

/* The onPaint rect follows scrolling: it is the node's border box after its
   scroll containers are scrolled, in paint-target pixels. Inside an opacity
   layer it is relative to the layer image the listener is painting into. */
TEST(Document, OnPaintRectFollowsScrollAndLayers) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto container = Node();
    container.setMarginLeft(50.0f);
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setOverflowY(NodeOverflow::Scroll);
    container.setDirection(NodeDirection::Vertical);
    document.appendChild(container);

    auto above = Node();
    above.setWidth(100.0f);
    above.setHeight(40.0f);
    container.appendChild(above);

    auto target = Node();
    target.setWidth(100.0f);
    target.setHeight(40.0f);
    container.appendChild(target);

    auto filler = Node();
    filler.setWidth(100.0f);
    filler.setHeight(300.0f);
    container.appendChild(filler);

    auto paintRect = Vec4();
    auto sub = Sub<Painter const&, Vec4 const&>(target.onPaint, [&](Painter const&, Vec4 const& rect) {
        paintRect = rect;
    });

    auto const scale = document.getScale();

    document.update();
    document.render();
    ASSERT_TRUE(paintRect == (Vec4{ 50.0f, 40.0f, 100.0f, 40.0f } * scale));

    document.scrollNode(target, { 0.0f, -30.0f });
    document.update();
    document.render();
    ASSERT_TRUE(paintRect == (Vec4{ 50.0f, 10.0f, 100.0f, 40.0f } * scale));

    /* With the container in a layer the listener paints into the layer image,
       whose origin is the container's border box. */
    container.setOpacity(0.5f);
    document.update();
    document.render();
    ASSERT_TRUE(paintRect == (Vec4{ 0.0f, 10.0f, 100.0f, 40.0f } * scale));
}

/* convertPointFromDocument() maps a document point into a node's local
   coordinates (origin at its border box, scroll applied), so a listener on
   any node of the path can localize a bubbling mouse event;
   convertPointToDocument() is its inverse. */
TEST(Document, ConvertPointBetweenDocumentAndNode) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto container = Node();
    container.setMarginLeft(50.0f);
    container.setMarginTop(20.0f);
    container.setWidth(100.0f);
    container.setHeight(100.0f);
    container.setOverflowY(NodeOverflow::Scroll);
    container.setDirection(NodeDirection::Vertical);
    document.appendChild(container);

    auto above = Node();
    above.setWidth(100.0f);
    above.setHeight(40.0f);
    container.appendChild(above);

    auto target = Node();
    target.setWidth(100.0f);
    target.setHeight(300.0f);
    container.appendChild(target);

    document.update();
    document.scrollNode(target, { 0.0f, -30.0f });
    document.update();

    /* target's border box starts at (50, 20 + 40 - 30) in the document. */
    ASSERT_TRUE(target.convertPointFromDocument({ 60.0f, 35.0f }) == Vec2(10.0f, 5.0f));
    ASSERT_TRUE(target.convertPointToDocument({ 10.0f, 5.0f }) == Vec2(60.0f, 35.0f));
    ASSERT_TRUE(container.convertPointFromDocument({ 60.0f, 35.0f }) == Vec2(10.0f, 15.0f));

    auto containerLocal = Vec2();
    auto targetLocal = Vec2();
    auto sub = Sub<NodeEvent const&>(container.onEvent, [&](NodeEvent const& event) {
        if (auto e = event.as<MouseMoveNodeEvent>()) {
            containerLocal = container.convertPointFromDocument(e->getPosition());
            targetLocal = e->getNode().convertPointFromDocument(e->getPosition());
        }
    });

    _ScriptMove(window, { 60.0f, 35.0f });

    ASSERT_TRUE(containerLocal == Vec2(10.0f, 15.0f));
    ASSERT_TRUE(targetLocal == Vec2(10.0f, 5.0f));
}

/* The document keeps the window's input area on the focused editable's
   caret (in window points), and clears it when focus leaves, which turns the
   OS text system off. */
TEST(Document, InputAreaFollowsFocusedEditable) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, "hello");
    document.update();
    document.render();

    auto const area = window.getInputArea();
    ASSERT_TRUE(area.has_value());

    auto const factor = (document.getScale() / window.getScale());
    auto const rect = (box.getComputedBorderRect() * factor);
    ASSERT_TRUE(area->x >= rect.x && area->getMaxX() <= rect.getMaxX());
    ASSERT_TRUE(area->y >= rect.y && area->getMaxY() <= rect.getMaxY());
    ASSERT_TRUE(area->height > 0.0f);

    /* the caret moves right as text is typed */
    _ScriptType(window, "!");
    document.render();
    ASSERT_TRUE(window.getInputArea().has_value());
    ASSERT_TRUE(window.getInputArea()->x > area->x);

    document.focusNode(nullptr);
    document.update();
    ASSERT_TRUE(window.getInputArea().has_value() == false);
}

/* A focused node with an input area takes text input although it is not
   editable: the window's input area is its local caret rect in window
   points, and typed text reaches it as BeforeInputNodeEvent. */
TEST(Document, NodeInputAreaTakesTextInput) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto spacer = Node();
    spacer.setHeight(30.0f);
    document.appendChild(spacer);

    auto canvas = Node();
    canvas.setWidth(200.0f);
    canvas.setHeight(100.0f);
    canvas.setTabIndex(1);
    canvas.setInputArea(Vec4{ 10.0f, 5.0f, 1.0f, 12.0f });
    document.appendChild(canvas);

    auto texts = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>(canvas.onEvent, [&](NodeEvent const& event) {
        if (auto e = event.as<BeforeInputNodeEvent>()) texts.push_back(e->getText());
    });

    document.update();
    ASSERT_TRUE(window.getInputArea().has_value() == false); /* not focused yet */

    document.focusNode(&canvas);
    document.update();

    auto const factor = (document.getScale() / window.getScale());
    auto const origin = canvas.convertPointToDocument({ 10.0f, 5.0f });
    ASSERT_TRUE(window.getInputArea().has_value());
    ASSERT_TRUE(*window.getInputArea() == Vec4(origin.x * factor, origin.y * factor, 1.0f * factor, 12.0f * factor));

    window.onEvent.publish(InputWindowEvent(window, "x"));
    window.onEvent.publish(InputWindowEvent(window, "日本語"));
    ASSERT_TRUE(texts == std::vector<std::string>({ "x", "日本語" }));

    /* clearing the node's input area turns text input off */
    canvas.setInputArea(std::nullopt);
    document.update();
    ASSERT_TRUE(window.getInputArea().has_value() == false);

    window.onEvent.publish(InputWindowEvent(window, "y"));
    ASSERT_TRUE(texts.size() == 2);
}

/* The scroll that keeps a long single-line field's caret in view is part of
   the update pass, so the input area is right after update() alone: at the
   end of overflowing text it stays inside the field, without a render. */
TEST(Document, InputAreaFollowsScrolledCaretWithoutRender) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, std::string(120, 'w'));

    _ScriptKey(window, Rocket::Scancode::End);
    document.update();

    auto const area = window.getInputArea();
    ASSERT_TRUE(area.has_value());
    ASSERT_TRUE(text.getComputedBorderRect().width > box.getComputedBorderRect().width); /* the text overflows */

    auto const factor = (document.getScale() / window.getScale());
    auto const rect = (box.getComputedBorderRect() * factor);
    ASSERT_TRUE(area->x >= rect.x && area->getMaxX() <= (rect.getMaxX() + 1.0f));
}

/* A fixed-height multi-line editable that scrolls vertically keeps its caret
   in view: moving the caret past an edge scrolls the box, while scrolling
   the box by hand leaves it alone until the caret is touched again. */
TEST(Document, EditingMultiLineCaretFollowsVerticalScroll) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);
    auto box = Node();
    auto text = Node();
    _ScriptEditableSetup(document, box, text, "a\nb\nc\nd\ne\nf\ng\nh");
    box.setContentMultiLine(true);
    box.setOverflowY(NodeOverflow::Scroll);
    document.update();

    auto const factor = (document.getScale() / window.getScale());
    auto const caretInsideBox = [&]() {
        auto const area = window.getInputArea();
        auto const rect = (box.getComputedBorderRect() * factor);
        return area.has_value() && (area->y >= (rect.y - 1.0f)) && (area->getMaxY() <= (rect.getMaxY() + 1.0f));
    };

    ASSERT_TRUE(text.getComputedBorderRect().height > box.getComputedBorderRect().height); /* the text overflows */
    ASSERT_TRUE(text.getComputedBorderRect().y == 0.0f);
    ASSERT_TRUE(caretInsideBox());

    /* Down to the last line: the box scrolls to its end. */
    for (auto i = 0; i < 7; i++) {
        _ScriptKey(window, Rocket::Scancode::ArrowDown);
    }
    document.update();
    auto const scrolledY = text.getComputedBorderRect().y;
    ASSERT_TRUE(scrolledY == (box.getComputedBorderRect().height - text.getComputedBorderRect().height));
    ASSERT_TRUE(caretInsideBox());

    /* Scrolling by hand is not fought: the caret stays out of view. */
    document.scrollNode(box, { 0.0f, 1000.0f });
    document.update();
    document.update();
    ASSERT_TRUE(text.getComputedBorderRect().y == 0.0f);
    ASSERT_FALSE(caretInsideBox());

    /* Touching the caret brings it back. */
    _ScriptKey(window, Rocket::Scancode::ArrowLeft);
    document.update();
    ASSERT_TRUE(text.getComputedBorderRect().y < 0.0f);
    ASSERT_TRUE(caretInsideBox());

    /* Typing new lines at the end keeps following as the text grows. */
    _ScriptKey(window, Rocket::Scancode::ArrowRight);
    _ScriptKey(window, Rocket::Scancode::Enter);
    _ScriptKey(window, Rocket::Scancode::Enter);
    document.update();
    ASSERT_TRUE(text.getComputedBorderRect().y < scrolledY);
    ASSERT_TRUE(caretInsideBox());

    /* Back up to the first line: the box scrolls to its start. */
    for (auto i = 0; i < 9; i++) {
        _ScriptKey(window, Rocket::Scancode::ArrowUp);
    }
    document.update();
    ASSERT_TRUE(text.getComputedBorderRect().y == 0.0f);
    ASSERT_TRUE(caretInsideBox());
}

/* The repeat flag of a key-down travels from the window event to the node event. */
TEST(Document, KeyRepeatReachesNodeEvent) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    auto document = Document(window);

    auto repeats = std::vector<bool>();
    auto sub = Sub<NodeEvent const&>(document.onEvent, [&](NodeEvent const& event) {
        if (auto e = event.as<KeyDownNodeEvent>()) repeats.push_back(e->isRepeat());
    });

    window.onEvent.publish(KeyDownWindowEvent(window, Rocket::Scancode::KeyA, std::string(Keycode::A), {}));
    window.onEvent.publish(KeyDownWindowEvent(window, Rocket::Scancode::KeyA, std::string(Keycode::A), {}, true));

    ASSERT_TRUE(repeats == std::vector<bool>({ false, true }));
}
