/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>

namespace Rocket {

/** Pixel format of raw data passed to Image. */
enum class ImageDataFormat {
    Grayscale,
    RGBA,
    BGRA,
    RGB,
    BGR
};

/** Alignment of an image within its paint region. */
enum class ImagePosition {
    Start,
    Center,
    Stretch,
    End
};

/** Texture sampling filter applied during magnification or minification. */
enum class ImageFilter {
    Nearest,
    Linear
};

/**
 * A GPU-backed image resource.
 *
 * Non-copyable and non-movable. Can be constructed from a file path, from
 * raw pixel data, or as an empty surface of a given size. Used directly
 * as a paint target via ImagePaintTarget or indirectly through ImageBrush.
 */
class Image {
public:
    ~Image();
    Image();

    /**
     * Constructs an empty image of the given size.
     *
     * @param size Dimensions in pixels.
     */
    Image(Vec2 const& size);

    /**
     * Constructs an image from raw RGBA pixel data at the given size.
     * The RGB channels are premultiplied by alpha on upload. Throws
     * std::runtime_error if data is null.
     *
     * @param size Dimensions in pixels.
     * @param data Raw RGBA8 pixel bytes, tightly packed, row-major.
     */
    Image(Vec2 const& size, std::uint8_t const* data);

    /**
     * Constructs an image from raw pixel data in the specified format.
     * The RGB channels are premultiplied by alpha on upload. Throws
     * std::runtime_error if data is null.
     *
     * @param size   Dimensions in pixels.
     * @param data   Raw pixel bytes in the given format.
     * @param format Pixel format of the supplied data.
     */
    Image(Vec2 const& size, std::uint8_t const* data, ImageDataFormat format);

    /**
     * Constructs an image by loading from an image file path.
     * Throws std::runtime_error if the file cannot be loaded or converted.
     *
     * @param path Path to the image file.
     */
    Image(std::string const& path);

    Image(Image &&) = delete;
    Image(Image const&) = delete;
    Image& operator=(Image &&) = delete;
    Image& operator=(Image const&) = delete;

    /** Returns the image dimensions in pixels. */
    Vec2 const& getSize() const;

    /** Returns the underlying GPU texture handle as void*. */
    void* getTexture() const;

    /**
     * Resizes the image in place, replacing its GPU texture with one at the new size.
     *
     * Downloads the current pixels to CPU, rescales them, then re-uploads to a
     * new GPU texture. This is synchronous — it waits for
     * the GPU download to complete before returning.
     *
     * @param newSize The new dimensions in pixels.
     * @param filter  Resampling filter to use during scaling (default Nearest).
     */
    void setSize(Vec2 const& newSize, ImageFilter filter = ImageFilter::Nearest);

    /**
     * Uploads new pixel data into the image starting at offset (0, 0). Data
     * is converted to premultiplied RGBA before upload; Grayscale input is
     * treated as a coverage mask (RGB set to 0, the source byte becomes
     * alpha). Throws std::runtime_error if data is null.
     *
     * @param size   Dimensions of the pixel region to upload.
     * @param data   Raw pixel bytes in the given format.
     * @param format Pixel format of the supplied data.
     */
    void setData(Vec2 const& size, std::uint8_t const* data, ImageDataFormat format);

    /**
     * Updates a sub-region of the image with new pixel data. Data is
     * converted to premultiplied RGBA before upload; Grayscale input is
     * treated as a coverage mask (RGB set to 0, the source byte becomes
     * alpha). Throws std::runtime_error if data is null.
     *
     * @param size   Dimensions of the region to update.
     * @param data   Raw pixel bytes in the given format.
     * @param format Pixel format of the supplied data.
     * @param offset Top-left corner of the destination region.
     */
    void setData(Vec2 const& size, std::uint8_t const* data, ImageDataFormat format, Vec2 const& offset);

    /**
     * Downloads the image's pixels from the GPU as tightly-packed RGBA8
     * (4 bytes per pixel, row-major, alpha-premultiplied). The vector is
     * cleared first, then populated with the pixel bytes in RGBA order.
     * This is synchronous — it waits for the GPU download to complete
     * before returning.
     *
     * @param data Receives the image's pixel bytes.
     */
    void getData(std::vector<std::uint8_t>& data) const;

    /**
     * Copies a rectangular region from another image into this one.
     *
     * @param image    The source image.
     * @param slice    Region to copy from the source image.
     * @param position Top-left corner of the destination region.
     */
    void copyFrom(Image const& image, Vec4 const& slice, Vec2 const& position);
private:
    struct _Image;
    _Image* _impl;
    Vec2 _size;

    void __init(Vec2 const&);
    void __init(std::string const&);
    void __done();
    void* __getTexture() const;
    void __resize(Vec2 const&);
    void __setData(Vec2 const&, std::uint8_t const*, Vec2 const&);
    void __getData(std::vector<std::uint8_t>&) const;
    void __copyFrom(Image const&, Vec4 const&, Vec2 const&);

    friend class Painter;
};

} /* namespace Rocket */
