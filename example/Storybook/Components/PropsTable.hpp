/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>
#include <Rocket/UI/Node.hpp>

namespace Storybook {

/** One documented prop of a PropsTable. */
struct PropsTableRow {
    /** Name of the prop. */
    std::string name;

    /** Type of the prop, as written in C++. */
    std::string type;

    /** Value used when the prop is unset; empty when there is none. */
    std::string defaultValue;

    /** What the prop does. */
    std::string description;
};

/** Props for the PropsTable component. */
struct PropsTableProps {
    /** Reconciliation key. */
    std::string key;

    /** The documented props, top to bottom (default none). */
    std::optional<std::vector<PropsTableRow>> rows;

    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    Rocket::NodeProps nodeProps;
};

/**
 * Renders a table of props: name, type, default and description.
 *
 * @param props Desired table configuration for this render.
 */
void PropsTable(PropsTableProps const& props);

} /* namespace Storybook */
