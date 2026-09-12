/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/Node.hpp>
#include <Rocket/Window/Window.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

namespace {

class TestEvent : public NodeEvent {
public:
    TestEvent(Node& node) : NodeEvent(node) {}
};

} /* namespace */

/* A fresh node is detached: no document, no parent, no children, no siblings. */
TEST(Node, FreshNodeDetached) {
    Node node;
    ASSERT_TRUE(node.getDocument() == nullptr);
    ASSERT_TRUE(node.getParent() == nullptr);
    ASSERT_TRUE(node.getFirstChild() == nullptr);
    ASSERT_TRUE(node.getLastChild() == nullptr);
    ASSERT_TRUE(node.getPrevSibling() == nullptr);
    ASSERT_TRUE(node.getNextSibling() == nullptr);
}

/* appendChild links parent, first/last child and sibling pointers. */
TEST(Node, AppendChildLinks) {
    Node parent;
    Node first;
    Node second;
    Node third;
    parent.appendChild(first);
    ASSERT_TRUE(first.getParent() == &parent);
    ASSERT_TRUE(parent.getFirstChild() == &first);
    ASSERT_TRUE(parent.getLastChild() == &first);
    ASSERT_TRUE(first.getPrevSibling() == nullptr);
    ASSERT_TRUE(first.getNextSibling() == nullptr);

    parent.appendChild(second);
    parent.appendChild(third);
    ASSERT_TRUE(parent.getFirstChild() == &first);
    ASSERT_TRUE(parent.getLastChild() == &third);
    ASSERT_TRUE(first.getNextSibling() == &second);
    ASSERT_TRUE(second.getPrevSibling() == &first);
    ASSERT_TRUE(second.getNextSibling() == &third);
    ASSERT_TRUE(third.getPrevSibling() == &second);
    ASSERT_TRUE(third.getNextSibling() == nullptr);
    ASSERT_TRUE(second.getParent() == &parent);
    ASSERT_TRUE(third.getParent() == &parent);
    ASSERT_TRUE(second.getDocument() == nullptr);
}

/* Appending an already-parented node re-parents it. */
TEST(Node, AppendChildReparents) {
    Node oldParent;
    Node newParent;
    Node child;
    Node sibling;
    oldParent.appendChild(child);
    oldParent.appendChild(sibling);
    newParent.appendChild(child);
    ASSERT_TRUE(child.getParent() == &newParent);
    ASSERT_TRUE(newParent.getFirstChild() == &child);
    ASSERT_TRUE(newParent.getLastChild() == &child);
    ASSERT_TRUE(oldParent.getFirstChild() == &sibling);
    ASSERT_TRUE(oldParent.getLastChild() == &sibling);
    ASSERT_TRUE(sibling.getPrevSibling() == nullptr);
    ASSERT_TRUE(sibling.getNextSibling() == nullptr);
}

/* insertChild places a node at an index: first, middle, end, past the end. */
TEST(Node, InsertChildAtIndex) {
    Node parent;
    Node a;
    Node b;
    Node c;
    Node d;

    parent.insertChild(a, 0);
    parent.insertChild(c, 1);
    parent.insertChild(b, 1);
    parent.insertChild(d, 99);
    ASSERT_TRUE(parent.getFirstChild() == &a);
    ASSERT_TRUE(a.getNextSibling() == &b);
    ASSERT_TRUE(b.getNextSibling() == &c);
    ASSERT_TRUE(c.getNextSibling() == &d);
    ASSERT_TRUE(d.getNextSibling() == nullptr);
    ASSERT_TRUE(parent.getLastChild() == &d);
    ASSERT_TRUE(d.getPrevSibling() == &c);
    ASSERT_TRUE(b.getPrevSibling() == &a);
    ASSERT_TRUE(a.getPrevSibling() == nullptr);

    Node e;
    parent.insertChild(e, -5);
    ASSERT_TRUE(parent.getFirstChild() == &e);
    ASSERT_TRUE(e.getNextSibling() == &a);
    ASSERT_TRUE(a.getPrevSibling() == &e);
}

/* insertChild moves a node within its own parent; the index refers to the
   list without it, and a node already at its index stays put. */
TEST(Node, InsertChildMovesWithinParent) {
    Node parent;
    Node a;
    Node b;
    Node c;
    Node d;
    parent.appendChild(a);
    parent.appendChild(b);
    parent.appendChild(c);
    parent.appendChild(d);

    parent.insertChild(a, 1); /* b a c d */
    ASSERT_TRUE(parent.getFirstChild() == &b);
    ASSERT_TRUE(b.getNextSibling() == &a);
    ASSERT_TRUE(a.getNextSibling() == &c);
    ASSERT_TRUE(c.getNextSibling() == &d);

    parent.insertChild(d, 0); /* d b a c */
    ASSERT_TRUE(parent.getFirstChild() == &d);
    ASSERT_TRUE(d.getNextSibling() == &b);
    ASSERT_TRUE(parent.getLastChild() == &c);
    ASSERT_TRUE(c.getNextSibling() == nullptr);

    parent.insertChild(b, 3); /* d a c b */
    ASSERT_TRUE(parent.getLastChild() == &b);
    ASSERT_TRUE(b.getPrevSibling() == &c);
    ASSERT_TRUE(d.getNextSibling() == &a);

    /* Already at index 1 (and past-the-end for the last child): no change. */
    parent.insertChild(a, 1);
    parent.insertChild(b, 3);
    parent.insertChild(b, 42);
    ASSERT_TRUE(parent.getFirstChild() == &d);
    ASSERT_TRUE(d.getNextSibling() == &a);
    ASSERT_TRUE(a.getNextSibling() == &c);
    ASSERT_TRUE(c.getNextSibling() == &b);
    ASSERT_TRUE(b.getNextSibling() == nullptr);
}

/* insertChild from another parent unlinks it there first. */
TEST(Node, InsertChildReparents) {
    Node oldParent;
    Node newParent;
    Node child;
    Node sibling;
    Node existing;
    oldParent.appendChild(child);
    oldParent.appendChild(sibling);
    newParent.appendChild(existing);

    newParent.insertChild(child, 0);
    ASSERT_TRUE(child.getParent() == &newParent);
    ASSERT_TRUE(newParent.getFirstChild() == &child);
    ASSERT_TRUE(child.getNextSibling() == &existing);
    ASSERT_TRUE(oldParent.getFirstChild() == &sibling);
    ASSERT_TRUE(oldParent.getLastChild() == &sibling);
    ASSERT_TRUE(sibling.getPrevSibling() == nullptr);
}

/* insertChild(self) and insertChild(ancestor) are no-ops (cycle guard). */
TEST(Node, InsertChildCycleGuard) {
    Node root;
    Node mid;
    Node leaf;
    root.appendChild(mid);
    mid.appendChild(leaf);

    root.insertChild(root, 0);
    ASSERT_TRUE(root.getParent() == nullptr);
    ASSERT_TRUE(root.getFirstChild() == &mid);
    ASSERT_TRUE(root.getLastChild() == &mid);

    leaf.insertChild(root, 0);
    ASSERT_TRUE(root.getParent() == nullptr);
    ASSERT_TRUE(leaf.getFirstChild() == nullptr);
    ASSERT_TRUE(mid.getParent() == &root);
    ASSERT_TRUE(leaf.getParent() == &mid);
}

/* The Yoga child list follows the sibling list: after inserts and moves
   the laid-out x positions of a row reflect the sibling order, not the
   order the nodes were first attached in. */
TEST(Node, InsertChildLayoutOrder) {
    Window window;
    window.setSize({ 640.0f, 480.0f });

    Document document(window);

    Node row;
    row.setDirection(NodeDirection::Horizontal);
    row.setWidth(300.0f);
    row.setHeight(50.0f);
    document.appendChild(row);

    Node a;
    Node b;
    Node c;
    Node d;
    for (auto node : { &a, &b, &c, &d }) {
        node->setWidth(10.0f);
        node->setHeight(10.0f);
    }

    row.appendChild(a);
    row.appendChild(b);
    row.appendChild(c);
    document.update();
    ASSERT_FLOAT_EQ(a.getComputedBorderRect().x, 0.0f);
    ASSERT_FLOAT_EQ(b.getComputedBorderRect().x, 10.0f);
    ASSERT_FLOAT_EQ(c.getComputedBorderRect().x, 20.0f);

    row.insertChild(c, 0); /* c a b */
    document.update();
    ASSERT_FLOAT_EQ(c.getComputedBorderRect().x, 0.0f);
    ASSERT_FLOAT_EQ(a.getComputedBorderRect().x, 10.0f);
    ASSERT_FLOAT_EQ(b.getComputedBorderRect().x, 20.0f);

    row.insertChild(a, 2); /* c b a */
    document.update();
    ASSERT_FLOAT_EQ(c.getComputedBorderRect().x, 0.0f);
    ASSERT_FLOAT_EQ(b.getComputedBorderRect().x, 10.0f);
    ASSERT_FLOAT_EQ(a.getComputedBorderRect().x, 20.0f);

    row.insertChild(d, 1); /* c d b a */
    document.update();
    ASSERT_FLOAT_EQ(c.getComputedBorderRect().x, 0.0f);
    ASSERT_FLOAT_EQ(d.getComputedBorderRect().x, 10.0f);
    ASSERT_FLOAT_EQ(b.getComputedBorderRect().x, 20.0f);
    ASSERT_FLOAT_EQ(a.getComputedBorderRect().x, 30.0f);

    row.insertChild(d, 1); /* no-op */
    document.update();
    ASSERT_FLOAT_EQ(d.getComputedBorderRect().x, 10.0f);
    ASSERT_FLOAT_EQ(b.getComputedBorderRect().x, 20.0f);

    row.removeChild(c);
    row.insertChild(c, 99); /* d b a c */
    document.update();
    ASSERT_FLOAT_EQ(d.getComputedBorderRect().x, 0.0f);
    ASSERT_FLOAT_EQ(b.getComputedBorderRect().x, 10.0f);
    ASSERT_FLOAT_EQ(a.getComputedBorderRect().x, 20.0f);
    ASSERT_FLOAT_EQ(c.getComputedBorderRect().x, 30.0f);
}

/* insertChild attaches the inserted subtree to the parent's document, a
   move within the parent keeps it attached, and the no-op path (already at
   the index) does not disturb focus. */
TEST(Node, InsertChildAttachment) {
    Window window;
    window.setSize({ 640.0f, 480.0f });

    Document document(window);

    Node parent;
    Node sibling;
    Node child;
    Node grandchild;
    document.appendChild(parent);
    parent.appendChild(sibling);
    child.appendChild(grandchild);
    child.setTabIndex(1);
    ASSERT_TRUE(child.getDocument() == nullptr);
    ASSERT_TRUE(grandchild.getDocument() == nullptr);

    parent.insertChild(child, 0);
    ASSERT_TRUE(child.getDocument() == &document);
    ASSERT_TRUE(grandchild.getDocument() == &document);
    ASSERT_TRUE(parent.getFirstChild() == &child);

    document.focusNode(&child);
    ASSERT_TRUE(child.isFocused());

    parent.insertChild(child, 0); /* already there: nothing happens */
    ASSERT_TRUE(child.isFocused());
    ASSERT_TRUE(child.getDocument() == &document);

    parent.insertChild(child, 1); /* real move: stays attached */
    ASSERT_TRUE(parent.getFirstChild() == &sibling);
    ASSERT_TRUE(sibling.getNextSibling() == &child);
    ASSERT_TRUE(child.getDocument() == &document);
    ASSERT_TRUE(grandchild.getDocument() == &document);

    Node other;
    other.insertChild(child, 0); /* leaves the document */
    ASSERT_TRUE(child.getDocument() == nullptr);
    ASSERT_TRUE(grandchild.getDocument() == nullptr);
    ASSERT_TRUE(child.isFocused() == false);
}

/* appendChild(self) and appendChild(ancestor) are no-ops (cycle guard). */
TEST(Node, AppendChildCycleGuard) {
    Node root;
    Node mid;
    Node leaf;
    root.appendChild(mid);
    mid.appendChild(leaf);

    root.appendChild(root);
    ASSERT_TRUE(root.getParent() == nullptr);
    ASSERT_TRUE(root.getFirstChild() == &mid);
    ASSERT_TRUE(root.getLastChild() == &mid);

    leaf.appendChild(root);
    ASSERT_TRUE(root.getParent() == nullptr);
    ASSERT_TRUE(leaf.getFirstChild() == nullptr);
    ASSERT_TRUE(mid.getParent() == &root);
    ASSERT_TRUE(leaf.getParent() == &mid);
}

/* setParent is equivalent to parent.appendChild(*this). */
TEST(Node, SetParent) {
    Node parent;
    Node child;
    child.setParent(parent);
    ASSERT_TRUE(child.getParent() == &parent);
    ASSERT_TRUE(parent.getFirstChild() == &child);
    ASSERT_TRUE(parent.getLastChild() == &child);
}

/* removeChild unlinks a middle child and repairs sibling links. */
TEST(Node, RemoveChild) {
    Node parent;
    Node first;
    Node second;
    Node third;
    parent.appendChild(first);
    parent.appendChild(second);
    parent.appendChild(third);

    parent.removeChild(second);
    ASSERT_TRUE(second.getParent() == nullptr);
    ASSERT_TRUE(second.getPrevSibling() == nullptr);
    ASSERT_TRUE(second.getNextSibling() == nullptr);
    ASSERT_TRUE(first.getNextSibling() == &third);
    ASSERT_TRUE(third.getPrevSibling() == &first);
    ASSERT_TRUE(parent.getFirstChild() == &first);
    ASSERT_TRUE(parent.getLastChild() == &third);

    /* removeChild on a non-child is a no-op. */
    Node stranger;
    parent.removeChild(stranger);
    ASSERT_TRUE(parent.getFirstChild() == &first);
    ASSERT_TRUE(parent.getLastChild() == &third);
}

/* removeFromParent detaches the node; no-op when already a root. */
TEST(Node, RemoveFromParent) {
    Node parent;
    Node child;
    parent.appendChild(child);
    child.removeFromParent();
    ASSERT_TRUE(child.getParent() == nullptr);
    ASSERT_TRUE(parent.getFirstChild() == nullptr);
    ASSERT_TRUE(parent.getLastChild() == nullptr);

    child.removeFromParent();
    ASSERT_TRUE(child.getParent() == nullptr);
}

/* removeAllChildren detaches every direct child. */
TEST(Node, RemoveAllChildren) {
    Node parent;
    Node first;
    Node second;
    Node third;
    parent.appendChild(first);
    parent.appendChild(second);
    parent.appendChild(third);
    parent.removeAllChildren();
    ASSERT_TRUE(parent.getFirstChild() == nullptr);
    ASSERT_TRUE(parent.getLastChild() == nullptr);
    ASSERT_TRUE(first.getParent() == nullptr);
    ASSERT_TRUE(second.getParent() == nullptr);
    ASSERT_TRUE(third.getParent() == nullptr);
    ASSERT_TRUE(first.getNextSibling() == nullptr);
    ASSERT_TRUE(second.getPrevSibling() == nullptr);
}

/* containsNode covers self, descendants and non-descendants. */
TEST(Node, ContainsNode) {
    Node root;
    Node mid;
    Node leaf;
    Node other;
    root.appendChild(mid);
    mid.appendChild(leaf);
    ASSERT_TRUE(root.containsNode(root));
    ASSERT_TRUE(root.containsNode(mid));
    ASSERT_TRUE(root.containsNode(leaf));
    ASSERT_TRUE(mid.containsNode(leaf));
    ASSERT_TRUE(!mid.containsNode(root));
    ASSERT_TRUE(!leaf.containsNode(mid));
    ASSERT_TRUE(!root.containsNode(other));
}

/* getPath returns the root-first ancestor chain ending at the node. */
TEST(Node, GetPath) {
    Node root;
    Node mid;
    Node leaf;
    root.appendChild(mid);
    mid.appendChild(leaf);

    std::vector<Node*> path;
    leaf.getPath(path);
    ASSERT_TRUE(path.size() == 3);
    ASSERT_TRUE(path[0] == &root);
    ASSERT_TRUE(path[1] == &mid);
    ASSERT_TRUE(path[2] == &leaf);

    /* getPath clears any previous contents first. */
    root.getPath(path);
    ASSERT_TRUE(path.size() == 1);
    ASSERT_TRUE(path[0] == &root);
}

/* Style properties default to std::nullopt. */
TEST(Node, StyleDefaults) {
    Node node;
    ASSERT_TRUE(!node.getDisplay().has_value());
    ASSERT_TRUE(!node.getDirection().has_value());
    ASSERT_TRUE(!node.getAlignment().has_value());
    ASSERT_TRUE(!node.getJustify().has_value());
    ASSERT_TRUE(!node.getPosition().has_value());
    ASSERT_TRUE(!node.getWidth().has_value());
    ASSERT_TRUE(!node.getHeight().has_value());
    ASSERT_TRUE(!node.getMinWidth().has_value());
    ASSERT_TRUE(!node.getMaxHeight().has_value());
    ASSERT_TRUE(!node.getTop().has_value());
    ASSERT_TRUE(!node.getPaddingLeft().has_value());
    ASSERT_TRUE(!node.getMarginBottom().has_value());
    ASSERT_TRUE(!node.getGapX().has_value());
    ASSERT_TRUE(!node.getBorderTopWidth().has_value());
    ASSERT_TRUE(!node.getVisible().has_value());
    ASSERT_TRUE(!node.getZIndex().has_value());
    ASSERT_TRUE(!node.getOffset().has_value());
    ASSERT_TRUE(!node.getOpacity().has_value());
    ASSERT_TRUE(!node.getContent().has_value());
}

/* Spacing shorthands (padding/margin/gap) round-trip and do not write through
   to the per-edge/per-axis storage. */
TEST(Node, SpacingShorthandRoundTrip) {
    Node node;
    ASSERT_TRUE(!node.getPadding().has_value());
    ASSERT_TRUE(!node.getMargin().has_value());
    ASSERT_TRUE(!node.getGap().has_value());

    node.setPadding(10.0f);
    ASSERT_TRUE(node.getPadding().has_value());
    ASSERT_TRUE(node.getPadding()->is<PixelValue>());
    ASSERT_TRUE(node.getPadding()->as<PixelValue>()->value == 10.0f);
    ASSERT_TRUE(!node.getPaddingTop().has_value());
    ASSERT_TRUE(!node.getPaddingLeft().has_value());
    ASSERT_TRUE(!node.getPaddingRight().has_value());
    ASSERT_TRUE(!node.getPaddingBottom().has_value());

    node.setMargin(5.0f);
    ASSERT_TRUE(node.getMargin().has_value());
    ASSERT_TRUE(node.getMargin()->as<PixelValue>()->value == 5.0f);
    ASSERT_TRUE(!node.getMarginTop().has_value());
    ASSERT_TRUE(!node.getMarginLeft().has_value());

    node.setGap(8.0f);
    ASSERT_TRUE(node.getGap().has_value());
    ASSERT_TRUE(node.getGap()->as<PixelValue>()->value == 8.0f);
    ASSERT_TRUE(!node.getGapX().has_value());
    ASSERT_TRUE(!node.getGapY().has_value());

    ASSERT_TRUE(!node.getBorderWidth().has_value());
    node.setBorderWidth(2.0f);
    ASSERT_TRUE(node.getBorderWidth().has_value());
    ASSERT_TRUE(node.getBorderWidth().value() == 2.0f);
    ASSERT_TRUE(!node.getBorderTopWidth().has_value());
    ASSERT_TRUE(!node.getBorderLeftWidth().has_value());
    ASSERT_TRUE(!node.getBorderRightWidth().has_value());
    ASSERT_TRUE(!node.getBorderBottomWidth().has_value());

    /* Border radius: fresh node has all five unset; the uniform radius does
       not write through to the per-corner storage. */
    ASSERT_TRUE(!node.getBorderRadius().has_value());
    ASSERT_TRUE(!node.getBorderTopLeftRadius().has_value());
    ASSERT_TRUE(!node.getBorderTopRightRadius().has_value());
    ASSERT_TRUE(!node.getBorderBottomLeftRadius().has_value());
    ASSERT_TRUE(!node.getBorderBottomRightRadius().has_value());
    node.setBorderRadius(6.0f);
    ASSERT_TRUE(node.getBorderRadius().has_value());
    ASSERT_TRUE(node.getBorderRadius().value() == 6.0f);
    ASSERT_TRUE(!node.getBorderTopLeftRadius().has_value());
    ASSERT_TRUE(!node.getBorderTopRightRadius().has_value());
    ASSERT_TRUE(!node.getBorderBottomLeftRadius().has_value());
    ASSERT_TRUE(!node.getBorderBottomRightRadius().has_value());

    /* A per-corner radius round-trips independently of the uniform one. */
    node.setBorderTopLeftRadius(12.0f);
    ASSERT_TRUE(node.getBorderTopLeftRadius().has_value());
    ASSERT_TRUE(node.getBorderTopLeftRadius().value() == 12.0f);
    ASSERT_TRUE(node.getBorderRadius().value() == 6.0f);
    ASSERT_TRUE(!node.getBorderTopRightRadius().has_value());
    ASSERT_TRUE(!node.getBorderBottomLeftRadius().has_value());
    ASSERT_TRUE(!node.getBorderBottomRightRadius().has_value());

    /* std::nullopt clears the shorthands. */
    node.setPadding(std::nullopt);
    node.setMargin(std::nullopt);
    node.setGap(std::nullopt);
    node.setBorderWidth(std::nullopt);
    node.setBorderRadius(std::nullopt);
    node.setBorderTopLeftRadius(std::nullopt);
    ASSERT_TRUE(!node.getPadding().has_value());
    ASSERT_TRUE(!node.getMargin().has_value());
    ASSERT_TRUE(!node.getGap().has_value());
    ASSERT_TRUE(!node.getBorderWidth().has_value());
    ASSERT_TRUE(!node.getBorderRadius().has_value());
    ASSERT_TRUE(!node.getBorderTopLeftRadius().has_value());
}

/* setWidth/getWidth round-trip with PixelValue and PercentValue. */
TEST(Node, WidthRoundTrip) {
    Node node;
    node.setWidth(NodeValue{PixelValue{120.0f}});
    ASSERT_TRUE(node.getWidth().has_value());
    ASSERT_TRUE(node.getWidth()->is<PixelValue>());
    ASSERT_TRUE(node.getWidth()->as<PixelValue>()->value == 120.0f);

    node.setWidth(NodeValue{PercentValue{50.0f}});
    ASSERT_TRUE(node.getWidth().has_value());
    ASSERT_TRUE(node.getWidth()->is<PercentValue>());
    ASSERT_TRUE(node.getWidth()->as<PercentValue>()->value == 50.0f);

    /* A bare float constructs a PixelValue. */
    node.setHeight(64.0f);
    ASSERT_TRUE(node.getHeight().has_value());
    ASSERT_TRUE(node.getHeight()->is<PixelValue>());
    ASSERT_TRUE(node.getHeight()->as<PixelValue>()->value == 64.0f);

    /* std::nullopt clears the value. */
    node.setWidth(std::nullopt);
    ASSERT_TRUE(!node.getWidth().has_value());
}

/* Scalar and flag properties round-trip. */
TEST(Node, ScalarAndFlagRoundTrip) {
    Node node;
    node.setOpacity(0.5f);
    ASSERT_TRUE(node.getOpacity().has_value());
    ASSERT_TRUE(*node.getOpacity() == 0.5f);
    node.setOpacity(std::nullopt);
    ASSERT_TRUE(!node.getOpacity().has_value());

    node.setVisible(false);
    ASSERT_TRUE(node.getVisible().has_value());
    ASSERT_TRUE(*node.getVisible() == false);
    node.setVisible(std::nullopt);
    ASSERT_TRUE(!node.getVisible().has_value());

    ASSERT_TRUE(!node.getFlex());
    node.setFlex(true);
    ASSERT_TRUE(node.getFlex());
    node.setFlex(false);
    ASSERT_TRUE(!node.getFlex());

    node.setDisplay(NodeDisplay::Text);
    ASSERT_TRUE(node.getDisplay().has_value());
    ASSERT_TRUE(*node.getDisplay() == NodeDisplay::Text);
    node.setDisplay(std::nullopt);
    ASSERT_TRUE(!node.getDisplay().has_value());
}

/* A nested text node's style must win over its ancestor's.
 *
 * Text nodes are flattened into one string plus a list of styled ranges. An
 * ancestor's range spans its entire subtree, so it has to be applied before its
 * descendants' or it overwrites them and nested styling silently collapses to
 * the outermost style — no crash, no warning, just the wrong glyphs. Measured
 * through layout width because that is what the collapse actually changes.
 *
 * The window is never made visible, so nothing flashes on screen.
 */
TEST(Node, NestedTextStyleOverridesAncestor) {
    Window window;
    window.setSize({ 640.0f, 480.0f });

    Document document(window);

    Node outer;
    outer.setDisplay(NodeDisplay::Text);
    outer.setFontSize(32.0f);
    outer.setContent("Count: ");

    Node inner;
    inner.setDisplay(NodeDisplay::Text);
    inner.setContent("8");

    outer.appendChild(inner);
    document.appendChild(outer);

    document.update();

    auto const normalWidth = outer.getComputedBorderRect().width;

    inner.setFontWeight(FontWeight::Bold);
    document.update();

    ASSERT_GT(outer.getComputedBorderRect().width, normalWidth);
}

/* Inherited text properties changed on an ancestor box must reach the text
   nodes below it: the label re-measures with the new size (and re-bakes its
   colour) on the next update, without the text node itself being touched. */
TEST(Node, AncestorTextStyleChangeRemeasuresText) {
    Window window;
    window.setSize({ 640.0f, 480.0f });

    Document document(window);

    Node box;
    Node label;
    label.setDisplay(NodeDisplay::Text);
    label.setContent("Count: 8");
    box.appendChild(label);
    document.appendChild(box);

    box.setFontSize(12.0f);
    document.update();
    auto const smallWidth = label.getComputedBorderRect().width;
    ASSERT_GT(smallWidth, 0.0f);

    box.setFontSize(32.0f);
    document.update();
    ASSERT_GT(label.getComputedBorderRect().width, smallWidth);
    ASSERT_FLOAT_EQ(label.getComputedFontSize(), 32.0f);

    /* Two levels up, through a middle box that sets nothing. */
    Node outer;
    Node middle;
    Node deep;
    deep.setDisplay(NodeDisplay::Text);
    deep.setContent("Count: 8");
    middle.appendChild(deep);
    outer.appendChild(middle);
    document.appendChild(outer);

    outer.setFontSize(12.0f);
    document.update();
    auto const deepSmall = deep.getComputedBorderRect().width;

    outer.setFontSize(32.0f);
    document.update();
    ASSERT_GT(deep.getComputedBorderRect().width, deepSmall);
}

/* triggerEvent fires only the requested channel, without propagation. */
TEST(Node, TriggerEvent) {
    Node parent;
    Node node;
    parent.appendChild(node);

    auto bubbleCount = 0;
    auto captureCount = 0;
    auto parentCount = 0;
    Sub<NodeEvent const&> bubbleSub(node.onEvent, [&bubbleCount](NodeEvent const&) { bubbleCount += 1; });
    Sub<NodeEvent const&> captureSub(node.onCaptureEvent, [&captureCount](NodeEvent const&) { captureCount += 1; });
    Sub<NodeEvent const&> parentSub(parent.onEvent, [&parentCount](NodeEvent const&) { parentCount += 1; });

    TestEvent event(node);
    node.triggerEvent(event);
    ASSERT_TRUE(bubbleCount == 1);
    ASSERT_TRUE(captureCount == 0);
    ASSERT_TRUE(parentCount == 0);

    node.triggerEvent(event, true);
    ASSERT_TRUE(bubbleCount == 1);
    ASSERT_TRUE(captureCount == 1);
    ASSERT_TRUE(parentCount == 0);
}

/* The event carries its target node, and events are Objects. */
TEST(Node, EventCarriesTarget) {
    Node node;
    TestEvent event(node);
    ASSERT_TRUE(&event.getNode() == &node);
    NodeEvent const& base = event;
    ASSERT_TRUE(base.is<TestEvent>());
    ASSERT_TRUE(base.as<TestEvent>() == &event);
    ASSERT_TRUE(base.as<MouseClickNodeEvent>() == nullptr);
}

/* An existing subclass works through the same machinery. */
TEST(Node, EventSubclass) {
    Node node;
    MouseEnterNodeEvent event(node, Vec2(3.0f, 4.0f), KeyModifiers{false, true, false, false});
    ASSERT_TRUE(&event.getNode() == &node);
    ASSERT_TRUE(event.getPosition().x == 3.0f);
    ASSERT_TRUE(event.getPosition().y == 4.0f);
    ASSERT_TRUE(event.getModifiers().shift);
    ASSERT_TRUE(!event.getModifiers().control);

    auto count = 0;
    Sub<NodeEvent const&> sub(node.onEvent, [&count](NodeEvent const& e) {
        ASSERT_TRUE(e.is<MouseEnterNodeEvent>());
        count += 1;
    });
    node.triggerEvent(event);
    ASSERT_TRUE(count == 1);
}

/* dispatchEvent runs capture root->leaf, then bubble leaf->root. */
TEST(Node, DispatchEventOrder) {
    Node root;
    Node mid;
    Node leaf;
    root.appendChild(mid);
    mid.appendChild(leaf);

    std::vector<std::string> order;
    Sub<NodeEvent const&> rootCapture(root.onCaptureEvent, [&order](NodeEvent const&) { order.push_back("root:capture"); });
    Sub<NodeEvent const&> midCapture(mid.onCaptureEvent, [&order](NodeEvent const&) { order.push_back("mid:capture"); });
    Sub<NodeEvent const&> leafCapture(leaf.onCaptureEvent, [&order](NodeEvent const&) { order.push_back("leaf:capture"); });
    Sub<NodeEvent const&> rootBubble(root.onEvent, [&order](NodeEvent const&) { order.push_back("root:bubble"); });
    Sub<NodeEvent const&> midBubble(mid.onEvent, [&order](NodeEvent const&) { order.push_back("mid:bubble"); });
    Sub<NodeEvent const&> leafBubble(leaf.onEvent, [&order](NodeEvent const&) { order.push_back("leaf:bubble"); });

    TestEvent event(leaf);
    leaf.dispatchEvent(event);
    ASSERT_TRUE(order == (std::vector<std::string>{
        "root:capture", "mid:capture", "leaf:capture",
        "leaf:bubble", "mid:bubble", "root:bubble"
    }));
}

/* stopPropagation in the target's bubble handler stops the upward walk. */
TEST(Node, StopPropagationBubble) {
    Node root;
    Node mid;
    Node leaf;
    root.appendChild(mid);
    mid.appendChild(leaf);

    std::vector<std::string> order;
    Sub<NodeEvent const&> leafBubble(leaf.onEvent, [&order](NodeEvent const& e) {
        order.push_back("leaf:bubble");
        e.stopPropagation();
    });
    Sub<NodeEvent const&> midBubble(mid.onEvent, [&order](NodeEvent const&) { order.push_back("mid:bubble"); });
    Sub<NodeEvent const&> rootBubble(root.onEvent, [&order](NodeEvent const&) { order.push_back("root:bubble"); });

    TestEvent event(leaf);
    leaf.dispatchEvent(event);
    ASSERT_TRUE(order == (std::vector<std::string>{"leaf:bubble"}));
}

/* stopPropagation in the root's capture handler suppresses the remaining
 * capture-phase deliveries and the upward bubble walk. Note: the current
 * implementation still delivers the bubble-channel event to the target
 * itself, because dispatch fires the target's bubble notification before
 * consulting the stopPropagation flag. */
TEST(Node, StopPropagationCapture) {
    Node root;
    Node mid;
    Node leaf;
    root.appendChild(mid);
    mid.appendChild(leaf);

    std::vector<std::string> order;
    Sub<NodeEvent const&> rootCapture(root.onCaptureEvent, [&order](NodeEvent const& e) {
        order.push_back("root:capture");
        e.stopPropagation();
    });
    Sub<NodeEvent const&> midCapture(mid.onCaptureEvent, [&order](NodeEvent const&) { order.push_back("mid:capture"); });
    Sub<NodeEvent const&> leafCapture(leaf.onCaptureEvent, [&order](NodeEvent const&) { order.push_back("leaf:capture"); });
    Sub<NodeEvent const&> rootBubble(root.onEvent, [&order](NodeEvent const&) { order.push_back("root:bubble"); });
    Sub<NodeEvent const&> midBubble(mid.onEvent, [&order](NodeEvent const&) { order.push_back("mid:bubble"); });
    Sub<NodeEvent const&> leafBubble(leaf.onEvent, [&order](NodeEvent const&) { order.push_back("leaf:bubble"); });

    TestEvent event(leaf);
    leaf.dispatchEvent(event);
    ASSERT_TRUE(order == (std::vector<std::string>{"root:capture", "leaf:bubble"}));
}

/* broadcastEvent delivers pre-order depth-first over the subtree, on the
 * bubble channel only. */
TEST(Node, BroadcastEvent) {
    Node root;
    Node left;
    Node leftChild;
    Node right;
    root.appendChild(left);
    root.appendChild(right);
    left.appendChild(leftChild);

    std::vector<std::string> order;
    Sub<NodeEvent const&> rootBubble(root.onEvent, [&order](NodeEvent const&) { order.push_back("root"); });
    Sub<NodeEvent const&> leftBubble(left.onEvent, [&order](NodeEvent const&) { order.push_back("left"); });
    Sub<NodeEvent const&> leftChildBubble(leftChild.onEvent, [&order](NodeEvent const&) { order.push_back("leftChild"); });
    Sub<NodeEvent const&> rightBubble(right.onEvent, [&order](NodeEvent const&) { order.push_back("right"); });
    Sub<NodeEvent const&> rootCapture(root.onCaptureEvent, [&order](NodeEvent const&) { order.push_back("root:capture"); });

    TestEvent event(root);
    root.broadcastEvent(event);
    ASSERT_TRUE(order == (std::vector<std::string>{"root", "left", "leftChild", "right"}));
}
