/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace Rocket {

void* __GetDefaultGPUDevice();
void* __GetDefaultGPUCommandQueue();
int __GetDefaultTextureFormat();

} /* namespace Rocket */
