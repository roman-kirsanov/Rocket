/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Base/Object.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

namespace {

class Animal : public Object {
public:
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    virtual ~Dog() = default;
};

class Cat : public Animal {
public:
    virtual ~Cat() = default;
};

} /* namespace */

/* is() reports the dynamic type across the whole hierarchy. */
TEST(Object, IsReportsDynamicType) {
    Dog dog;
    Object& object = dog;
    ASSERT_TRUE(object.is<Dog>());
    ASSERT_TRUE(object.is<Animal>());
    ASSERT_TRUE(object.is<Object>());
    ASSERT_TRUE(!object.is<Cat>());
}

/* as() downcasts to the dynamic type and returns nullptr otherwise. */
TEST(Object, AsDowncastsOrReturnsNullptr) {
    Dog dog;
    Object& object = dog;
    ASSERT_TRUE(object.as<Dog>() == &dog);
    ASSERT_TRUE(object.as<Animal>() == static_cast<Animal*>(&dog));
    ASSERT_TRUE(object.as<Cat>() == nullptr);
}

/* The const overload of as() mirrors the mutable one. */
TEST(Object, ConstAsMirrorsMutable) {
    Dog dog;
    Object const& object = dog;
    ASSERT_TRUE(object.as<Dog>() == &dog);
    ASSERT_TRUE(object.as<Cat>() == nullptr);
    ASSERT_TRUE(object.is<Dog>());
}

/* A sibling type is not an instance of the other sibling. */
TEST(Object, SiblingIsNotInstanceOfSibling) {
    Cat cat;
    Animal& animal = cat;
    ASSERT_TRUE(animal.is<Cat>());
    ASSERT_TRUE(!animal.is<Dog>());
    ASSERT_TRUE(animal.as<Dog>() == nullptr);
}

/* onDestroy publishes exactly once when the object is destroyed. */
TEST(Object, OnDestroyPublishesOnce) {
    auto count = 0;
    Sub<> sub;
    auto* dog = new Dog();
    sub.on(dog->onDestroy, [&count]() { count += 1; });
    ASSERT_TRUE(count == 0);
    delete dog;
    ASSERT_TRUE(count == 1);
}

/* onDestroy tracks subscription state via hasSubs. */
TEST(Object, OnDestroyTracksHasSubs) {
    Dog dog;
    ASSERT_TRUE(!dog.onDestroy.hasSubs());
    {
        Sub<> sub;
        sub.on(dog.onDestroy, []() {});
        ASSERT_TRUE(dog.onDestroy.hasSubs());
        sub.off(dog.onDestroy);
        ASSERT_TRUE(!dog.onDestroy.hasSubs());

        sub.on(dog.onDestroy, []() {});
        ASSERT_TRUE(dog.onDestroy.hasSubs());
    }
    ASSERT_TRUE(!dog.onDestroy.hasSubs()); /* Sub destructor unsubscribed */
}

/* An unsubscribed handler is not called on destruction. */
TEST(Object, UnsubscribedHandlerNotCalled) {
    auto count = 0;
    Sub<> sub;
    auto* dog = new Dog();
    sub.on(dog->onDestroy, [&count]() { count += 1; });
    sub.off(dog->onDestroy);
    delete dog;
    ASSERT_TRUE(count == 0);
}

/* Multiple subscribers are all notified of destruction. */
TEST(Object, MultipleSubscribersAllNotified) {
    auto first = 0;
    auto second = 0;
    Sub<> subFirst;
    Sub<> subSecond;
    auto* dog = new Dog();
    subFirst.on(dog->onDestroy, [&first]() { first += 1; });
    subSecond.on(dog->onDestroy, [&second]() { second += 1; });
    delete dog;
    ASSERT_TRUE(first == 1);
    ASSERT_TRUE(second == 1);
}
