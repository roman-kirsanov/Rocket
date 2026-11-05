/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>

namespace Storybook {

/** Props for the SwitchPage component. */
struct SwitchPageProps {
    /** Reconciliation key. */
    std::string key;
};

/**
 * Renders the documentation page of Switch: a playground for its props, the
 * code it makes, a section on each group of props, and its theme.
 *
 * @param props Desired page configuration for this render.
 */
void SwitchPage(SwitchPageProps const& props);

} /* namespace Storybook */
