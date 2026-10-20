/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace Rocket {

/** Returns the process-wide default GPU device (id<MTLDevice> on macOS, SDL_GPUDevice* on SDL3, ID3D12Device* on Win32). */
void* __GetDefaultGPUDevice();

/** Returns the process-wide default GPU command queue (id<MTLCommandQueue>); macOS only, as SDL_GPU has no queue object and the Win32 painter keeps its own. */
void* __GetDefaultGPUCommandQueue();

/** Returns the texture format used for painter targets and images (MTLPixelFormat on macOS, SDL_GPUTextureFormat on SDL3, DXGI_FORMAT on Win32). */
int __GetDefaultTextureFormat();

} /* namespace Rocket */
