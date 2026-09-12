/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/PubSub.hpp>

namespace Rocket {

/**
 * Base class for objects with RTTI helpers and a destruction event.
 *
 * Subclass Object to gain dynamic_cast-based type queries via as() and is(),
 * and to expose onDestroy so dependents can react before the object is gone.
 */
class Object {
public:
    /** Published once from the destructor, before member cleanup. */
    Pub<> onDestroy;

    Object();

    /**
     * Returns a const pointer to this object cast to Type, or nullptr if the cast fails.
     *
     * @tparam Type The target type.
     */
    template<typename Type>
    Type const* as() const;

    /**
     * Returns a mutable pointer to this object cast to Type, or nullptr if the cast fails.
     *
     * @tparam Type The target type.
     */
    template<typename Type>
    Type* as();

    /**
     * Returns true if this object is an instance of Type.
     *
     * @tparam Type The type to test against.
     */
    template<typename Type>
    bool is() const;

    virtual ~Object();
};

template<typename Type>
inline Type const* Object::as() const {
    PROFILE

    return dynamic_cast<Type const*>(this);
}

template<typename Type>
inline Type* Object::as() {
    PROFILE

    return dynamic_cast<Type*>(this);
}

template<typename Type>
inline bool Object::is() const {
    PROFILE

    return (dynamic_cast<Type const*>(this) != nullptr);
}

} /* namespace Rocket */
