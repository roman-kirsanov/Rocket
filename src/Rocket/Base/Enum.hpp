/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <variant>
#include <Rocket/Base/Profile.hpp>

namespace Rocket {

/**
 * A type-safe variant with ergonomic access and exhaustive pattern matching.
 *
 * Wraps std::variant and forwards its constructors and assignment operators,
 * adding named helpers for type querying and visitor dispatch.
 *
 * @tparam Types The set of alternative types this Enum can hold.
 */
template<typename... Types>
class Enum : public std::variant<Types...> {
public:
    using std::variant<Types...>::variant;
    using std::variant<Types...>::operator=;

    /**
     * Returns a const pointer to the stored value if the active alternative is Type.
     *
     * @tparam Type The alternative type to access.
     * @return Const pointer to the value, or nullptr if a different alternative is active.
     */
    template<typename Type>
    Type const* as() const;

    /**
     * Returns a mutable pointer to the stored value if the active alternative is Type.
     *
     * @tparam Type The alternative type to access.
     * @return Mutable pointer to the value, or nullptr if a different alternative is active.
     */
    template<typename Type>
    Type* as();

    /**
     * Returns true if the active alternative is Type.
     *
     * @tparam Type The alternative type to check.
     */
    template<typename Type>
    bool is() const;

    /**
     * Dispatches to the matching visitor callable using an overload set.
     *
     * Each visitor must handle at least one alternative; together they must
     * cover all alternatives exhaustively or the call will not compile.
     *
     * @tparam Visitors Callable types forming the overload set.
     * @param visitors  Callables to dispatch to.
     */
    template<typename... Visitors>
    decltype(auto) match(Visitors &&... visitors);

    /**
     * Dispatches to the matching visitor callable using an overload set.
     *
     * Each visitor must handle at least one alternative; together they must
     * cover all alternatives exhaustively or the call will not compile.
     *
     * @tparam Visitors Callable types forming the overload set.
     * @param visitors  Callables to dispatch to.
     */
    template<typename... Visitors>
    decltype(auto) match(Visitors &&... visitors) const;
};

template<typename... Types>
template<typename Type>
inline Type const* Enum<Types...>::as() const {
    PROFILE

    return std::get_if<Type>(this);
}

template<typename... Types>
template<typename Type>
inline Type* Enum<Types...>::as() {
    PROFILE

    return std::get_if<Type>(this);
}

template<typename... Types>
template<typename Type>
inline bool Enum<Types...>::is() const {
    PROFILE

    return std::holds_alternative<Type>(*this);
}

template<typename... Types>
template<typename... Visitors>
inline decltype(auto) Enum<Types...>::match(Visitors &&... visitors) {
    PROFILE

    struct _Overloaded : std::decay_t<Visitors>... { using std::decay_t<Visitors>::operator()...; };
    return std::visit(_Overloaded{std::forward<Visitors>(visitors)...}, *this);
}

template<typename... Types>
template<typename... Visitors>
inline decltype(auto) Enum<Types...>::match(Visitors &&... visitors) const {
    PROFILE

    struct _Overloaded : std::decay_t<Visitors>... { using std::decay_t<Visitors>::operator()...; };
    return std::visit(_Overloaded{std::forward<Visitors>(visitors)...}, *this);
}

} /* namespace Rocket */
