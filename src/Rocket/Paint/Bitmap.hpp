/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/**
 * Layout of the pixels of a bitmap. A bitmap is a buffer of 8-bit channels,
 * with straight (not premultiplied) alpha, laid out row by row. Pixels within
 * a row are tightly packed; how far apart the rows are is given by the
 * bitmap's bytes per row.
 *
 * Bytes per row is the distance in bytes from the start of one row to the
 * start of the next. When it is 0, it is taken from the format and the
 * width: the width times the bytes per pixel, with no gap between rows. It
 * is larger when rows are padded, or when the
 * data points into a larger bitmap: then it is that bitmap's bytes per row,
 * and the data pointer is the first pixel of the region. It is negative when
 * rows are stored bottom row first: the data pointer is then still the top
 * row, which is the last in memory.
 */
enum class BitmapFormat {
    /** One luminance channel per pixel, from black (0) to white (255); fully opaque. */
    Grayscale,
    /** Red, green, blue and alpha. */
    RGBA,
    /** Blue, green, red and alpha. */
    BGRA,
    /** Red, green and blue; fully opaque. */
    RGB,
    /** Blue, green and red; fully opaque. */
    BGR
};

/** Sampling filter used when a blitted bitmap is scaled. */
enum class BitmapFilter {
    /** Takes the nearest source pixel. */
    Nearest,
    /** Interpolates between the four nearest source pixels. */
    Linear
};

/**
 * How a blitted pixel is combined with the destination pixel under it. Every
 * mode but Replace is weighted by the source alpha, so a transparent source
 * pixel leaves the destination as it is.
 */
enum class BitmapBlend {
    /** Writes the source over the destination, alpha included. */
    Replace,
    /** Draws the source over the destination (source-over). */
    Normal,
    /** Multiplies the colors; the result is never lighter than either. */
    Multiply,
    /** Inverts, multiplies and inverts again; the result is never darker than either. */
    Screen,
    /** Multiply where the destination is dark, Screen where it is light. */
    Overlay,
    /** Adds the colors, clamped to white. */
    Add,
    /** Keeps the darker of the two colors, per channel. */
    Darken,
    /** Keeps the lighter of the two colors, per channel. */
    Lighten
};

/** Alignment of a blitted bitmap within its destination rectangle, along one axis. */
enum class BitmapPosition {
    /** Aligned to the left or top edge, at its own size. */
    Start,
    /** Centered, at its own size. */
    Center,
    /** Scaled to fill the rectangle. */
    Stretch,
    /** Aligned to the right or bottom edge, at its own size. */
    End
};

/**
 * Describes one blit: the destination and source bitmaps, and how the
 * source is sliced, placed, scaled, tiled and tinted in the destination.
 * Every field that is not optional is required; an unset optional field
 * takes the default given with it.
 */
struct BitmapBlitOptions {
    /** Pixels of the destination bitmap, written by the blit. */
    std::uint8_t* dstData;
    /** Pixel format of the destination bitmap. */
    BitmapFormat dstFormat;
    /** Dimensions of the destination bitmap in pixels. */
    Vec2 dstSize;
    /** Distance in bytes between the starts of two rows of the destination, or 0 to take it from the format and the width; see BitmapFormat. */
    std::ptrdiff_t dstBytesPerRow;
    /** Region of the destination to blit into (x, y, width, height); the part outside the bitmap is ignored. */
    Vec4 dstRect;
    /** Pixels of the source bitmap. */
    std::uint8_t const* srcData;
    /** Pixel format of the source bitmap. */
    BitmapFormat srcFormat;
    /** Dimensions of the source bitmap in pixels. */
    Vec2 srcSize;
    /** Distance in bytes between the starts of two rows of the source, or 0 to take it from the format and the width; see BitmapFormat. */
    std::ptrdiff_t srcBytesPerRow;
    /** Region of the source to blit (x, y, width, height). Default: the whole source. */
    std::optional<Vec4> srcRect;
    /** 9-patch inset (left, top, right, bottom) of the source region whose borders keep their size while the center stretches. Default: none. */
    std::optional<Vec4> nPatch;
    /** RGBA color that replaces the source RGB; its alpha is multiplied with the source alpha. Ignored when its alpha is 0. Default: none. */
    std::optional<Vec4> color;
    /** How the source is combined with the destination. Default: Normal. */
    std::optional<BitmapBlend> blend;
    /** Sampling filter used when the source is scaled. Default: Nearest. */
    std::optional<BitmapFilter> filter;
    /** Horizontal alignment of the source within the destination region. Default: Stretch. */
    std::optional<BitmapPosition> positionX;
    /** Vertical alignment of the source within the destination region. Default: Stretch. */
    std::optional<BitmapPosition> positionY;
    /** Horizontal shift in pixels of the positioned source; positive moves it right. On a repeated axis it is wrapped by one tile. Default: 0. */
    std::optional<float> offsetX;
    /** Vertical shift in pixels of the positioned source; positive moves it down. On a repeated axis it is wrapped by one tile. Default: 0. */
    std::optional<float> offsetY;
    /** Tile the source horizontally when true. Default: false. */
    std::optional<bool> repeatX;
    /** Tile the source vertically when true. Default: false. */
    std::optional<bool> repeatY;
    /** Mirror the source horizontally when true. Default: false. */
    std::optional<bool> flipX;
    /** Mirror the source vertically when true. Default: false. */
    std::optional<bool> flipY;
};

/**
 * Allocates a bitmap with every channel set to 0, with no gap between rows.
 * The caller owns the buffer and releases it with delete[].
 *
 * @param format Pixel format of the bitmap.
 * @param size   Dimensions in pixels.
 */
std::uint8_t* CreateBitmap(BitmapFormat format, Vec2 const& size);

/**
 * Allocates a new bitmap holding the pixels of another converted to a new
 * format. Alpha is dropped when the new format has none and is opaque when
 * the old one has none. Color becomes Grayscale as its luminance
 * (0.2126 R + 0.7152 G + 0.0722 B), and Grayscale becomes color with the
 * value in every channel. The new bitmap has no gap between rows. The caller
 * owns the buffer and releases it with delete[]. Throws std::runtime_error
 * if data is null.
 *
 * @param data        Pixels of the bitmap to convert.
 * @param bytesPerRow Distance in bytes between the starts of two rows of data, or 0 to take it from the format and the width; see BitmapFormat.
 * @param format      Pixel format of data.
 * @param size        Dimensions in pixels.
 * @param newFormat   Pixel format of the new bitmap.
 */
std::uint8_t* ConvertBitmap(std::uint8_t const* data, std::ptrdiff_t bytesPerRow, BitmapFormat format, Vec2 const& size, BitmapFormat newFormat);

/**
 * Blits a source bitmap into a region of a destination bitmap, combining it
 * with what is there as options.blend says. When the destination format has
 * no alpha, the destination is treated as opaque and the resulting alpha is
 * dropped. Source and destination may be the same buffer. Throws
 * std::runtime_error if either data pointer is null.
 *
 * @param options The bitmaps and how the source is placed in the destination.
 */
void BlitBitmap(BitmapBlitOptions const& options);

} /* namespace Rocket */
