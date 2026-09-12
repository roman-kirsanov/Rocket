/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <functional>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

// Internal helpers for GPU snapshot (golden image) tests. Each case renders
// into an offscreen Image and is compared per-pixel against a golden PNG in
// src/Rocket/Paint/Snapshots/. Run with ROCKET_UPDATE_SNAPSHOTS set to
// (re)generate the goldens; failing cases save a `<name>.actual.png` under the
// build directory (snapshots/) for visual inspection, keeping the source tree clean.
//
// PNG IO goes through ImageIO. The stored bytes are the painter's premultiplied
// RGBA output, written and read *verbatim*: images are declared to ImageIO as
// straight-alpha (kCGImageAlphaLast) so no premultiplication conversion is ever
// applied, and goldens are read from the decoded image's data provider rather
// than through a CGBitmapContext (which only supports premultiplied drawing and
// would corrupt low-alpha pixels).

// Golden images are stored per module under tests/_Snapshots/<Module>/; each
// consumer defines its module name before including this header:
//
//   #define ROCKET_SNAPSHOT_MODULE "Paint"
//   #include "../../_Snapshots/Snapshot.hpp"
#ifndef ROCKET_SNAPSHOT_MODULE
#error "define ROCKET_SNAPSHOT_MODULE (e.g. \"Paint\") before including Snapshot.hpp"
#endif

inline auto const _SNAPSHOT_DIR        = std::string(ROCKET_SOURCE_DIR "/tests/_Snapshots/" ROCKET_SNAPSHOT_MODULE "/");
inline auto const _SNAPSHOT_ACTUAL_DIR = std::string(ROCKET_BINARY_DIR "/snapshots/" ROCKET_SNAPSHOT_MODULE "/");

// Snapshot goldens must stay portable across GPU drivers. Two kinds of benign
// cross-renderer differences occur: smooth ~1-2/255 rounding almost everywhere,
// and hard flips on a thin band of edge pixels where nearest-filtered image/shape
// edges land on a different texel between drivers. So a pixel counts as "differing"
// only when a channel exceeds the rounding tolerance, and a case fails only when
// the differing fraction exceeds the budget (which absorbs edge bands but still
// catches a real, widespread rendering change).
inline int    constexpr _SNAPSHOT_CHANNEL_TOLERANCE = 2;
inline double constexpr _SNAPSHOT_MAX_DIFF_PERCENT   = 2.0;

inline int _snapshotCasesTotal  = 0;
inline int _snapshotCasesPassed = 0;

/**
 * Saves tightly-packed RGBA8 pixels as a PNG file (bytes stored verbatim).
 *
 * @param path   Destination file path.
 * @param width  Image width in pixels.
 * @param height Image height in pixels.
 * @param pixels Tightly-packed RGBA8 pixel bytes, row-major.
 */
inline bool _SaveSnapshotPixels(std::string const& path, int width, int height, std::vector<std::uint8_t> const& pixels) {
    auto provider = ::CGDataProviderCreateWithData(nullptr, pixels.data(), pixels.size(), nullptr);
    auto colorSpace = ::CGColorSpaceCreateDeviceRGB();
    auto image = ::CGImageCreate(
        width, height, 8, 32, ((std::size_t)width * 4), colorSpace,
        ((CGBitmapInfo)kCGImageAlphaLast | (CGBitmapInfo)kCGBitmapByteOrderDefault),
        provider, nullptr, false, kCGRenderingIntentDefault
    );

    ::CGColorSpaceRelease(colorSpace);
    ::CGDataProviderRelease(provider);

    if (image == nullptr) {
        std::printf("  failed to wrap pixels for %s\n", path.c_str());
        return false;
    }

    auto url = ::CFURLCreateFromFileSystemRepresentation(nullptr, (UInt8 const*)path.c_str(), (CFIndex)path.size(), false);
    auto destination = (url != nullptr) ? ::CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr) : nullptr;

    if (url != nullptr) {
        ::CFRelease(url);
    }

    if (destination == nullptr) {
        ::CGImageRelease(image);
        std::printf("  failed to open %s for writing\n", path.c_str());
        return false;
    }

    ::CGImageDestinationAddImage(destination, image, nullptr);

    auto const saved = ::CGImageDestinationFinalize(destination);

    ::CFRelease(destination);
    ::CGImageRelease(image);

    if (saved == false) {
        std::printf("  failed to save %s\n", path.c_str());
        return false;
    }

    return true;
}

/**
 * Loads a PNG's decoded RGBA8 bytes verbatim (no alpha conversion);
 * returns false with a printed reason on failure.
 *
 * @param path   Path to the PNG file to load.
 * @param width  Receives the image width in pixels.
 * @param height Receives the image height in pixels.
 * @param pixels Receives the tightly-packed RGBA8 pixel bytes.
 */
inline bool _LoadSnapshotPixels(std::string const& path, int& width, int& height, std::vector<std::uint8_t>& pixels) {
    auto url = ::CFURLCreateFromFileSystemRepresentation(nullptr, (UInt8 const*)path.c_str(), (CFIndex)path.size(), false);
    auto source = (url != nullptr) ? ::CGImageSourceCreateWithURL(url, nullptr) : nullptr;

    if (url != nullptr) {
        ::CFRelease(url);
    }

    if (source == nullptr) {
        std::printf("  missing golden %s\n", path.c_str());
        return false;
    }

    auto image = ::CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    ::CFRelease(source);

    if (image == nullptr) {
        std::printf("  failed to decode golden %s\n", path.c_str());
        return false;
    }

    width  = (int)::CGImageGetWidth(image);
    height = (int)::CGImageGetHeight(image);

    auto const bitsPerPixel = ::CGImageGetBitsPerPixel(image);
    auto const alphaInfo = ::CGImageGetAlphaInfo(image);

    if ((bitsPerPixel != 32) || (alphaInfo != kCGImageAlphaLast)) {
        ::CGImageRelease(image);
        std::printf("  golden %s has unexpected format (bpp %zu, alpha %d)\n", path.c_str(), bitsPerPixel, (int)alphaInfo);
        return false;
    }

    auto const bytesPerRow = ::CGImageGetBytesPerRow(image);
    auto data = ::CGDataProviderCopyData(::CGImageGetDataProvider(image));
    ::CGImageRelease(image);

    if (data == nullptr) {
        std::printf("  failed to read golden %s\n", path.c_str());
        return false;
    }

    auto const bytes = ::CFDataGetBytePtr(data);

    pixels.resize((std::size_t)width * height * 4);

    for (auto y = 0; y < height; y++) {
        std::memcpy(
            pixels.data() + ((std::size_t)y * width * 4),
            bytes + ((std::size_t)y * bytesPerRow),
            ((std::size_t)width * 4)
        );
    }

    ::CFRelease(data);

    return true;
}

/**
 * Saves a failing case's actual pixels to the build directory (created
 * on demand).
 *
 * @param name   Case name; the file is written as `<name>.actual.png`.
 * @param width  Image width in pixels.
 * @param height Image height in pixels.
 * @param pixels Tightly-packed RGBA8 pixel bytes, row-major.
 */
inline bool _SaveSnapshotActual(std::string const& name, int width, int height, std::vector<std::uint8_t> const& pixels) {
    auto error = std::error_code();
    std::filesystem::create_directories(_SNAPSHOT_ACTUAL_DIR, error);

    return _SaveSnapshotPixels(_SNAPSHOT_ACTUAL_DIR + name + ".actual.png", width, height, pixels);
}

/**
 * Renders a case into an offscreen Image, then either updates the golden
 * PNG (when ROCKET_UPDATE_SNAPSHOTS is set) or compares the rendered pixels
 * against it with a small per-channel tolerance.
 *
 * @param name   Case name, used for the golden file path and log output.
 * @param width  Offscreen target width in pixels.
 * @param height Offscreen target height in pixels.
 * @param render Callback that paints the case into the given Painter and target Image.
 * @return True when the case passes (or the golden was updated), false on any failure.
 */
inline bool _RunSnapshotCase(std::string const& name, int width, int height, std::function<void(Painter&, Image&)> const& render) {
    _snapshotCasesTotal++;

    auto target  = Image(Vec2((float)width, (float)height));
    auto painter = Painter();

    painter.beginPaint(ImagePaintTarget{
        .image = target,
        .clearColor = Vec4{ 0.1f, 0.1f, 0.1f, 1.0f }
    });
    render(painter, target);
    painter.endPaint();

    auto pixels = std::vector<std::uint8_t>();
    target.getData(pixels);

    auto const goldenPath = _SNAPSHOT_DIR + name + ".png";

    if (std::getenv("ROCKET_UPDATE_SNAPSHOTS") != nullptr) {
        if (_SaveSnapshotPixels(goldenPath, width, height, pixels)) {
            std::printf("UPDATED %s\n", name.c_str());
            _snapshotCasesPassed++;
            return true;
        }
        std::printf("FAILED %s: could not write golden\n", name.c_str());
        return false;
    }

    auto goldenWidth = 0;
    auto goldenHeight = 0;
    auto golden = std::vector<std::uint8_t>();

    if (_LoadSnapshotPixels(goldenPath, goldenWidth, goldenHeight, golden) == false) {
        std::printf("FAILED %s: could not load golden\n", name.c_str());
        _SaveSnapshotActual(name, width, height, pixels);
        return false;
    }

    if ((goldenWidth != width) || (goldenHeight != height)) {
        std::printf("FAILED %s: golden is %dx%d, actual is %dx%d\n", name.c_str(), goldenWidth, goldenHeight, width, height);
        _SaveSnapshotActual(name, width, height, pixels);
        return false;
    }

    auto maxDiff = 0;
    auto diffCount = 0;

    for (int y = 0; y < height; y++) {
        auto goldenRow = golden.data() + ((std::size_t)y * width * 4);
        auto actualRow = pixels.data() + ((std::size_t)y * width * 4);

        for (int x = 0; x < width; x++) {
            auto pixelDiff = 0;

            for (int c = 0; c < 4; c++) {
                auto const diff = std::abs((int)goldenRow[(x * 4) + c] - (int)actualRow[(x * 4) + c]);
                pixelDiff = std::max(pixelDiff, diff);
            }

            maxDiff = std::max(maxDiff, pixelDiff);
            if (pixelDiff > _SNAPSHOT_CHANNEL_TOLERANCE) {
                diffCount++;
            }
        }
    }

    auto const diffPercent = ((double)diffCount * 100.0) / ((double)width * (double)height);

    if (diffPercent > _SNAPSHOT_MAX_DIFF_PERCENT) {
        std::printf("FAILED %s: max channel diff %d, %.3f%% pixels differ\n", name.c_str(), maxDiff, diffPercent);
        _SaveSnapshotActual(name, width, height, pixels);
        return false;
    }

    _snapshotCasesPassed++;
    return true;
}

/** Builds a deterministic 32x32 checkerboard (8px white/red cells, 1px blue border). */
inline std::vector<std::uint8_t> _MakeCheckerboardData() {
    auto data = std::vector<std::uint8_t>(32 * 32 * 4);

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            auto const i = (std::size_t)((y * 32) + x) * 4;
            auto const border = (x == 0) || (y == 0) || (x == 31) || (y == 31);
            auto const white  = ((((x / 8) + (y / 8)) % 2) == 0);

            if (border) {
                data[i + 0] = 0;   data[i + 1] = 0;   data[i + 2] = 255;
            } else if (white) {
                data[i + 0] = 255; data[i + 1] = 255; data[i + 2] = 255;
            } else {
                data[i + 0] = 255; data[i + 1] = 0;   data[i + 2] = 0;
            }
            data[i + 3] = 255;
        }
    }

    return data;
}

/** Builds a deterministic 48x48 image with distinct colored border bands and center. */
inline std::vector<std::uint8_t> _MakeNPatchData() {
    auto data = std::vector<std::uint8_t>(48 * 48 * 4);

    for (int y = 0; y < 48; y++) {
        for (int x = 0; x < 48; x++) {
            auto const i = (std::size_t)((y * 48) + x) * 4;

            if (y < 12) {
                data[i + 0] = 0;   data[i + 1] = 200; data[i + 2] = 0;    /* top: green */
            } else if (y >= 36) {
                data[i + 0] = 255; data[i + 1] = 220; data[i + 2] = 0;    /* bottom: yellow */
            } else if (x < 12) {
                data[i + 0] = 220; data[i + 1] = 0;   data[i + 2] = 0;    /* left: red */
            } else if (x >= 36) {
                data[i + 0] = 0;   data[i + 1] = 80;  data[i + 2] = 255;  /* right: blue */
            } else {
                data[i + 0] = 200; data[i + 1] = 0;   data[i + 2] = 200;  /* center: magenta */
            }
            data[i + 3] = 255;
        }
    }

    return data;
}

/** Prints the pass/fail summary and returns the process exit code. */
} /* namespace Rocket */
