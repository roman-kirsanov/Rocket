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
#import <Metal/Metal.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Painter_Private.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

struct Image::_Image {
    id<MTLTexture> texture = nil;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<Vec4> pendingRegions;
    std::vector<std::uint8_t> pendingPixels;
};

static id<MTLDevice> _GetDevice() {
    PROFILE

    return (__bridge id<MTLDevice>)__GetDefaultGPUDevice();
}

static id<MTLCommandQueue> _GetQueue() {
    PROFILE

    return (__bridge id<MTLCommandQueue>)__GetDefaultGPUCommandQueue();
}

static id<MTLTexture> _CreateTexture(std::uint32_t width, std::uint32_t height) {
    PROFILE

    auto descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat: (::MTLPixelFormat)__GetDefaultTextureFormat()
                                     width: std::max(1u, width)
                                    height: std::max(1u, height)
                                 mipmapped: NO];

    descriptor.usage = (::MTLTextureUsageShaderRead | ::MTLTextureUsageRenderTarget);
    descriptor.storageMode = ::MTLStorageModePrivate;

    auto texture = [_GetDevice() newTextureWithDescriptor: descriptor];

    if (texture == nil) {
        throw std::runtime_error("Failed to create a GPU texture");
    }

    return texture;
}

static void _DownloadPixels(id<MTLTexture> texture, std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t>& pixels) {
    PROFILE

    auto const dataSize = (NSUInteger)(width * height * 4);
    auto staging = [_GetDevice() newBufferWithLength: dataSize options: ::MTLResourceStorageModeShared];
    auto commandBuffer = [_GetQueue() commandBuffer];
    auto commandEncoder = [commandBuffer blitCommandEncoder];

    [commandEncoder copyFromTexture: texture
              sourceSlice: 0
              sourceLevel: 0
             sourceOrigin: ::MTLOriginMake(0, 0, 0)
               sourceSize: ::MTLSizeMake(width, height, 1)
                 toBuffer: staging
        destinationOffset: 0
   destinationBytesPerRow: (width * 4)
 destinationBytesPerImage: dataSize];

    [commandEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    pixels.resize(dataSize);
    std::memcpy(pixels.data(), staging.contents, dataSize);
}

static void _UploadPixels(id<MTLTexture> texture, std::vector<Vec4> const& regions, std::uint8_t const* pixels) {
    PROFILE

    auto dataSize = (NSUInteger)0;

    for (auto const& region : regions) {
        dataSize += ((NSUInteger)region.width * (NSUInteger)region.height * 4);
    }

    auto staging = [_GetDevice() newBufferWithBytes: pixels length: dataSize options: ::MTLResourceStorageModeShared];
    auto commandBuffer = [_GetQueue() commandBuffer];
    auto commandEncoder = [commandBuffer blitCommandEncoder];
    auto sourceOffset = (NSUInteger)0;

    for (auto const& region : regions) {
        auto const width = (NSUInteger)region.width;
        auto const height = (NSUInteger)region.height;

        [commandEncoder copyFromBuffer: staging
                sourceOffset: sourceOffset
           sourceBytesPerRow: (width * 4)
         sourceBytesPerImage: (width * height * 4)
                  sourceSize: ::MTLSizeMake(width, height, 1)
                   toTexture: texture
            destinationSlice: 0
            destinationLevel: 0
           destinationOrigin: ::MTLOriginMake((NSUInteger)region.x, (NSUInteger)region.y, 0)];

        sourceOffset += (width * height * 4);
    }

    [commandEncoder endEncoding];
    [commandBuffer commit];
}

static void _UploadPending(id<MTLTexture> texture, std::vector<Vec4>& regions, std::vector<std::uint8_t>& pixels) {
    PROFILE

    _UploadPixels(texture, regions, pixels.data());

    regions.clear();
    pixels = std::vector<std::uint8_t>();
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

    auto url = ::CFURLCreateFromFileSystemRepresentation(nullptr, (::UInt8 const*)path.c_str(), (::CFIndex)path.size(), false);
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
        ((::CGBitmapInfo)::kCGImageAlphaPremultipliedLast | (::CGBitmapInfo)::kCGBitmapByteOrder32Big)
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

    __setData(_size, pixels.data(), { 0.0f, 0.0f }, ImageUpload::Immediate);
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

    if (_impl->pendingRegions.empty() == false) {
        _UploadPending(_impl->texture, _impl->pendingRegions, _impl->pendingPixels);
    }

    return (__bridge void*)_impl->texture;
}

void Image::__resize(Vec2 const& size) {
    PROFILE

    _impl->pendingRegions.clear();
    _impl->pendingPixels.clear();
    _impl->texture = _CreateTexture((std::uint32_t)size.width, (std::uint32_t)size.height);
    _impl->width = std::max(1u, (std::uint32_t)size.width);
    _impl->height = std::max(1u, (std::uint32_t)size.height);
}

void Image::__setData(Vec2 const& size, std::uint8_t const* pixels, Vec2 const& offset, ImageUpload upload) {
    PROFILE

    auto const region = Vec4{ offset.x, offset.y, (float)(std::uint32_t)size.width, (float)(std::uint32_t)size.height };
    auto const byteCount = ((std::size_t)region.width * (std::size_t)region.height * 4);

    if (byteCount == 0) {
        return;
    }

    if (upload == ImageUpload::Queued) {
        _impl->pendingRegions.push_back(region);
        _impl->pendingPixels.insert(_impl->pendingPixels.end(), pixels, (pixels + byteCount));
        return;
    }

    if (_impl->pendingRegions.empty() == false) {
        _UploadPending(_impl->texture, _impl->pendingRegions, _impl->pendingPixels);
    }

    _UploadPixels(_impl->texture, { region }, pixels);
}

void Image::__getData(std::vector<std::uint8_t>& data) const {
    PROFILE

    if (_impl->pendingRegions.empty() == false) {
        _UploadPending(_impl->texture, _impl->pendingRegions, _impl->pendingPixels);
    }
    _DownloadPixels(_impl->texture, _impl->width, _impl->height, data);
}

void Image::__copyFrom(Image const& image, Vec4 const& slice, Vec2 const& position) {
    PROFILE

    if (image._impl->pendingRegions.empty() == false) {
        _UploadPending(image._impl->texture, image._impl->pendingRegions, image._impl->pendingPixels);
    }
    if (_impl->pendingRegions.empty() == false) {
        _UploadPending(_impl->texture, _impl->pendingRegions, _impl->pendingPixels);
    }

    auto const width = (std::uint32_t)slice.width;
    auto const height = (std::uint32_t)slice.height;

    if (width == 0 || height == 0) {
        return;
    }

    auto commandBuffer = [_GetQueue() commandBuffer];
    auto commandEncoder = [commandBuffer blitCommandEncoder];

    [commandEncoder copyFromTexture: image._impl->texture
              sourceSlice: 0
              sourceLevel: 0
             sourceOrigin: ::MTLOriginMake((NSUInteger)slice.x, (NSUInteger)slice.y, 0)
               sourceSize: ::MTLSizeMake(width, height, 1)
                toTexture: _impl->texture
         destinationSlice: 0
         destinationLevel: 0
        destinationOrigin: ::MTLOriginMake((NSUInteger)position.x, (NSUInteger)position.y, 0)];

    [commandEncoder endEncoding];
    [commandBuffer commit];
}

} /* namespace Rocket */
