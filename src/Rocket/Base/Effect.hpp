/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <tuple>
#include <optional>
#include <Rocket/Base/Profile.hpp>

namespace Rocket {

/**
 * Tracks a set of dependency values and reports when they change.
 *
 * Calling operator() returns true the first time and on any subsequent call
 * where at least one value differs from the previous invocation. Copy and
 * move construction intentionally reset stored state so that copied effects
 * always fire on their first evaluation.
 *
 * @tparam Types The types of the tracked dependency values.
 */
template<typename... Types>
class Effect {
public:
    Effect();

    /**
     * Constructs with empty state; the moved-from snapshot is intentionally not carried over.
     *
     * @param other The effect to move from.
     */
    Effect(Effect<Types...> && other);

    /**
     * Constructs with empty state; the source's snapshot is intentionally not copied.
     *
     * @param other The effect to copy from.
     */
    Effect(Effect<Types...> const& other);
    Effect<Types...>& operator=(Effect<Types...> &&);
    Effect<Types...>& operator=(Effect<Types...> const&);

    /**
     * Evaluates the dependencies and returns whether they have changed.
     *
     * @param args The current dependency values.
     * @return true on the first call and whenever any value differs from the last call.
     */
    bool operator()(Types const&... args);

    /**
     * Clears the stored snapshot so the next operator() call returns true.
     */
    void reset();
private:
    std::optional<std::tuple<Types...>> _deps;
};

template<typename... Types>
inline Effect<Types...>::Effect()
    : _deps() {}

template<typename... Types>
inline Effect<Types...>::Effect(Effect<Types...> &&)
    : _deps() {}

template<typename... Types>
inline Effect<Types...>::Effect(Effect<Types...> const&)
    : _deps() {}

template<typename... Types>
inline Effect<Types...>& Effect<Types...>::operator=(Effect<Types...> &&) {
    PROFILE

    return *this;
}

template<typename... Types>
inline Effect<Types...>& Effect<Types...>::operator=(Effect<Types...> const&) {
    PROFILE

    return *this;
}

template<typename... Types>
inline bool Effect<Types...>::operator()(Types const&... args) {
    PROFILE

    auto deps = std::forward_as_tuple(args...);

    if (_deps.has_value()) {
        if (*_deps != deps) {
            _deps = deps;
            return true;
        } else {
            return false;
        }
    } else {
        _deps = deps;
        return true;
    }
}

template<typename... Types>
inline void Effect<Types...>::reset() {
    PROFILE

    _deps = {};
}

} /* namespace Rocket */
