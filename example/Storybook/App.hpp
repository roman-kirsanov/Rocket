/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <functional>
#include <string>

namespace Storybook {

/** Props for the App component. */
struct AppProps {
    /** Reconciliation key. */
    std::string key;

    /** Invoked when the Storybook window is closed. */
    std::function<void()> onClose;
};

/**
 * Renders the Storybook window: a sidebar listing the pages, and the page
 * picked in it.
 *
 * @param props Desired Storybook configuration for this render.
 */
void App(AppProps const& props);

} /* namespace Storybook */
