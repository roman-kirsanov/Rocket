/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>

namespace Storybook {

/** Props for the SpinnerPage component. */
struct SpinnerPageProps {
    /** Reconciliation key. */
    std::string key;
};

/**
 * Renders the documentation page of Spinner: a playground for its props and the
 * code it makes.
 *
 * @param props Desired page configuration for this render.
 */
void SpinnerPage(SpinnerPageProps const& props);

} /* namespace Storybook */
