/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* Randomized soak test: builds a random node tree, then drives thousands of
   random input events interleaved with structural mutations (append, detach,
   delete — including mid-drag and mid-edit), checking engine invariants after
   every step. The answer to combinatorial capability explosion: stop
   enumerating, randomize under invariants. ASan (on in Debug) backstops the
   dangling-pointer class; the invariants below catch structural corruption.

   Deterministic by default; set ROCKET_FUZZ_SEED to replay a failure — the
   seed and step index are printed in every assertion message. */

namespace {

std::string const _typables[] = { "a", "Z", "0", " ", "hello", "é", "\U0001F600", "\n" };

struct Fuzzer {
    std::mt19937 rng;
    Window window;
    Document document{ window };
    std::vector<std::unique_ptr<Node>> pool;
    std::uint32_t seed;
    int step = 0;

    Fuzzer(std::uint32_t seed) : rng(seed), seed(seed) {
        window.setSize({ 640.0f, 480.0f });
    }

    int intIn(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); }
    float floatIn(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); }
    bool chance(int percent) { return intIn(1, 100) <= percent; }

    /** All pool nodes currently attached to the document (plus the document itself as an append target). */
    Node* randomAttachedParent() {
        auto attached = std::vector<Node*>();
        for (auto& node : pool) {
            if (node->getDocument() == &document) attached.push_back(node.get());
        }
        if (attached.empty() || chance(15)) return &document;
        return attached[(std::size_t)intIn(0, (int)attached.size() - 1)];
    }

    Node* randomPoolNode() {
        if (pool.empty()) return nullptr;
        return pool[(std::size_t)intIn(0, (int)pool.size() - 1)].get();
    }

    Node& makeNode() {
        pool.push_back(std::make_unique<Node>());
        auto& node = *pool.back();

        if (chance(20)) { /* text node */
            node.setDisplay(NodeDisplay::Text);
            node.setContent(_typables[(std::size_t)intIn(0, 7)] + " lorem ipsum dolor");
            return node;
        }

        node.setDisplay(NodeDisplay::Box);
        if (chance(70)) node.setWidth(floatIn(10.0f, 250.0f));
        if (chance(70)) node.setHeight(floatIn(10.0f, 250.0f));
        if (chance(30)) node.setFlex(true);
        if (chance(25)) node.setZIndex(intIn(-2, 3));
        if (chance(20)) node.setPaddingLeft(floatIn(0.0f, 20.0f));
        if (chance(20)) node.setOverflowY(chance(50) ? NodeOverflow::Scroll : NodeOverflow::Hidden);
        if (chance(15)) node.setTabIndex(intIn(1, 3));
        if (chance(10)) node.setVisible(false);
        if (chance(10)) node.setMouseEvents(false);
        if (chance(8))  node.setSkip(true);
        if (chance(10)) node.setOpacity(floatIn(0.1f, 1.0f));

        if (chance(15)) { /* editable box: needs a single text child */
            node.setContentEditable(true);
            auto& text = makeNode();
            text.setDisplay(NodeDisplay::Text);
            text.setContent("editable content");
            node.appendChild(text);
        }

        return node;
    }

    void buildInitialTree() {
        auto const count = intIn(20, 40);
        for (auto i = 0; i < count; i++) {
            auto& node = makeNode();
            if (node.getParent() == nullptr) { /* editable text children are already attached */
                randomAttachedParent()->appendChild(node);
            }
        }
        document.update();
    }

    Vec2 randomPosition() {
        /* mostly inside the window, sometimes at or past the edges */
        if (chance(85)) return { floatIn(0.0f, 640.0f), floatIn(0.0f, 480.0f) };
        return { floatIn(-200.0f, 900.0f), floatIn(-200.0f, 700.0f) };
    }

    void randomEvent() {
        auto const roll = intIn(1, 100);

        if (roll <= 30) {
            window.onEvent.publish(MouseMoveWindowEvent(window, randomPosition(), KeyModifiers{}));
        } else if (roll <= 40) {
            window.onEvent.publish(MouseDownWindowEvent(window, chance(85) ? Mouse::LeftButton : Mouse::RightButton, randomPosition(), KeyModifiers{}));
        } else if (roll <= 50) {
            window.onEvent.publish(MouseUpWindowEvent(window, chance(85) ? Mouse::LeftButton : Mouse::RightButton, randomPosition(), KeyModifiers{}));
        } else if (roll <= 58) {
            window.onEvent.publish(MouseWheelWindowEvent(window, { floatIn(-300.0f, 300.0f), floatIn(-300.0f, 300.0f) }, KeyModifiers{}));
        } else if (roll <= 72) {
            /* keyboard: navigation, deletion, select-all, and typing — no
               clipboard keys, so the system clipboard is never touched */
            auto const mods = KeyModifiers{
                .shift = chance(30),
                .meta = chance(15),
                .alt = chance(15)
            };
            auto const keyRoll = intIn(0, 9);
            auto key = Rocket::Key::Unknown;
            auto input = std::string();
            switch (keyRoll) {
                case 0: key = Rocket::Key::ArrowLeft; break;
                case 1: key = Rocket::Key::ArrowRight; break;
                case 2: key = Rocket::Key::ArrowUp; break;
                case 3: key = Rocket::Key::ArrowDown; break;
                case 4: key = Rocket::Key::Home; break;
                case 5: key = Rocket::Key::End; break;
                case 6: key = Rocket::Key::Backspace; break;
                case 7: key = Rocket::Key::Delete; break;
                case 8: key = Rocket::Key::Enter; break;
                default: input = _typables[(std::size_t)intIn(0, 7)]; break;
            }
            if (chance(5)) { key = Rocket::Key::KeyA; input = ""; } /* meta-A select-all sometimes */
            window.onEvent.publish(KeyDownWindowEvent(window, key, mods, input));
            window.onEvent.publish(KeyUpWindowEvent(window, key, mods));
        } else if (roll <= 88) {
            /* structural mutation — deliberately legal mid-drag and mid-edit */
            auto const kind = intIn(0, 5);
            if (kind == 0) { /* grow */
                auto& node = makeNode();
                if (node.getParent() == nullptr) randomAttachedParent()->appendChild(node);
            } else if (kind == 1) { /* detach (stays in the pool) */
                if (auto node = randomPoolNode()) node->removeFromParent();
            } else if (kind == 2) { /* destroy outright */
                if (pool.empty() == false) {
                    pool.erase(pool.begin() + intIn(0, (int)pool.size() - 1));
                }
            } else if (kind == 3) { /* re-attach a detached node (the cycle guard makes any target safe) */
                if (auto node = randomPoolNode()) {
                    if (node->getParent() == nullptr && node->getDocument() != &document) {
                        randomAttachedParent()->appendChild(*node);
                    }
                }
            } else if (kind == 4) { /* insert a new node at an arbitrary index (negative and past-the-end included) */
                auto& node = makeNode();
                if (node.getParent() == nullptr) randomAttachedParent()->insertChild(node, intIn(-1, 8));
            } else { /* move any node to an arbitrary index of any attached parent: reparents,
                        moves within its own parent, or no-ops when already there */
                if (auto node = randomPoolNode()) {
                    randomAttachedParent()->insertChild(*node, intIn(-1, 8));
                }
            }
        } else if (roll <= 92) {
            document.setScale(float(intIn(2, 6)) * 0.5f); /* 1.0 .. 3.0 */
        } else if (roll <= 96) {
            window.setSize({ floatIn(200.0f, 1200.0f), floatIn(200.0f, 900.0f) });
        } else {
            document.update();
            document.render(); /* headless window pass is a no-op, but the render path runs */
        }
    }

    /** Walks the attached tree checking structural invariants. */
    void checkInvariants() {
        auto trace = [&](char const* what) {
            return testing::Message() << what << " (seed " << seed << ", step " << step << ")";
        };

        auto walk = [&](Node* node, auto&& walk) -> void {
            Node* prev = nullptr;
            for (auto child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
                ASSERT_TRUE(child->getParent() == node) << trace("child/parent link broken");
                ASSERT_TRUE(child->getPrevSibling() == prev) << trace("sibling chain broken");
                ASSERT_TRUE(child->getDocument() == &document) << trace("attached child lost its document");

                auto const& rect = child->getComputedBorderRect();
                ASSERT_TRUE(std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.width) && std::isfinite(rect.height))
                    << trace("non-finite border rect")
                    << " rect (" << rect.x << "," << rect.y << " " << rect.width << "x" << rect.height << ")"
                    << " display " << (child->getDisplay().has_value() ? (int)*child->getDisplay() : -1)
                    << " skip " << (int)child->getSkip()
                    << " parentSkip " << (int)node->getSkip()
                    << " flex " << (int)child->getFlex();

                /* every node inherits its parent's clip before intersecting its
                   own bounds, so a child clip never escapes its parent's — an
                   empty clip region (zero width or height) is trivially inside */
                auto const& clip = child->getComputedClipRect();
                auto const& parentClip = node->getComputedClipRect();
                if ((clip.width > 0.0f) && (clip.height > 0.0f)) {
                    ASSERT_TRUE(
                        (clip.x >= parentClip.x - 0.01f) &&
                        (clip.y >= parentClip.y - 0.01f) &&
                        (clip.getMaxX() <= parentClip.getMaxX() + 0.01f) &&
                        (clip.getMaxY() <= parentClip.getMaxY() + 0.01f)
                    )
                        << trace("child clip escapes parent clip")
                        << " child clip (" << clip.x << "," << clip.y << " " << clip.width << "x" << clip.height << ")"
                        << " parent clip (" << parentClip.x << "," << parentClip.y << " " << parentClip.width << "x" << parentClip.height << ")";
                }

                walk(child, walk);
                prev = child;
            }
            ASSERT_TRUE(node->getLastChild() == prev) << trace("lastChild does not match the sibling chain");
        };
        walk(&document, walk);

        /* hover/focus flags only ever mark attached nodes */
        for (auto& node : pool) {
            if (node->getDocument() != &document) {
                ASSERT_TRUE(node->isHover() == false) << trace("detached node still hovered");
                ASSERT_TRUE(node->isFocused() == false) << trace("detached node still focused");
            }
        }
    }

    void run(int steps) {
        buildInitialTree();

        for (step = 0; step < steps; step++) {
            randomEvent();
            document.update();
            checkInvariants();
            if (::testing::Test::HasFatalFailure()) return;

            if ((step % 100) == 99) {
                /* update() is idempotent: a second call changes no rects */
                auto const before = document.getComputedBorderRect();
                document.update();
                ASSERT_TRUE(document.getComputedBorderRect() == before)
                    << testing::Message() << "update() not idempotent (seed " << seed << ", step " << step << ")";
            }
        }
    }
};

} /* namespace */

TEST(Fuzz, RandomTreeAndEventSoak) {
    auto seeds = std::vector<std::uint32_t>();

    if (auto const env = std::getenv("ROCKET_FUZZ_SEED")) {
        seeds.push_back((std::uint32_t)std::strtoul(env, nullptr, 10));
    } else {
        seeds = { 20260802u, 424242u, 7u }; /* fixed: deterministic in CI */
    }

    for (auto const seed : seeds) {
        auto fuzzer = Fuzzer(seed);
        fuzzer.run(800);
        if (::testing::Test::HasFatalFailure()) return;
    }
}
