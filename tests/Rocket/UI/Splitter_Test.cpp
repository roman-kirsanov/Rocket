/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Window/Window.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Splitter.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* A 100-point panel, a splitter after it and the rest of the row, under a manually provided Document. */
struct _SplitterHarness {
    Window window;
    class Document document;
    Reconciler reconciler;
    class Node* panel = nullptr;
    class Node* splitter = nullptr;
    std::vector<float> resizes;

    _SplitterHarness()
        : window()
        , document(window) {
        window.setSize({ 640.0f, 480.0f });

        reconciler.setUpdateFn([&] {
            Context(document, [&] {
                Node({ .direction = NodeDirection::Horizontal, .width = NodeValue(640.0f), .height = NodeValue(100.0f) }, [&] {
                    Node({ .ref = &panel, .width = NodeValue(100.0f) });
                    Splitter({
                        .onResize = [&](float size) { resizes.push_back(size); },
                        .nodeProps = { .ref = &splitter }
                    });
                    Node({ .flex = true });
                });
            });
        });
    }

    void render() {
        reconciler.update();
        document.update();
    }

    void drag(float distance, bool end) {
        splitter->dispatchEvent(MouseBeginDragNodeEvent(*splitter, { 100.0f, 10.0f }, { 0.0f, 0.0f }, KeyModifiers{}));
        splitter->dispatchEvent(MouseDragNodeEvent(*splitter, { (100.0f + distance), 10.0f }, { distance, 0.0f }, KeyModifiers{}));

        if (end) {
            splitter->dispatchEvent(MouseEndDragNodeEvent(*splitter, { (100.0f + distance), 10.0f }, { distance, 0.0f }, KeyModifiers{}));
        }
    }
};

TEST(Splitter, ResizesItsTargetWhileDragged) {
    auto h = _SplitterHarness();
    h.render();

    h.drag(30.0f, false);
    h.render();

    ASSERT_FLOAT_EQ(h.panel->getComputedBorderRect().width, 130.0f);
    ASSERT_TRUE(h.resizes.empty());
}

TEST(Splitter, ReportsTheNewSizeWhenTheDragEnds) {
    auto h = _SplitterHarness();
    h.render();

    h.drag(-40.0f, true);

    ASSERT_EQ(h.resizes, (std::vector<float>{ 60.0f }));
}

TEST(Splitter, ReportsNothingWhenADragEndsWithoutMoving) {
    auto h = _SplitterHarness();
    h.render();

    h.splitter->dispatchEvent(MouseBeginDragNodeEvent(*h.splitter, { 100.0f, 10.0f }, { 0.0f, 0.0f }, KeyModifiers{}));
    h.splitter->dispatchEvent(MouseEndDragNodeEvent(*h.splitter, { 100.0f, 10.0f }, { 0.0f, 0.0f }, KeyModifiers{}));

    ASSERT_TRUE(h.resizes.empty());
}
