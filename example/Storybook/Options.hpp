/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

namespace Storybook {

/**
 * Returns the names of the options, in order, for a Select or a
 * SegmentedControl.
 *
 * @param options Each value with its name.
 */
template <typename Type>
std::vector<std::string> GetOptionNames(std::vector<std::tuple<Type, std::string>> const& options);

/**
 * Returns the position of a value among the options, or their count when it
 * is not one of them.
 *
 * @param options Each value with its name.
 * @param value   The value to find.
 */
template <typename Type>
std::size_t GetOptionIndex(std::vector<std::tuple<Type, std::string>> const& options, Type const& value);

/**
 * Returns the name of a value, which must be one of the options.
 *
 * @param options Each value with its name.
 * @param value   The value to name.
 */
template <typename Type>
std::string const& GetOptionName(std::vector<std::tuple<Type, std::string>> const& options, Type const& value);

template <typename Type>
inline std::vector<std::string> GetOptionNames(std::vector<std::tuple<Type, std::string>> const& options) {
    auto names = std::vector<std::string>();

    for (auto const& [value, name] : options) {
        names.push_back(name);
    }

    return names;
}

template <typename Type>
inline std::size_t GetOptionIndex(std::vector<std::tuple<Type, std::string>> const& options, Type const& value) {
    return static_cast<std::size_t>(
        std::ranges::find(options, value, [](auto const& option) { return std::get<0>(option); }) - options.begin()
    );
}

template <typename Type>
inline std::string const& GetOptionName(std::vector<std::tuple<Type, std::string>> const& options, Type const& value) {
    return std::get<1>(options[GetOptionIndex(options, value)]);
}

} /* namespace Storybook */
