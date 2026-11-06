/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>

namespace Storybook {

/** Props for the ModalPage component. */
struct ModalPageProps {
    /** Reconciliation key. */
    std::string key;
};

/**
 * Renders the documentation page of Modal: a playground for its props and the
 * code it makes.
 *
 * @param props Desired page configuration for this render.
 */
void ModalPage(ModalPageProps const& props);

} /* namespace Storybook */
