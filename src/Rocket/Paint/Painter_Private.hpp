/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace Rocket {

/** Returns the process-wide default GPU device (id<MTLDevice> on macOS). */
void* __GetDefaultGPUDevice();

/** Returns the process-wide default GPU command queue (id<MTLCommandQueue> on macOS). */
void* __GetDefaultGPUCommandQueue();

/** Returns the texture format used for painter targets and images (MTLPixelFormat on macOS). */
int __GetDefaultTextureFormat();

} /* namespace Rocket */
