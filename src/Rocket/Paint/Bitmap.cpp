/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <Rocket/Paint/Bitmap.hpp>

namespace Rocket {

struct _PixelBytes {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

struct _AxisSample {
    bool inside;
    std::ptrdiff_t first;
    std::ptrdiff_t second;
    float weight;
};

struct _BlitRegion {
    std::uint8_t const* srcData;
    std::uint8_t* dstData;
    std::ptrdiff_t dstBytesPerRow;
    std::int32_t startX;
    std::int32_t startY;
    std::vector<_AxisSample> const& columns;
    std::vector<_AxisSample> const& rows;
};

static std::int32_t _ToInt(float value) {
    if (std::isnan(value)) {
        return 0;
    }

    return static_cast<std::int32_t>(std::clamp(value, -2147483520.0f, 2147483520.0f));
}

static std::int32_t _ToPixels(float size) {
    return std::max(_ToInt(size), 0);
}

static constexpr std::ptrdiff_t _GetBytesPerPixel(BitmapFormat format) {
    return (
        format == BitmapFormat::Grayscale ? 1 :
        format == BitmapFormat::RGB ? 3 :
        format == BitmapFormat::BGR ? 3 :
                                      4
    );
}

static std::ptrdiff_t _GetBytesPerRow(std::ptrdiff_t bytesPerRow, BitmapFormat format, std::int32_t width) {
    return ((bytesPerRow != 0) ? bytesPerRow : (width * _GetBytesPerPixel(format)));
}

static std::size_t _GetByteCount(BitmapFormat format, Vec2 const& size) {
    return (static_cast<std::size_t>(_ToPixels(size.width)) * static_cast<std::size_t>(_ToPixels(size.height)) * static_cast<std::size_t>(_GetBytesPerPixel(format)));
}

static std::pair<std::uintptr_t, std::uintptr_t> _GetMemoryRange(std::uint8_t const* data, std::ptrdiff_t bytesPerRow, BitmapFormat format, std::int32_t width, std::int32_t height) {
    if ((width <= 0) || (height <= 0)) {
        return { 0u, 0u };
    }

    auto const first = reinterpret_cast<std::uintptr_t>(data);
    auto const last = reinterpret_cast<std::uintptr_t>(data + ((height - 1) * bytesPerRow));

    return { std::min(first, last), (std::max(first, last) + (width * _GetBytesPerPixel(format))) };
}

static std::uint8_t _ToByte(float value) {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

static float _ToFloat(std::uint8_t value) {
    return (static_cast<float>(value) / 255.0f);
}

static Vec4 _ToColor(_PixelBytes const& bytes) {
    return Vec4{ _ToFloat(bytes.red), _ToFloat(bytes.green), _ToFloat(bytes.blue), _ToFloat(bytes.alpha) };
}

static std::uint8_t _GetLuminance(float red, float green, float blue) {
    return _ToByte((0.2126f * red) + (0.7152f * green) + (0.0722f * blue));
}

template<BitmapFormat Format>
static _PixelBytes _ReadBytes(std::uint8_t const* pixel) {
    if constexpr (Format == BitmapFormat::Grayscale) {
        return { pixel[0], pixel[0], pixel[0], 255 };
    } else if constexpr (Format == BitmapFormat::RGBA) {
        return { pixel[0], pixel[1], pixel[2], pixel[3] };
    } else if constexpr (Format == BitmapFormat::BGRA) {
        return { pixel[2], pixel[1], pixel[0], pixel[3] };
    } else if constexpr (Format == BitmapFormat::RGB) {
        return { pixel[0], pixel[1], pixel[2], 255 };
    } else {
        return { pixel[2], pixel[1], pixel[0], 255 };
    }
}

template<BitmapFormat Format>
static void _WriteBytes(std::uint8_t* pixel, _PixelBytes const& bytes) {
    if constexpr (Format == BitmapFormat::Grayscale) {
        pixel[0] = _GetLuminance(_ToFloat(bytes.red), _ToFloat(bytes.green), _ToFloat(bytes.blue));
    } else if constexpr (Format == BitmapFormat::RGBA) {
        pixel[0] = bytes.red;
        pixel[1] = bytes.green;
        pixel[2] = bytes.blue;
        pixel[3] = bytes.alpha;
    } else if constexpr (Format == BitmapFormat::BGRA) {
        pixel[0] = bytes.blue;
        pixel[1] = bytes.green;
        pixel[2] = bytes.red;
        pixel[3] = bytes.alpha;
    } else if constexpr (Format == BitmapFormat::RGB) {
        pixel[0] = bytes.red;
        pixel[1] = bytes.green;
        pixel[2] = bytes.blue;
    } else {
        pixel[0] = bytes.blue;
        pixel[1] = bytes.green;
        pixel[2] = bytes.red;
    }
}

template<BitmapFormat Format>
static void _WritePixel(std::uint8_t* pixel, Vec4 const& color) {
    if constexpr (Format == BitmapFormat::Grayscale) {
        pixel[0] = _GetLuminance(color.red, color.green, color.blue);
    } else {
        _WriteBytes<Format>(pixel, { _ToByte(color.red), _ToByte(color.green), _ToByte(color.blue), _ToByte(color.alpha) });
    }
}

static _PixelBytes _ReadBytes(std::uint8_t const* pixel, BitmapFormat format) {
    switch (format) {
        case BitmapFormat::Grayscale: return _ReadBytes<BitmapFormat::Grayscale>(pixel);
        case BitmapFormat::RGBA: return _ReadBytes<BitmapFormat::RGBA>(pixel);
        case BitmapFormat::BGRA: return _ReadBytes<BitmapFormat::BGRA>(pixel);
        case BitmapFormat::RGB: return _ReadBytes<BitmapFormat::RGB>(pixel);
        case BitmapFormat::BGR: return _ReadBytes<BitmapFormat::BGR>(pixel);
    }

    return {};
}

static void _WritePixel(std::uint8_t* pixel, BitmapFormat format, Vec4 const& color) {
    switch (format) {
        case BitmapFormat::Grayscale: _WritePixel<BitmapFormat::Grayscale>(pixel, color); break;
        case BitmapFormat::RGBA: _WritePixel<BitmapFormat::RGBA>(pixel, color); break;
        case BitmapFormat::BGRA: _WritePixel<BitmapFormat::BGRA>(pixel, color); break;
        case BitmapFormat::RGB: _WritePixel<BitmapFormat::RGB>(pixel, color); break;
        case BitmapFormat::BGR: _WritePixel<BitmapFormat::BGR>(pixel, color); break;
    }
}

template<BitmapFormat Src, BitmapFormat Dst>
static void _ConvertRows(std::uint8_t const* data, std::ptrdiff_t bytesPerRow, std::uint8_t* result, std::int32_t width, std::int32_t height) {
    for (auto y = 0; y < height; y++) {
        auto const srcRow = (data + (y * bytesPerRow));
        auto const newRow = (result + (y * (width * _GetBytesPerPixel(Dst))));

        for (auto x = 0; x < width; x++) {
            _WriteBytes<Dst>((newRow + (x * _GetBytesPerPixel(Dst))), _ReadBytes<Src>(srcRow + (x * _GetBytesPerPixel(Src))));
        }
    }
}

template<BitmapFormat Src>
static void _ConvertRowsTo(BitmapFormat newFormat, std::uint8_t const* data, std::ptrdiff_t bytesPerRow, std::uint8_t* result, std::int32_t width, std::int32_t height) {
    switch (newFormat) {
        case BitmapFormat::Grayscale: _ConvertRows<Src, BitmapFormat::Grayscale>(data, bytesPerRow, result, width, height); break;
        case BitmapFormat::RGBA: _ConvertRows<Src, BitmapFormat::RGBA>(data, bytesPerRow, result, width, height); break;
        case BitmapFormat::BGRA: _ConvertRows<Src, BitmapFormat::BGRA>(data, bytesPerRow, result, width, height); break;
        case BitmapFormat::RGB: _ConvertRows<Src, BitmapFormat::RGB>(data, bytesPerRow, result, width, height); break;
        case BitmapFormat::BGR: _ConvertRows<Src, BitmapFormat::BGR>(data, bytesPerRow, result, width, height); break;
    }
}

static float _NPatchAxis(float position, float srcSize, float dstSize, float srcStart, float srcEnd, float dstStart, float dstEnd) {
    if (position < dstStart) {
        return (srcStart * (position / dstStart));
    }

    auto const srcCenterEnd = (srcSize - srcEnd);
    auto const dstCenterEnd = (dstSize - dstEnd);

    if (position < dstCenterEnd) {
        return (srcStart + ((srcCenterEnd - srcStart) * ((position - dstStart) / (dstCenterEnd - dstStart))));
    }

    return (srcCenterEnd + ((srcSize - srcCenterEnd) * ((position - dstCenterEnd) / (dstSize - dstCenterEnd))));
}

static Vec4 _Blend(Vec4 const& destination, Vec4 const& source, BitmapBlend blend) {
    if (blend == BitmapBlend::Replace) {
        return source;
    }

    if (source.alpha <= 0.0f) {
        return destination;
    }

    auto const alpha = (source.alpha + (destination.alpha * (1.0f - source.alpha)));

    auto const mix = [&](float backdrop, float color) {
        return (
            blend == BitmapBlend::Multiply ? (backdrop * color) :
            blend == BitmapBlend::Screen ? ((backdrop + color) - (backdrop * color)) :
            blend == BitmapBlend::Overlay ? ((backdrop <= 0.5f) ? (2.0f * backdrop * color) : (1.0f - (2.0f * (1.0f - backdrop) * (1.0f - color)))) :
            blend == BitmapBlend::Add ? std::min((backdrop + color), 1.0f) :
            blend == BitmapBlend::Darken ? std::min(backdrop, color) :
            blend == BitmapBlend::Lighten ? std::max(backdrop, color) :
                                            color
        );
    };

    auto const composite = [&](float backdrop, float color) {
        return (
            (
                (source.alpha * (1.0f - destination.alpha) * color) +
                (source.alpha * destination.alpha * mix(backdrop, color)) +
                ((1.0f - source.alpha) * destination.alpha * backdrop)
            ) / alpha
        );
    };

    return Vec4{
        composite(destination.red, source.red),
        composite(destination.green, source.green),
        composite(destination.blue, source.blue),
        alpha
    };
}

static std::optional<std::pair<std::size_t, std::size_t>> _FindCopySpan(std::vector<_AxisSample> const& columns, std::ptrdiff_t bytesPerPixel) {
    auto first = 0uz;

    while ((first < columns.size()) && (columns[first].inside == false)) {
        first++;
    }

    if (first == columns.size()) {
        return std::nullopt;
    }

    auto last = first;

    while (
        (last < columns.size()) &&
        columns[last].inside &&
        (columns[last].first == (columns[first].first + (static_cast<std::ptrdiff_t>(last - first) * bytesPerPixel)))
    ) {
        last++;
    }

    if (std::any_of((columns.begin() + last), columns.end(), [](_AxisSample const& column) { return column.inside; })) {
        return std::nullopt;
    }

    return std::pair{ first, (last - first) };
}

static void _CopyRows(_BlitRegion const& region, std::size_t first, std::size_t count, std::ptrdiff_t bytesPerPixel) {
    for (auto i = 0uz; i < region.rows.size(); i++) {
        if (region.rows[i].inside == false) {
            continue;
        }

        std::memcpy(
            (region.dstData + ((region.startY + static_cast<std::ptrdiff_t>(i)) * region.dstBytesPerRow) + ((region.startX + static_cast<std::ptrdiff_t>(first)) * bytesPerPixel)),
            (region.srcData + region.rows[i].first + region.columns[first].first),
            (count * bytesPerPixel)
        );
    }
}

template<BitmapFormat Src, BitmapFormat Dst>
static void _BlitNearest(_BlitRegion const& region, BitmapBlend blend) {
    for (auto i = 0uz; i < region.rows.size(); i++) {
        if (region.rows[i].inside == false) {
            continue;
        }

        auto const srcRow = (region.srcData + region.rows[i].first);
        auto const dstRow = (region.dstData + ((region.startY + static_cast<std::ptrdiff_t>(i)) * region.dstBytesPerRow) + (region.startX * _GetBytesPerPixel(Dst)));

        for (auto j = 0uz; j < region.columns.size(); j++) {
            if (region.columns[j].inside == false) {
                continue;
            }

            auto const target = (dstRow + (static_cast<std::ptrdiff_t>(j) * _GetBytesPerPixel(Dst)));
            auto const bytes = _ReadBytes<Src>(srcRow + region.columns[j].first);

            if ((blend == BitmapBlend::Replace) || (bytes.alpha == 255)) {
                _WriteBytes<Dst>(target, bytes);
            } else if (bytes.alpha != 0) {
                _WritePixel<Dst>(target, _Blend(_ToColor(_ReadBytes<Dst>(target)), _ToColor(bytes), BitmapBlend::Normal));
            }
        }
    }
}

template<BitmapFormat Src>
static void _BlitNearestTo(BitmapFormat dstFormat, _BlitRegion const& region, BitmapBlend blend) {
    switch (dstFormat) {
        case BitmapFormat::Grayscale: _BlitNearest<Src, BitmapFormat::Grayscale>(region, blend); break;
        case BitmapFormat::RGBA: _BlitNearest<Src, BitmapFormat::RGBA>(region, blend); break;
        case BitmapFormat::BGRA: _BlitNearest<Src, BitmapFormat::BGRA>(region, blend); break;
        case BitmapFormat::RGB: _BlitNearest<Src, BitmapFormat::RGB>(region, blend); break;
        case BitmapFormat::BGR: _BlitNearest<Src, BitmapFormat::BGR>(region, blend); break;
    }
}

static void _BlitGeneral(_BlitRegion const& region, BitmapFormat srcFormat, BitmapFormat dstFormat, BitmapFilter filter, BitmapBlend blend, Vec4 const& color) {
    auto const isTinted = (color.alpha > 0.0f);
    auto const dstBytesPerPixel = _GetBytesPerPixel(dstFormat);

    auto const interpolate = [&](std::uint8_t const* srcRow, std::uint8_t const* srcNextRow, _AxisSample const& row, _AxisSample const& column) {
        auto result = Vec4{};

        for (auto const& [ pixel, weight ] : {
            std::pair{ (srcRow + column.first), ((1.0f - column.weight) * (1.0f - row.weight)) },
            std::pair{ (srcRow + column.second), (column.weight * (1.0f - row.weight)) },
            std::pair{ (srcNextRow + column.first), ((1.0f - column.weight) * row.weight) },
            std::pair{ (srcNextRow + column.second), (column.weight * row.weight) }
        }) {
            auto const texel = _ToColor(_ReadBytes(pixel, srcFormat));
            result.red += (texel.red * texel.alpha * weight);
            result.green += (texel.green * texel.alpha * weight);
            result.blue += (texel.blue * texel.alpha * weight);
            result.alpha += (texel.alpha * weight);
        }

        if (result.alpha <= 0.0f) {
            return Vec4{};
        }

        return Vec4{ (result.red / result.alpha), (result.green / result.alpha), (result.blue / result.alpha), result.alpha };
    };

    for (auto i = 0uz; i < region.rows.size(); i++) {
        auto const& row = region.rows[i];

        if (row.inside == false) {
            continue;
        }

        auto const srcRow = (region.srcData + row.first);
        auto const srcNextRow = (region.srcData + row.second);
        auto const dstRow = (region.dstData + ((region.startY + static_cast<std::ptrdiff_t>(i)) * region.dstBytesPerRow) + (region.startX * dstBytesPerPixel));

        for (auto j = 0uz; j < region.columns.size(); j++) {
            auto const& column = region.columns[j];

            if (column.inside == false) {
                continue;
            }

            auto const target = (dstRow + (static_cast<std::ptrdiff_t>(j) * dstBytesPerPixel));
            auto pixel = Vec4{};

            if (filter == BitmapFilter::Nearest) {
                auto const bytes = _ReadBytes((srcRow + column.first), srcFormat);

                if ((blend != BitmapBlend::Replace) && (bytes.alpha == 0)) {
                    continue;
                }

                pixel = _ToColor(bytes);
            } else {
                pixel = interpolate(srcRow, srcNextRow, row, column);
            }

            if ((pixel.alpha > 0.0f) && isTinted) {
                pixel = Vec4{ color.red, color.green, color.blue, (pixel.alpha * color.alpha) };
            }

            _WritePixel(target, dstFormat, _Blend(_ToColor(_ReadBytes(target, dstFormat)), pixel, blend));
        }
    }
}

std::uint8_t* CreateBitmap(BitmapFormat format, Vec2 const& size) {
    return new std::uint8_t[_GetByteCount(format, size)]();
}

std::uint8_t* ConvertBitmap(std::uint8_t const* data, std::ptrdiff_t bytesPerRow, BitmapFormat format, Vec2 const& size, BitmapFormat newFormat) {
    if (data == nullptr) {
        throw std::runtime_error("Failed to convert a bitmap: no pixel data");
    }

    auto const width = _ToPixels(size.width);
    auto const height = _ToPixels(size.height);
    auto const srcBytesPerRow = _GetBytesPerRow(bytesPerRow, format, width);
    auto const result = new std::uint8_t[_GetByteCount(newFormat, size)];

    if (format == newFormat) {
        for (auto y = 0; y < height; y++) {
            std::memcpy((result + (y * (width * _GetBytesPerPixel(format)))), (data + (y * srcBytesPerRow)), (width * _GetBytesPerPixel(format)));
        }

        return result;
    }

    switch (format) {
        case BitmapFormat::Grayscale: _ConvertRowsTo<BitmapFormat::Grayscale>(newFormat, data, srcBytesPerRow, result, width, height); break;
        case BitmapFormat::RGBA: _ConvertRowsTo<BitmapFormat::RGBA>(newFormat, data, srcBytesPerRow, result, width, height); break;
        case BitmapFormat::BGRA: _ConvertRowsTo<BitmapFormat::BGRA>(newFormat, data, srcBytesPerRow, result, width, height); break;
        case BitmapFormat::RGB: _ConvertRowsTo<BitmapFormat::RGB>(newFormat, data, srcBytesPerRow, result, width, height); break;
        case BitmapFormat::BGR: _ConvertRowsTo<BitmapFormat::BGR>(newFormat, data, srcBytesPerRow, result, width, height); break;
    }

    return result;
}

void BlitBitmap(BitmapBlitOptions const& options) {
    if ((options.dstData == nullptr) || (options.srcData == nullptr)) {
        throw std::runtime_error("Failed to blit a bitmap: no pixel data");
    }

    auto const dstWidth = _ToPixels(options.dstSize.width);
    auto const dstHeight = _ToPixels(options.dstSize.height);
    auto const dstBytesPerRow = _GetBytesPerRow(options.dstBytesPerRow, options.dstFormat, dstWidth);
    auto const srcWidth = _ToPixels(options.srcSize.width);
    auto const srcHeight = _ToPixels(options.srcSize.height);
    auto const srcBytesPerPixel = _GetBytesPerPixel(options.srcFormat);
    auto srcBytesPerRow = _GetBytesPerRow(options.srcBytesPerRow, options.srcFormat, srcWidth);
    auto srcCopy = std::vector<std::uint8_t>();
    auto srcData = options.srcData;

    auto const dstRange = _GetMemoryRange(options.dstData, dstBytesPerRow, options.dstFormat, dstWidth, dstHeight);
    auto const srcRange = _GetMemoryRange(srcData, srcBytesPerRow, options.srcFormat, srcWidth, srcHeight);

    if ((srcRange.first < dstRange.second) && (dstRange.first < srcRange.second)) {
        auto const packedBytesPerRow = (srcWidth * srcBytesPerPixel);

        srcCopy.resize(static_cast<std::size_t>(packedBytesPerRow) * static_cast<std::size_t>(srcHeight));

        for (auto y = 0; y < srcHeight; y++) {
            std::memcpy((srcCopy.data() + (y * packedBytesPerRow)), (srcData + (y * srcBytesPerRow)), packedBytesPerRow);
        }

        srcData = srcCopy.data();
        srcBytesPerRow = packedBytesPerRow;
    }

    auto const& bounds = options.dstRect;
    auto const nPatch = options.nPatch.value_or(Vec4{});
    auto const color = options.color.value_or(Vec4{});
    auto const blend = options.blend.value_or(BitmapBlend::Normal);
    auto const filter = options.filter.value_or(BitmapFilter::Nearest);
    auto const positionX = options.positionX.value_or(BitmapPosition::Stretch);
    auto const positionY = options.positionY.value_or(BitmapPosition::Stretch);
    auto const offsetX = options.offsetX.value_or(0.0f);
    auto const offsetY = options.offsetY.value_or(0.0f);
    auto const repeatX = options.repeatX.value_or(false);
    auto const repeatY = options.repeatY.value_or(false);
    auto const imageSrc = options.srcRect.value_or(Vec4{ 0.0f, 0.0f, options.srcSize.width, options.srcSize.height });
    auto imageDst = Vec4{ 0.0f, 0.0f, imageSrc.width, imageSrc.height };

    if (positionX == BitmapPosition::Center) {
        imageDst.x = ((bounds.width - imageDst.width) / 2.0f);
    } else if (positionX == BitmapPosition::End) {
        imageDst.x = (bounds.width - imageDst.width);
    } else if (positionX == BitmapPosition::Stretch) {
        imageDst.width = bounds.width;
    }

    if (positionY == BitmapPosition::Center) {
        imageDst.y = ((bounds.height - imageDst.height) / 2.0f);
    } else if (positionY == BitmapPosition::End) {
        imageDst.y = (bounds.height - imageDst.height);
    } else if (positionY == BitmapPosition::Stretch) {
        imageDst.height = bounds.height;
    }

    if ((imageSrc.width <= 0.0f) || (imageSrc.height <= 0.0f) || (imageDst.width <= 0.0f) || (imageDst.height <= 0.0f)) {
        return;
    }

    imageDst.x += (repeatX ? std::fmod(offsetX, imageDst.width) : offsetX);
    imageDst.y += (repeatY ? std::fmod(offsetY, imageDst.height) : offsetY);

    auto const sampleMinX = std::max(0, _ToInt(std::floor(imageSrc.x)));
    auto const sampleMinY = std::max(0, _ToInt(std::floor(imageSrc.y)));
    auto const sampleMaxX = (std::min(srcWidth, _ToInt(std::ceil(imageSrc.getMaxX()))) - 1);
    auto const sampleMaxY = (std::min(srcHeight, _ToInt(std::ceil(imageSrc.getMaxY()))) - 1);

    if ((sampleMinX > sampleMaxX) || (sampleMinY > sampleMaxY)) {
        return;
    }

    auto const mapAxis = [&](std::int32_t axis, std::int32_t start, std::int32_t end, bool repeat, bool flip, std::int32_t sampleMin, std::int32_t sampleMax, std::ptrdiff_t stride) {
        auto samples = std::vector<_AxisSample>(std::max(0, (end - start)));
        auto const srcOrigin = imageSrc.data[axis];
        auto const srcSize = imageSrc.data[axis + 2];
        auto const dstSize = imageDst.data[axis + 2];
        auto const nPatchStart = nPatch.data[axis];
        auto const nPatchEnd = nPatch.data[axis + 2];
        auto const nPatchSize = std::max((nPatchStart + nPatchEnd), 0.0001f);
        auto const srcFit = std::min(1.0f, (srcSize / nPatchSize));
        auto const dstFit = std::min(1.0f, (dstSize / nPatchSize));

        for (auto i = start; i < end; i++) {
            auto local = (((static_cast<float>(i) + 0.5f) - bounds.data[axis]) - imageDst.data[axis]);

            if (repeat) {
                local -= (dstSize * std::floor(local / dstSize));
            }

            if (((local >= 0.0f) && (local < dstSize)) == false) {
                continue;
            }

            auto const position = _NPatchAxis(local, srcSize, dstSize, (nPatchStart * srcFit), (nPatchEnd * srcFit), (nPatchStart * dstFit), (nPatchEnd * dstFit));
            auto const coordinate = (flip ? ((srcOrigin + srcSize) - position) : (srcOrigin + position));

            if (filter == BitmapFilter::Nearest) {
                auto const index = (std::clamp(_ToInt(std::floor(coordinate)), sampleMin, sampleMax) * stride);
                samples[i - start] = { true, index, index, 0.0f };
                continue;
            }

            auto const lower = std::floor(coordinate - 0.5f);

            samples[i - start] = {
                true,
                (std::clamp(_ToInt(lower), sampleMin, sampleMax) * stride),
                (std::clamp(_ToInt(lower + 1.0f), sampleMin, sampleMax) * stride),
                ((coordinate - 0.5f) - lower)
            };
        }

        return samples;
    };

    auto const startX = std::max(0, _ToInt(std::ceil(bounds.x - 0.5f)));
    auto const startY = std::max(0, _ToInt(std::ceil(bounds.y - 0.5f)));
    auto const endX = std::min(dstWidth, _ToInt(std::ceil(bounds.getMaxX() - 0.5f)));
    auto const endY = std::min(dstHeight, _ToInt(std::ceil(bounds.getMaxY() - 0.5f)));
    auto const columns = mapAxis(0, startX, endX, repeatX, options.flipX.value_or(false), sampleMinX, sampleMaxX, srcBytesPerPixel);
    auto const rows = mapAxis(1, startY, endY, repeatY, options.flipY.value_or(false), sampleMinY, sampleMaxY, srcBytesPerRow);
    auto const region = _BlitRegion{ srcData, options.dstData, dstBytesPerRow, startX, startY, columns, rows };
    auto const isPlain = ((filter == BitmapFilter::Nearest) && (color.alpha <= 0.0f) && ((blend == BitmapBlend::Replace) || (blend == BitmapBlend::Normal)));
    auto const isOpaqueFormat = ((options.srcFormat == BitmapFormat::Grayscale) || (options.srcFormat == BitmapFormat::RGB) || (options.srcFormat == BitmapFormat::BGR));

    if (isPlain && (options.srcFormat == options.dstFormat) && ((blend == BitmapBlend::Replace) || isOpaqueFormat)) {
        if (auto const span = _FindCopySpan(columns, srcBytesPerPixel); span.has_value()) {
            _CopyRows(region, span->first, span->second, srcBytesPerPixel);
            return;
        }
    }

    if (isPlain) {
        switch (options.srcFormat) {
            case BitmapFormat::Grayscale: _BlitNearestTo<BitmapFormat::Grayscale>(options.dstFormat, region, blend); break;
            case BitmapFormat::RGBA: _BlitNearestTo<BitmapFormat::RGBA>(options.dstFormat, region, blend); break;
            case BitmapFormat::BGRA: _BlitNearestTo<BitmapFormat::BGRA>(options.dstFormat, region, blend); break;
            case BitmapFormat::RGB: _BlitNearestTo<BitmapFormat::RGB>(options.dstFormat, region, blend); break;
            case BitmapFormat::BGR: _BlitNearestTo<BitmapFormat::BGR>(options.dstFormat, region, blend); break;
        }

        return;
    }

    _BlitGeneral(region, options.srcFormat, options.dstFormat, filter, blend, color);
}

} /* namespace Rocket */
