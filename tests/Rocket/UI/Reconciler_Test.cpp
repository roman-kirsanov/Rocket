/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <Rocket/UI/Reconciler.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

static int trackedCtor = 0;
static int trackedDtor = 0;

struct Tracked {
    Tracked() { trackedCtor++; }
    ~Tracked() { trackedDtor++; }
    int value = 0;
};

struct Props {
    std::string key;
};

/* Parent/Child: state persistence & unmounting. */

static bool renderChild = true;
static int parentCount = 0;
static int childValue = 0;
static std::string childName;

static void Child(Props const& props) {
    COMPONENT

    auto& tracked = UseState<Tracked>();
    tracked.value += 1;
    childValue = tracked.value;
    childName = UseComponentName();
}

static void Parent(Props const& props) {
    COMPONENT

    auto& count = UseState<int>(0);
    count += 1;
    parentCount = count;

    if (renderChild) {
        Child({});
    }
}

/* Context visibility. */

static int seenContext = 0;
static int* outsideContext = nullptr;

static void Consumer() {
    COMPONENT_NO_KEY

    seenContext = UseContext<int>();
}

static void Outsider() {
    COMPONENT_NO_KEY

    outsideContext = UseContextIf<int>();
}

/* Multiple contexts per component. */

static int seenInt = 0;
static float seenFloat = 0.0f;
static int* seenIntIf = nullptr;

static void PairConsumer() {
    COMPONENT_NO_KEY

    seenInt = UseContext<int>();
    seenFloat = UseContext<float>();
}

static void IntProbe() {
    COMPONENT_NO_KEY

    seenIntIf = UseContextIf<int>();
}

static void PairProvider(int& a, float& b, std::function<void()> const& children) {
    COMPONENT_NO_KEY

    SetContext(a);
    SetContext(b);
    children();
}

/* Keyed identity. */

static std::unordered_map<std::string, int> itemValues;

static void Item(Props const& props) {
    COMPONENT

    auto& n = UseState<int>(0);
    n += 1;
    itemValues[UseComponentKey()] = n;
}

/* Effects. */

static int effectRuns = 0;
static int effectDep = 1;

static void Fx() {
    COMPONENT_NO_KEY

    if (UseEffect(effectDep)) {
        effectRuns += 1;
    }
}

/* Teardown. */

static void Leaf() {
    COMPONENT_NO_KEY

    UseState<Tracked>();
}

static void Nested() {
    COMPONENT_NO_KEY

    Leaf();
}

/* The following stages all drive one shared reconciler (and the file-scope
   counters above) and depend on the state left behind by earlier stages, so
   they run in order inside a single test. */
TEST(Reconciler, StateContextKeysEffectsAndScheduling) {
    auto reconciler = Reconciler();

    /* State persists across updates; child state is constructed once. */
    {
        reconciler.setUpdateFn([] { Parent({ .key = "root" }); });
        reconciler.update();
        reconciler.update();
        ASSERT_TRUE(parentCount == 2);
        ASSERT_TRUE(childValue == 2);
        ASSERT_TRUE(childName == "Child"); /* __func__ captured by COMPONENT */
        ASSERT_TRUE(trackedCtor == 1);
        ASSERT_TRUE(trackedDtor == 0);
    }

    /* Un-rendered components unmount and their state is destroyed. */
    {
        renderChild = false;
        reconciler.update();
        ASSERT_TRUE(parentCount == 3);
        ASSERT_TRUE(trackedDtor == 1);
    }

    /* Remounting creates fresh state. */
    {
        renderChild = true;
        reconciler.update();
        ASSERT_TRUE(trackedCtor == 2);
        ASSERT_TRUE(childValue == 1);
    }

    /* Context is visible to descendants, absent outside the provider. */
    {
        auto contextValue = 42;

        reconciler.setUpdateFn([&] {
            Context(contextValue, [] { Consumer(); });
            Outsider();
        });

        reconciler.update();
        ASSERT_TRUE(seenContext == 42);
        ASSERT_TRUE(outsideContext == nullptr);
    }

    /* Keys give components stable identity under reordering. */
    {
        auto order = std::vector<std::string>{ "a", "b" };

        reconciler.setUpdateFn([&] {
            for (auto& key : order) {
                Item({ .key = key });
            }
        });

        reconciler.update();
        reconciler.update();
        ASSERT_TRUE(itemValues["a"] == 2);

        order = { "b", "a" }; /* reorder -> "a" keeps its state */
        reconciler.update();
        ASSERT_TRUE(itemValues["a"] == 3);
    }

    /* UseEffect fires on first render and on dependency change. */
    {
        reconciler.setUpdateFn([] { Fx(); });
        reconciler.update(); /* first render -> runs */
        reconciler.update(); /* same dep -> skipped */
        ASSERT_TRUE(effectRuns == 1);

        effectDep = 2;
        reconciler.update(); /* dep changed -> runs */
        ASSERT_TRUE(effectRuns == 2);
    }

    /* The reconciler provides itself as a root context. */
    {
        auto seenReconciler = static_cast<Reconciler*>(nullptr);

        reconciler.setUpdateFn([&] {
            seenReconciler = UseContextIf<Reconciler>();
        });

        reconciler.update();
        ASSERT_TRUE(seenReconciler == &reconciler);
    }

    /* A re-entrant update() on the same reconciler (e.g. an event fired
       synchronously from inside a render) is coalesced into one follow-up
       render after the current one finishes, instead of corrupting it. */
    {
        auto renders = 0;
        auto requestOnce = true;

        reconciler.setUpdateFn([&] {
            renders += 1;

            auto& self = UseContext<Reconciler>();

            if (requestOnce) {
                requestOnce = false;
                self.update();          /* re-entrant: must not break the outer render */
            }

            UseState<int>() += 1;       /* hook after the re-entrant call must still work */
        });

        reconciler.update();
        ASSERT_TRUE(renders == 2);           /* original render + one coalesced follow-up */
    }

    /* needsUpdate() during a render schedules exactly one follow-up render;
       multiple requests within the same pass coalesce. */
    {
        auto renders = 0;

        reconciler.setUpdateFn([&] {
            renders += 1;

            auto& self = UseContext<Reconciler>();

            if (renders == 1) {
                self.needsUpdate();
                self.needsUpdate();     /* coalesces with the request above */
                self.needsUpdate();
            }
        });

        reconciler.update();
        ASSERT_TRUE(renders == 2);           /* original render + one follow-up */
    }

    /* Follow-up renders can request further renders; the chain stops as
       soon as a pass makes no request. */
    {
        auto renders = 0;

        reconciler.setUpdateFn([&] {
            renders += 1;

            if (renders < 3) {
                UseContext<Reconciler>().needsUpdate();
            }
        });

        reconciler.update();
        ASSERT_TRUE(renders == 3);           /* two requesting passes + one quiet pass */
    }

    /* needsUpdate() outside a render currently has no effect: update()
       always renders, exactly once when nothing requests more. */
    {
        auto renders = 0;

        reconciler.setUpdateFn([&] { renders += 1; });

        reconciler.needsUpdate();
        reconciler.update();
        ASSERT_TRUE(renders == 1);

        reconciler.update();
        ASSERT_TRUE(renders == 2);
    }
}

/* Hooks outside an update must throw. */
TEST(Reconciler, HooksOutsideUpdateThrow) {
    auto threw = false;

    try {
        UseStateAny();
    } catch (std::runtime_error const&) {
        threw = true;
    }

    ASSERT_TRUE(threw);
}

/* Destroying a reconciler tears down the tree and destroys live state. */
TEST(Reconciler, DestroyTearsDownState) {
    auto ctorsBefore = trackedCtor;
    auto dtorsBefore = trackedDtor;

    {
        auto scoped = Reconciler();
        scoped.setUpdateFn([] { Nested(); });
        scoped.update();
        ASSERT_TRUE(trackedCtor == ctorsBefore + 1);
        ASSERT_TRUE(trackedDtor == dtorsBefore); /* still alive while mounted */
    }

    ASSERT_TRUE(trackedDtor == dtorsBefore + 1); /* ~Reconciler destroyed it */
}

/* A component exposes several contexts at once; same-type SetContext
   replaces; contexts vanish when the component closes; the innermost
   provider wins and the outer value comes back once the inner closes;
   the per-component limit throws. */
TEST(Reconciler, MultipleContexts) {
    auto reconciler = Reconciler();

    /* Two types from one component, visible to the component's children. */
    {
        auto a = 7;
        auto b = 2.5f;

        reconciler.setUpdateFn([&] {
            PairProvider(a, b, [] { PairConsumer(); });
        });

        reconciler.update();
        ASSERT_TRUE(seenInt == 7);
        ASSERT_TRUE(seenFloat == 2.5f);
    }

    /* Same type set twice in one component: the later value wins. */
    {
        auto first = 1;
        auto second = 2;

        reconciler.setUpdateFn([&] {
            SetContext(first);
            SetContext(second);
            IntProbe();
        });

        reconciler.update();
        ASSERT_TRUE(seenIntIf == &second);
    }

    /* Scope: a sibling rendered after the provider closes sees nothing,
       and inside nested providers the innermost wins, then the outer value
       is visible again once the inner provider has closed. */
    {
        auto outer = 10;
        auto inner = 20;
        auto b = 0.0f;
        int* seenInner = nullptr;
        int* seenAfterInner = nullptr;
        int* seenAfterOuter = nullptr;

        reconciler.setUpdateFn([&] {
            PairProvider(outer, b, [&] {
                PairProvider(inner, b, [&] {
                    IntProbe();
                    seenInner = seenIntIf;
                });
                IntProbe();
                seenAfterInner = seenIntIf;
            });
            IntProbe();
            seenAfterOuter = seenIntIf;
        });

        reconciler.update();
        ASSERT_TRUE(seenInner == &inner);
        ASSERT_TRUE(seenAfterInner == &outer);
        ASSERT_TRUE(seenAfterOuter == nullptr);
    }

    /* A context set by a component is visible to that component itself. */
    {
        auto value = 3;
        int* seenSelf = nullptr;

        reconciler.setUpdateFn([&] {
            SetContext(value);
            seenSelf = UseContextIf<int>();
        });

        reconciler.update();
        ASSERT_TRUE(seenSelf == &value);
    }

    /* A sixth context type in one component throws. */
    {
        auto v1 = 1;
        auto v2 = 2.0f;
        auto v3 = 3.0;
        auto v4 = 'x';
        auto v5 = 5l;
        auto v6 = 6u;

        reconciler.setUpdateFn([&] {
            SetContext(v1);
            SetContext(v2);
            SetContext(v3);
            SetContext(v4);
            SetContext(v5);
            SetContext(v6);
        });

        ASSERT_THROW(reconciler.update(), std::runtime_error);

        /* The reconciler recovers on the next update. */
        reconciler.setUpdateFn([&] {
            SetContext(v1);
            IntProbe();
        });

        reconciler.update();
        ASSERT_TRUE(seenIntIf == &v1);
    }
}
