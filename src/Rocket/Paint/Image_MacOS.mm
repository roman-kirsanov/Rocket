/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <vector>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#import <Metal/Metal.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Painter_Private.hpp>

namespace Rocket {

struct Image::_Image {
    id<MTLTexture> texture = nil;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

static id<MTLDevice> _Device() {
    PROFILE

    return (__bridge id<MTLDevice>)__GetDefaultGPUDevice();
}

static id<MTLCommandQueue> _Queue() {
    PROFILE

    return (__bridge id<MTLCommandQueue>)__GetDefaultGPUCommandQueue();
}

static id<MTLTexture> _CreateTexture(std::uint32_t width, std::uint32_t height) {
    PROFILE

    auto descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat: (MTLPixelFormat)__GetDefaultTextureFormat()
                                     width: std::max(1u, width)
                                    height: std::max(1u, height)
                                 mipmapped: NO];

    descriptor.usage = (MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget);
    descriptor.storageMode = MTLStorageModePrivate;

    return [_Device() newTextureWithDescriptor: descriptor];
}

/** Uploads tightly-packed premultiplied RGBA8 pixels into a texture region via a staging buffer. */
static void _UploadPixels(id<MTLTexture> texture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint8_t const* pixels) {
    PROFILE

    auto const dataSize = (NSUInteger)(width * height * 4);
    auto staging = [_Device() newBufferWithBytes: pixels length: dataSize options: MTLResourceStorageModeShared];

    auto commandBuffer = [_Queue() commandBuffer];
    auto blit = [commandBuffer blitCommandEncoder];

    [blit copyFromBuffer: staging
            sourceOffset: 0
       sourceBytesPerRow: (width * 4)
     sourceBytesPerImage: dataSize
              sourceSize: MTLSizeMake(width, height, 1)
               toTexture: texture
        destinationSlice: 0
        destinationLevel: 0
       destinationOrigin: MTLOriginMake(x, y, 0)];

    [blit endEncoding];
    [commandBuffer commit];
}

/** Downloads a texture's pixels as tightly-packed RGBA8; waits for the GPU to finish. */
static void _DownloadPixels(id<MTLTexture> texture, std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t>& pixels) {
    PROFILE

    auto const dataSize = (NSUInteger)(width * height * 4);
    auto staging = [_Device() newBufferWithLength: dataSize options: MTLResourceStorageModeShared];

    auto commandBuffer = [_Queue() commandBuffer];
    auto blit = [commandBuffer blitCommandEncoder];

    [blit copyFromTexture: texture
              sourceSlice: 0
              sourceLevel: 0
             sourceOrigin: MTLOriginMake(0, 0, 0)
               sourceSize: MTLSizeMake(width, height, 1)
                 toBuffer: staging
        destinationOffset: 0
   destinationBytesPerRow: (width * 4)
 destinationBytesPerImage: dataSize];

    [blit endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    pixels.resize(dataSize);
    std::memcpy(pixels.data(), staging.contents, dataSize);
}

void Image::__init(Vec2 const& size) {
    PROFILE

    _impl = new _Image{};
    _impl->texture = _CreateTexture((std::uint32_t)size.width, (std::uint32_t)size.height);
    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
}

void Image::__init(std::string const& path) {
    PROFILE

    auto url = ::CFURLCreateFromFileSystemRepresentation(nullptr, (UInt8 const*)path.c_str(), (CFIndex)path.size(), false);
    auto source = (url != nullptr) ? ::CGImageSourceCreateWithURL(url, nullptr) : nullptr;

    if (url != nullptr) {
        ::CFRelease(url);
    }

    if (source == nullptr) {
        throw std::runtime_error("Failed to open image: " + path);
    }

    auto cgImage = ::CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    ::CFRelease(source);

    if (cgImage == nullptr) {
        throw std::runtime_error("Failed to decode image: " + path);
    }

    auto const width = (std::uint32_t)::CGImageGetWidth(cgImage);
    auto const height = (std::uint32_t)::CGImageGetHeight(cgImage);

    // Drawing into a premultiplied-RGBA bitmap context both converts the
    // source format and premultiplies alpha, which the painter expects.
    auto pixels = std::vector<std::uint8_t>((std::size_t)width * height * 4);
    auto colorSpace = ::CGColorSpaceCreateDeviceRGB();
    auto context = ::CGBitmapContextCreate(
        pixels.data(), width, height, 8, ((std::size_t)width * 4), colorSpace,
        ((CGBitmapInfo)kCGImageAlphaPremultipliedLast | (CGBitmapInfo)kCGBitmapByteOrder32Big)
    );

    ::CGColorSpaceRelease(colorSpace);

    if (context == nullptr) {
        ::CGImageRelease(cgImage);
        throw std::runtime_error("Failed to convert image: " + path);
    }

    ::CGContextDrawImage(context, ::CGRectMake(0, 0, width, height), cgImage);
    ::CGContextRelease(context);
    ::CGImageRelease(cgImage);

    __init(Vec2{ (float)width, (float)height });
    _size = { (float)width, (float)height };

    _UploadPixels(_impl->texture, 0, 0, width, height, pixels.data());
}

void Image::__done() {
    PROFILE

    if (_impl != nullptr) {
        delete _impl; // ARC releases the texture
        _impl = nullptr;
    }
}

void* Image::__getTexture() const {
    PROFILE

    return (__bridge void*)_impl->texture;
}

void Image::__resize(Vec2 const& size) {
    PROFILE

    _impl->texture = _CreateTexture((std::uint32_t)size.width, (std::uint32_t)size.height);
    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
}

void Image::__setData(Vec2 const& size, std::uint8_t const* pixels, Vec2 const& offset) {
    PROFILE

    _UploadPixels(
        _impl->texture,
        (std::uint32_t)offset.x,
        (std::uint32_t)offset.y,
        (std::uint32_t)size.width,
        (std::uint32_t)size.height,
        pixels
    );
}

void Image::__getData(std::vector<std::uint8_t>& data) const {
    PROFILE

    _DownloadPixels(_impl->texture, _impl->width, _impl->height, data);
}

void Image::__copyFrom(Image const& image, Vec4 const& slice, Vec2 const& position) {
    PROFILE

    auto commandBuffer = [_Queue() commandBuffer];
    auto blit = [commandBuffer blitCommandEncoder];

    [blit copyFromTexture: image._impl->texture
              sourceSlice: 0
              sourceLevel: 0
             sourceOrigin: MTLOriginMake((NSUInteger)slice.x, (NSUInteger)slice.y, 0)
               sourceSize: MTLSizeMake((NSUInteger)slice.width, (NSUInteger)slice.height, 1)
                toTexture: _impl->texture
         destinationSlice: 0
         destinationLevel: 0
        destinationOrigin: MTLOriginMake((NSUInteger)position.x, (NSUInteger)position.y, 0)];

    [blit endEncoding];
    [commandBuffer commit];
}

} /* namespace Rocket */
