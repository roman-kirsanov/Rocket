/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <Rocket/Base/Profile.hpp>

namespace Rocket {

template<typename... Arguments>
class Sub;

/**
 * Event source that broadcasts to all active Sub instances.
 *
 * Non-copyable and non-movable; own it by value as a member of the publishing
 * object. Automatically unlinks all subscribers in its destructor.
 *
 * @tparam Arguments The argument types forwarded to each subscriber's handler.
 */
template<typename... Arguments>
class Pub {
public:
    ~Pub();
    Pub();

    Pub(Pub<Arguments...> &&) = delete;
    Pub(Pub<Arguments...> const&) = delete;
    Pub<Arguments...>& operator=(Pub<Arguments...> &&) = delete;
    Pub<Arguments...>& operator=(Pub<Arguments...> const&) = delete;

    /**
     * Calls every subscribed handler with the given arguments.
     *
     * @param args Arguments forwarded to each handler.
     */
    void publish(Arguments &&... args);

    /**
     * Returns true if at least one Sub is currently subscribed.
     */
    bool hasSubs() const;
private:
    mutable std::unordered_map<
        void*,
        std::tuple<
            std::function<void()>,
            std::function<void(Arguments...)>
        >
    > _subs;

    template<typename... _>
    friend class Sub;
};

/**
 * Event sink that subscribes to one or more Pub instances.
 *
 * Non-copyable and non-movable; own it by value inside the subscribing object.
 * Automatically unsubscribes from all publishers in its destructor.
 *
 * @tparam Arguments The argument types expected by subscribed handlers.
 */
template<typename... Arguments>
class Sub {
public:
    ~Sub();
    Sub();

    /**
     * A subscription to pub, already active with fn as the handler.
     *
     * @param pub The publisher to subscribe to.
     * @param fn  The handler called when pub publishes.
     */
    template<typename Function>
    Sub(Pub<Arguments...> const& pub, Function && fn);

    Sub(Sub<Arguments...> &&) = delete;
    Sub(Sub<Arguments...> const&) = delete;
    Sub<Arguments...>& operator=(Sub<Arguments...> &&) = delete;
    Sub<Arguments...>& operator=(Sub<Arguments...> const&) = delete;

    /**
     * Subscribes to pub with the handler fn. If this Sub is already subscribed to pub, the
     * existing handler stays active and fn has no effect.
     *
     * @param pub The publisher to subscribe to.
     * @param fn  The handler called when pub publishes.
     */
    void on(Pub<Arguments...> const& pub, std::function<void(Arguments &&...)> const& fn) const;

    /**
     * Unsubscribes from pub.
     *
     * @param pub The publisher to unsubscribe from.
     */
    void off(Pub<Arguments...> const& pub) const;

    /**
     * Unsubscribes from all currently subscribed publishers.
     */
    void reset();
private:
    mutable std::unordered_map<void*, std::function<void()>> _pubs;
};

template<typename... Arguments>
inline Pub<Arguments...>::Pub() : _subs() {}

template<typename... Arguments>
inline Pub<Arguments...>::~Pub() {
    PROFILE

    for (auto& [ _, tuple ] : _subs) {
        auto& [ unlink, __ ] = tuple;
        unlink();
    }
}

template<typename... Arguments>
inline void Pub<Arguments...>::publish(Arguments &&... args) {
    PROFILE

    for (auto& [ _, tuple ] : _subs) {
        auto& [ __, emit ] = tuple;
        emit(std::forward<Arguments>(args)...);
    }
}

template<typename... Arguments>
inline bool Pub<Arguments...>::hasSubs() const {
    PROFILE

    return (_subs.empty() == false);
}

template<typename... Arguments>
inline Sub<Arguments...>::~Sub() {
    PROFILE

    for (auto& [ _, unlink ] : _pubs) {
        unlink();
    }
}

template<typename... Arguments>
inline Sub<Arguments...>::Sub() : _pubs() {}

template<typename... Arguments>
template<typename Function>
inline Sub<Arguments...>::Sub(Pub<Arguments...> const& pub, Function && fn) : _pubs() {
    PROFILE

    on(pub, fn);
}

template<typename... Arguments>
inline void Sub<Arguments...>::on(Pub<Arguments...> const& pub, std::function<void(Arguments &&...)> const& fn) const {
    PROFILE

    auto pubPtr = &pub;
    auto pubKey = (void*)&pub;
    auto subKey = (void*)this;

    _pubs.erase(pubKey);
    _pubs.insert({
        pubKey,
        [pubPtr, subKey]() {
            pubPtr->_subs.erase(subKey);
        }
    });

    pub._subs.insert({
        subKey, {
        [pubKey, this]() {
            _pubs.erase(pubKey);
        },
        fn
    }});
}

template<typename... Arguments>
inline void Sub<Arguments...>::off(Pub<Arguments...> const& pub) const {
    PROFILE

    auto pubKey = (void*)&pub;
    auto subKey = (void*)this;

    _pubs.erase(pubKey);
    pub._subs.erase(subKey);
}

template<typename... Arguments>
inline void Sub<Arguments...>::reset() {
    PROFILE

    for (auto& [ _, unlink ] : _pubs) {
        unlink();
    }

    _pubs.clear();
}

} /* namespace Rocket */
