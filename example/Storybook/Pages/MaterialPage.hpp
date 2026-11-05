/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>

namespace Storybook {

/** Props for the MaterialPage component. */
struct MaterialPageProps {
    /** Reconciliation key. */
    std::string key;
};

/**
 * Renders the documentation page of Material: a playground for every field
 * of MaterialVariantOptions, the code it makes, and a section on each field.
 *
 * @param props Desired page configuration for this render.
 */
void MaterialPage(MaterialPageProps const& props);

} /* namespace Storybook */
