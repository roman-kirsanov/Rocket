/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <string>
#include <vector>
#include <optional>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Window/Window.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Document.hpp>
#include <Rocket/UI/Node.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* A minimal composite component wrapping a Node, as Tooltip/Icon do. */
struct _WrappedProps {
    std::string key;
    class Node** ref = nullptr;
};

static void _Wrapped(_WrappedProps const& props) {
    COMPONENT

    Node({ .ref = props.ref });
}

/* Note: windows are never made visible, so nothing flashes on screen while
   the suite runs. The declarative Node component is driven by a Reconciler
   whose update function provides the Document as a context — first manually
   via Context, then through the declarative Document component. */

/* One reconciler drives every stage of a node component's life: the stages
   share the window, document, refs and setUpdateFn closure declared up front,
   and each stage depends on the state left by the previous one, so they run
   in order inside a single test. */
TEST(UINode, NodeComponentLifecycle) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });

    class Document document(window);
    auto reconciler = Reconciler();

    class Node* outer = nullptr;
    class Node* inner = nullptr;
    class Node* rooted = nullptr;

    auto renderTree = true;
    auto renderInner = true;
    auto width = NodeValue(100.0f);
    auto events = 0;

    reconciler.setUpdateFn([&] {
        Context(document, [&] {
            if (renderTree == false) {
                return;
            }

            Node({
                .ref = &outer,
                .width = width,
                .height = NodeValue(50.0f),
                .onEvent = [&](NodeEvent const&) { events += 1; },
            }, [&] {
                if (renderInner) {
                    Node({
                        .ref = &inner,
                        .key = "inner",
                        .display = NodeDisplay::Text,
                        .content = "hello",
                    });
                }

                Node({
                    .ref = &rooted,
                    .key = "rooted",
                    .root = true,
                });
            });
        });
    });

    /* Mount: the components create retained nodes attached under the document,
       and forward refs to the underlying nodes. */
    {
        reconciler.update();

        ASSERT_TRUE(outer != nullptr);
        ASSERT_TRUE(inner != nullptr);
        ASSERT_TRUE(rooted != nullptr);
        ASSERT_TRUE(outer->getParent() == &document);
        ASSERT_TRUE(document.getFirstChild() == outer);
        ASSERT_TRUE(outer->getDocument() == &document);
    }

    /* Prop sync: props round-trip through the retained accessors; an unset
       display stays unset (the retained node renders it as Box). */
    {
        ASSERT_TRUE(outer->getWidth() == NodeValue(100.0f));
        ASSERT_TRUE(outer->getHeight() == NodeValue(50.0f));
        ASSERT_TRUE(outer->getDisplay() == std::nullopt);
    }

    /* Nesting: the inner component parents under the outer component's node;
       display Text and content are applied. */
    {
        ASSERT_TRUE(inner->getParent() == outer);
        ASSERT_TRUE(outer->getFirstChild() == inner);
        ASSERT_TRUE(inner->getDisplay() == NodeDisplay::Text);
        ASSERT_TRUE(inner->getContent() == "hello");
    }

    /* root = true parents to the document even when nested. */
    {
        ASSERT_TRUE(rooted->getParent() == &document);
        ASSERT_TRUE(outer->getNextSibling() == rooted);
    }

    /* Re-render: state (and the retained node) persists; a changed prop is
       pushed to the node. */
    {
        auto outerBefore = outer;

        reconciler.update();
        ASSERT_TRUE(outer == outerBefore);
        ASSERT_TRUE(outer->getWidth() == NodeValue(100.0f));

        width = NodeValue(200.0f);
        reconciler.update();
        ASSERT_TRUE(outer == outerBefore);
        ASSERT_TRUE(outer->getWidth() == NodeValue(200.0f));
    }

    /* Event forwarding: an event fired on the retained node reaches the
       props.onEvent callback. */
    {
        events = 0;
        outer->triggerEvent(MouseClickNodeEvent(*outer, { 0.0f, 0.0f }, KeyModifiers{}));
        ASSERT_TRUE(events == 1);
    }

    /* Conditional rendering: a component that stops rendering unmounts, its
       node is destroyed and detached, and its ref is reset to nullptr. */
    {
        auto rootedBefore = rooted;

        renderInner = false;
        reconciler.update();

        ASSERT_TRUE(inner == nullptr);
        ASSERT_TRUE(outer->getFirstChild() == nullptr);
        ASSERT_TRUE(rooted == rootedBefore); /* keyed sibling keeps its state */
        ASSERT_TRUE(rooted->getParent() == &document);
    }

    /* Unmounting the whole tree detaches every node from the document and
       clears every ref. */
    {
        renderTree = false;
        reconciler.update();

        ASSERT_TRUE(outer == nullptr);
        ASSERT_TRUE(rooted == nullptr);
        ASSERT_TRUE(document.getFirstChild() == nullptr);
    }

    /* Insertion order: a sibling mounted between existing children lands in
       the middle, even when the following sibling's node lives inside a
       composite component (regression: it used to be appended last). */
    {
        class Node* parent = nullptr;
        class Node* first = nullptr;
        class Node* middle = nullptr;
        class Node* wrapped = nullptr;

        auto renderMiddle = false;
        auto order = Reconciler();

        order.setUpdateFn([&] {
            Context(document, [&] {
                Node({ .ref = &parent }, [&] {
                    Node({ .ref = &first, .key = "first" });

                    if (renderMiddle) {
                        Node({ .ref = &middle, .key = "middle" });
                    }

                    _Wrapped({ .key = "wrapped", .ref = &wrapped });
                });
            });
        });

        order.update();
        ASSERT_TRUE(parent->getFirstChild() == first);
        ASSERT_TRUE(first->getNextSibling() == wrapped);

        renderMiddle = true;
        order.update();
        ASSERT_TRUE(first->getNextSibling() == middle);
        ASSERT_TRUE(middle->getNextSibling() == wrapped);

        renderMiddle = false;
        order.update();
        ASSERT_TRUE(middle == nullptr);
        ASSERT_TRUE(first->getNextSibling() == wrapped);
        ASSERT_TRUE(wrapped->getNextSibling() == nullptr);
    }
}

/* Full declarative stack: a Document component consumes the Window
   context, provides the Document context to Node components (they parent
   under it), forwards its ref, syncs the scale prop, and ends every pass
   laid out (update) and painted (render) — safely, on a hidden window. */
TEST(UINode, FullDeclarativeStack) {
    auto stackWindow = Window();
    stackWindow.setSize({ 640.0f, 480.0f });

    auto stack = Reconciler();

    class Document* doc = nullptr;
    class Node* box = nullptr;

    stack.setUpdateFn([&] {
        Context(stackWindow, [&] {
            Document({
                .ref = &doc,
                .scale = 2.0f,
            }, [&] {
                Node({
                    .ref = &box,
                    .width = NodeValue(120.0f),
                    .height = NodeValue(60.0f),
                });
            });
        });
    });

    stack.update();

    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(box != nullptr);
    ASSERT_TRUE(&doc->getWindow() == &stackWindow);
    ASSERT_TRUE(doc->getScale() == 2.0f);
    ASSERT_TRUE(box->getParent() == doc);
    ASSERT_TRUE(box->getDocument() == doc);

    /* The pass ran document.update(): layout is already computed. */
    ASSERT_TRUE(box->getComputedBorderRect() == Vec4(0.0f, 0.0f, 120.0f, 60.0f));

    /* Unmounting destroys the document and clears both refs. */
    stack.setUpdateFn([&] { Context(stackWindow, [] {}); });
    stack.update();
    ASSERT_TRUE(doc == nullptr);
    ASSERT_TRUE(box == nullptr);
}

/* --- Sibling order of retained nodes after remounts and keyed reorders --- */

/* Renders one keyed Node per name under `parent`, skipping names listed in
   `hidden`, and returns the names in retained sibling order. */
struct _OrderHarness {
    Window window;
    class Document document;
    Reconciler reconciler;
    std::vector<std::string> names;
    std::vector<std::string> hidden;
    class Node* parent = nullptr;
    std::map<std::string, class Node*> refs;

    _OrderHarness()
        : window()
        , document(window) {
        window.setSize({ 640.0f, 480.0f });

        reconciler.setUpdateFn([&] {
            Context(document, [&] {
                Node({ .ref = &parent }, [&] {
                    for (auto const& name : names) {
                        auto isHidden = false;
                        for (auto const& h : hidden) {
                            if (h == name) isHidden = true;
                        }
                        if (isHidden == false) {
                            Node({ .ref = &refs[name], .key = name });
                        }
                    }
                });
            });
        });
    }

    std::vector<std::string> render() {
        reconciler.update();

        auto order = std::vector<std::string>();
        for (auto child = parent->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
            for (auto const& [name, node] : refs) {
                if (node == child) order.push_back(name);
            }
        }
        return order;
    }
};

TEST(UINode, RemountMiddleSibling) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c" };

    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "b", "c" }));

    h.hidden = { "b" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "c" }));

    h.hidden = {};
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "b", "c" }));
}

TEST(UINode, RemountFirstSibling) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c" };
    h.render();

    h.hidden = { "a" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "b", "c" }));

    h.hidden = {};
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "b", "c" }));
}

TEST(UINode, RemountTwoSiblings) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c", "d", "e" };
    h.render();

    h.hidden = { "b", "d" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "c", "e" }));

    h.hidden = {};
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "b", "c", "d", "e" }));
}

TEST(UINode, KeyedSwapOfLeadingPair) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c", "d" };
    h.render();

    /* c and d keep their index, so only a and b are re-parented. */
    h.names = { "b", "a", "c", "d" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "b", "a", "c", "d" }));
}

TEST(UINode, KeyedRotate) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c" };
    h.render();

    h.names = { "c", "a", "b" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "c", "a", "b" }));
}

TEST(UINode, KeyedMoveLastToFront) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c", "d" };
    h.render();

    h.names = { "d", "a", "b", "c" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "d", "a", "b", "c" }));
}

TEST(UINode, KeyedSwapOfMiddlePair) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c", "d", "e" };
    h.render();

    h.names = { "a", "c", "b", "d", "e" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "c", "b", "d", "e" }));

    /* And back again, exercising the re-appended tail a second time. */
    h.names = { "a", "b", "c", "d", "e" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "b", "c", "d", "e" }));
}

TEST(UINode, KeyedReverse) {
    auto h = _OrderHarness();
    h.names = { "a", "b", "c", "d" };
    h.render();

    h.names = { "d", "c", "b", "a" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "d", "c", "b", "a" }));
}

TEST(UINode, StableOrderDoesNotReparent) {
    /* A render with no change must not move any node: the untouched prefix
       and tail keep their retained positions. */
    auto h = _OrderHarness();
    h.names = { "a", "b", "c" };
    h.render();

    auto const before = std::vector<class Node*>{ h.refs["a"], h.refs["b"], h.refs["c"] };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "b", "c" }));
    ASSERT_EQ(h.refs["a"], before[0]);
    ASSERT_EQ(h.refs["b"], before[1]);
    ASSERT_EQ(h.refs["c"], before[2]);
}

/* Top-level siblings, parented straight to the Document (no wrapping Node),
   rendered through the Document component as applications do. */
struct _DocumentOrderHarness {
    Window window;
    class Document* document = nullptr;
    Reconciler reconciler;
    std::vector<std::string> names;
    std::vector<std::string> hidden;
    std::map<std::string, class Node*> refs;

    _DocumentOrderHarness() {
        window.setSize({ 640.0f, 480.0f });

        reconciler.setUpdateFn([&] {
            Context(window, [&] {
                Document({ .ref = &document }, [&] {
                    for (auto const& name : names) {
                        auto isHidden = false;
                        for (auto const& h : hidden) {
                            if (h == name) isHidden = true;
                        }
                        if (isHidden == false) {
                            Node({ .ref = &refs[name], .key = name });
                        }
                    }
                });
            });
        });
    }

    std::vector<std::string> render() {
        reconciler.update();

        auto order = std::vector<std::string>();
        for (auto child = document->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
            for (auto const& [name, node] : refs) {
                if (node == child) order.push_back(name);
            }
        }
        return order;
    }
};

TEST(UINode, DocumentRemountMiddleSibling) {
    auto h = _DocumentOrderHarness();
    h.names = { "a", "b", "c" };
    h.render();

    h.hidden = { "b" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "c" }));

    h.hidden = {};
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "a", "b", "c" }));
}

TEST(UINode, DocumentKeyedSwapOfLeadingPair) {
    auto h = _DocumentOrderHarness();
    h.names = { "a", "b", "c", "d" };
    h.render();

    h.names = { "b", "a", "c", "d" };
    ASSERT_EQ(h.render(), (std::vector<std::string>{ "b", "a", "c", "d" }));
}

/* Keyless siblings re-match by ordinal: when the first of two unmounts, the
   second's props land on the first's component. The forwarded ref must end
   up pointing at the surviving node, and the unmounted component must not
   clear a ref it no longer owns. */
TEST(UINode, RefSurvivesOrdinalRematch) {
    Window window;
    window.setSize({ 640.0f, 480.0f });

    class Document document(window);
    auto reconciler = Reconciler();

    class Node* parent = nullptr;
    class Node* first = nullptr;
    class Node* second = nullptr;
    auto renderFirst = true;

    reconciler.setUpdateFn([&] {
        Context(document, [&] {
            Node({ .ref = &parent }, [&] {
                if (renderFirst) {
                    Node({ .ref = &first, .width = NodeValue(10.0f) });
                }
                Node({ .ref = &second, .width = NodeValue(20.0f) });
            });
        });
    });

    reconciler.update();
    ASSERT_TRUE(first != nullptr);
    ASSERT_TRUE(second != nullptr);
    ASSERT_TRUE(parent->getFirstChild() == first);

    renderFirst = false;
    reconciler.update();
    ASSERT_TRUE(first == nullptr);
    ASSERT_TRUE(second != nullptr);
    ASSERT_TRUE(parent->getFirstChild() == second);
    ASSERT_TRUE(second->getNextSibling() == nullptr);
    ASSERT_TRUE(second->getWidth() == NodeValue(20.0f));
}
