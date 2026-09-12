/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <utility>
#include <Rocket/Base/Effect.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* The first evaluation always reports a change. */
TEST(Effect, FirstEvaluationReportsChange) {
    Effect<int> effect;
    ASSERT_TRUE(effect(1));
}

/* Unchanged dependencies report no change. */
TEST(Effect, UnchangedDependenciesReportNoChange) {
    Effect<int> effect;
    ASSERT_TRUE(effect(1));
    ASSERT_TRUE(!effect(1));
    ASSERT_TRUE(!effect(1));
}

/* A changed dependency reports a change and updates the snapshot. */
TEST(Effect, ChangedDependencyUpdatesSnapshot) {
    Effect<int> effect;
    ASSERT_TRUE(effect(1));
    ASSERT_TRUE(effect(2));
    ASSERT_TRUE(!effect(2));
    ASSERT_TRUE(effect(1));
    ASSERT_TRUE(!effect(1));
}

/* reset() re-arms the effect so the next evaluation fires. */
TEST(Effect, ResetRearmsEffect) {
    Effect<int> effect;
    ASSERT_TRUE(effect(5));
    ASSERT_TRUE(!effect(5));
    effect.reset();
    ASSERT_TRUE(effect(5));
    ASSERT_TRUE(!effect(5));
}

/* With multiple dependencies, any single change fires the effect. */
TEST(Effect, AnySingleDependencyChangeFires) {
    Effect<int, std::string> effect;
    ASSERT_TRUE(effect(1, "a"));
    ASSERT_TRUE(!effect(1, "a"));
    ASSERT_TRUE(effect(2, "a"));
    ASSERT_TRUE(!effect(2, "a"));
    ASSERT_TRUE(effect(2, "b"));
    ASSERT_TRUE(!effect(2, "b"));
    ASSERT_TRUE(effect(3, "c"));
}

/* Copy construction resets state, so the copy fires on first evaluation. */
TEST(Effect, CopyConstructionResetsState) {
    Effect<int> original;
    ASSERT_TRUE(original(7));
    ASSERT_TRUE(!original(7));

    Effect<int> copy(original);
    ASSERT_TRUE(copy(7));
    ASSERT_TRUE(!copy(7));

    /* The original keeps its own snapshot. */
    ASSERT_TRUE(!original(7));
}

/* Move construction also resets state in the new effect. */
TEST(Effect, MoveConstructionResetsState) {
    Effect<int> original;
    ASSERT_TRUE(original(7));

    Effect<int> moved(std::move(original));
    ASSERT_TRUE(moved(7));
    ASSERT_TRUE(!moved(7));
}

/* Assignment leaves the target's stored snapshot untouched. */
TEST(Effect, AssignmentLeavesSnapshotUntouched) {
    Effect<int> a;
    Effect<int> b;
    ASSERT_TRUE(a(1));
    ASSERT_TRUE(b(2));

    a = b;
    ASSERT_TRUE(!a(1)); /* still compares against its own snapshot */
    ASSERT_TRUE(a(3));

    Effect<int> c;
    ASSERT_TRUE(c(4));
    c = Effect<int>();
    ASSERT_TRUE(!c(4));
}

/* Effects with no dependencies fire once until reset. */
TEST(Effect, NoDependenciesFireOnceUntilReset) {
    Effect<> effect;
    ASSERT_TRUE(effect());
    ASSERT_TRUE(!effect());
    ASSERT_TRUE(!effect());
    effect.reset();
    ASSERT_TRUE(effect());
}
