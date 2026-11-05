/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <tuple>
#include <vector>

namespace Rocket {

template <typename Type>
void _Cascade(std::optional<Type>& target, std::optional<Type> const& source);

template <typename Rule>
std::vector<Rule const*> _GetCascade(
    std::vector<Rule> const& rules,
    std::function<bool(Rule const&)> const& matches,
    std::function<int(Rule const&)> const& specificity
);

template <typename Type>
inline void _Cascade(std::optional<Type>& target, std::optional<Type> const& source) {
    if (source.has_value()) {
        target = source;
    }
}

template <typename Rule>
inline std::vector<Rule const*> _GetCascade(
    std::vector<Rule> const& rules,
    std::function<bool(Rule const&)> const& matches,
    std::function<int(Rule const&)> const& specificity
) {
    auto cascade = std::vector<Rule const*>();

    for (auto const& rule : rules) {
        if (matches(rule)) {
            cascade.push_back(&rule);
        }
    }

    std::ranges::stable_sort(cascade, [&](auto const first, auto const second) {
        return (std::tuple(first->important, specificity(*first)) < std::tuple(second->important, specificity(*second)));
    });

    return cascade;
}

} /* namespace Rocket */
