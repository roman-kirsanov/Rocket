/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace Rocket {
namespace App {

/** Initializes SDL's video subsystem, with the hints the backend relies on; safe to call any number of times. */
void __InitVideo();

} /* namespace App */
} /* namespace Rocket */
