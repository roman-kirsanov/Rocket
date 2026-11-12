/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <array>
#include <limits>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <initializer_list>
#include <Rocket/Paint/Bitmap.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

using Bytes = std::vector<std::uint8_t>;
using Pixel = std::array<std::uint8_t, 4>;

static auto const _red = Pixel{ 255, 0, 0, 255 };
static auto const _green = Pixel{ 0, 255, 0, 255 };
static auto const _blue = Pixel{ 0, 0, 255, 255 };
static auto const _white = Pixel{ 255, 255, 255, 255 };
static auto const _clear = Pixel{ 0, 0, 0, 0 };

static Bytes _Pixels(std::initializer_list<Pixel> pixels) {
    auto bytes = Bytes{};

    for (auto const& pixel : pixels) {
        bytes.insert(bytes.end(), pixel.begin(), pixel.end());
    }

    return bytes;
}

static Bytes _Take(std::uint8_t* data, std::size_t count) {
    auto const bytes = Bytes(data, (data + count));
    delete[] data;
    return bytes;
}

static Pixel _PixelAt(Bytes const& bytes, int width, int x, int y) {
    auto const index = (((y * width) + x) * 4);
    return Pixel{ bytes[index], bytes[index + 1], bytes[index + 2], bytes[index + 3] };
}

static BitmapBlitOptions _Options(Bytes& dst, Vec2 const& dstSize, Bytes const& src, Vec2 const& srcSize) {
    return BitmapBlitOptions{
        .dstData = dst.data(),
        .dstFormat = BitmapFormat::RGBA,
        .dstSize = dstSize,
        .dstBytesPerRow = 0,
        .dstRect = Vec4{ 0.0f, 0.0f, dstSize.width, dstSize.height },
        .srcData = src.data(),
        .srcFormat = BitmapFormat::RGBA,
        .srcSize = srcSize,
        .srcBytesPerRow = 0
    };
}

static Pixel _BlendOne(Pixel const& destination, Pixel const& source, BitmapBlend blend) {
    auto dst = _Pixels({ destination });
    auto const src = _Pixels({ source });
    auto options = _Options(dst, Vec2{ 1.0f, 1.0f }, src, Vec2{ 1.0f, 1.0f });
    options.blend = blend;
    BlitBitmap(options);
    return _PixelAt(dst, 1, 0, 0);
}

TEST(Bitmap, CreateAllocatesZeroedPixelsForEveryFormat) {
    for (auto const& [ format, bytesPerPixel ] : {
        std::pair{ BitmapFormat::Grayscale, 1 },
        std::pair{ BitmapFormat::RGB, 3 },
        std::pair{ BitmapFormat::BGR, 3 },
        std::pair{ BitmapFormat::RGBA, 4 },
        std::pair{ BitmapFormat::BGRA, 4 }
    }) {
        auto const count = static_cast<std::size_t>(3 * 2 * bytesPerPixel);
        EXPECT_EQ(_Take(CreateBitmap(format, Vec2{ 3.0f, 2.0f }), count), Bytes(count, 0));
    }
}

TEST(Bitmap, CreateWithZeroSizeReturnsABuffer) {
    auto const data = CreateBitmap(BitmapFormat::RGBA, Vec2{ 0.0f, 0.0f });
    EXPECT_NE(data, nullptr);
    delete[] data;
}

TEST(Bitmap, ConvertSwapsRGBAndBGR) {
    auto const rgb = Bytes{ 1, 2, 3, 4, 5, 6 };
    EXPECT_EQ(_Take(ConvertBitmap(rgb.data(), 0, BitmapFormat::RGB, Vec2{ 2.0f, 1.0f }, BitmapFormat::BGR), 6), (Bytes{ 3, 2, 1, 6, 5, 4 }));
    EXPECT_EQ(_Take(ConvertBitmap(rgb.data(), 0, BitmapFormat::BGR, Vec2{ 2.0f, 1.0f }, BitmapFormat::RGB), 6), (Bytes{ 3, 2, 1, 6, 5, 4 }));
}

TEST(Bitmap, ConvertSwapsRGBAAndBGRAKeepingAlpha) {
    auto const rgba = Bytes{ 1, 2, 3, 40 };
    EXPECT_EQ(_Take(ConvertBitmap(rgba.data(), 0, BitmapFormat::RGBA, Vec2{ 1.0f, 1.0f }, BitmapFormat::BGRA), 4), (Bytes{ 3, 2, 1, 40 }));
    EXPECT_EQ(_Take(ConvertBitmap(rgba.data(), 0, BitmapFormat::BGRA, Vec2{ 1.0f, 1.0f }, BitmapFormat::RGBA), 4), (Bytes{ 3, 2, 1, 40 }));
}

TEST(Bitmap, ConvertDropsAlphaWhenTheNewFormatHasNone) {
    auto const rgba = Bytes{ 10, 20, 30, 0 };
    EXPECT_EQ(_Take(ConvertBitmap(rgba.data(), 0, BitmapFormat::RGBA, Vec2{ 1.0f, 1.0f }, BitmapFormat::RGB), 3), (Bytes{ 10, 20, 30 }));
    EXPECT_EQ(_Take(ConvertBitmap(rgba.data(), 0, BitmapFormat::RGBA, Vec2{ 1.0f, 1.0f }, BitmapFormat::BGR), 3), (Bytes{ 30, 20, 10 }));
}

TEST(Bitmap, ConvertMakesAlphaOpaqueWhenTheOldFormatHasNone) {
    auto const rgb = Bytes{ 10, 20, 30 };
    EXPECT_EQ(_Take(ConvertBitmap(rgb.data(), 0, BitmapFormat::RGB, Vec2{ 1.0f, 1.0f }, BitmapFormat::RGBA), 4), (Bytes{ 10, 20, 30, 255 }));
    EXPECT_EQ(_Take(ConvertBitmap(rgb.data(), 0, BitmapFormat::RGB, Vec2{ 1.0f, 1.0f }, BitmapFormat::BGRA), 4), (Bytes{ 30, 20, 10, 255 }));
}

TEST(Bitmap, ConvertToGrayscaleUsesLuminance) {
    auto const rgb = Bytes{ 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255 };
    EXPECT_EQ(_Take(ConvertBitmap(rgb.data(), 0, BitmapFormat::RGB, Vec2{ 4.0f, 1.0f }, BitmapFormat::Grayscale), 4), (Bytes{ 54, 182, 18, 255 }));
}

TEST(Bitmap, ConvertFromGrayscaleCopiesTheValueToEveryChannel) {
    auto const gray = Bytes{ 0, 128 };
    EXPECT_EQ(_Take(ConvertBitmap(gray.data(), 0, BitmapFormat::Grayscale, Vec2{ 2.0f, 1.0f }, BitmapFormat::RGBA), 8), (Bytes{ 0, 0, 0, 255, 128, 128, 128, 255 }));
    EXPECT_EQ(_Take(ConvertBitmap(gray.data(), 0, BitmapFormat::Grayscale, Vec2{ 2.0f, 1.0f }, BitmapFormat::Grayscale), 2), gray);
}

TEST(Bitmap, ConvertReadsPaddedRows) {
    auto const rgb = Bytes{
        1, 2, 3, 4, 5, 6, 99, 99,
        7, 8, 9, 10, 11, 12, 99, 99
    };
    EXPECT_EQ(_Take(ConvertBitmap(rgb.data(), 8, BitmapFormat::RGB, Vec2{ 2.0f, 2.0f }, BitmapFormat::RGB), 12), (Bytes{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 }));
}

TEST(Bitmap, ConvertReadsRowsStoredBottomFirst) {
    auto const rgb = Bytes{
        7, 8, 9, 10, 11, 12,
        1, 2, 3, 4, 5, 6
    };
    EXPECT_EQ(_Take(ConvertBitmap((rgb.data() + 6), -6, BitmapFormat::RGB, Vec2{ 2.0f, 2.0f }, BitmapFormat::RGB), 12), (Bytes{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 }));
}

TEST(Bitmap, ConvertReadsARegionOfALargerBitmap) {
    auto const rgba = _Pixels({
        _clear, _clear, _clear,
        _clear, _red, _green,
        _clear, _blue, _white
    });
    EXPECT_EQ(_Take(ConvertBitmap((rgba.data() + 16), 12, BitmapFormat::RGBA, Vec2{ 2.0f, 2.0f }, BitmapFormat::RGBA), 16), _Pixels({ _red, _green, _blue, _white }));
}

TEST(Bitmap, ConvertThrowsWithoutData) {
    EXPECT_THROW(ConvertBitmap(nullptr, 0, BitmapFormat::RGB, Vec2{ 1.0f, 1.0f }, BitmapFormat::RGBA), std::runtime_error);
}

TEST(Bitmap, BlitStretchesTheSourceOverTheRectByDefault) {
    auto dst = Bytes(4 * 4 * 4, 0);
    auto const src = _Pixels({ _red, _green, _blue, _white });
    BlitBitmap(_Options(dst, Vec2{ 4.0f, 4.0f }, src, Vec2{ 2.0f, 2.0f }));
    EXPECT_EQ(dst, _Pixels({
        _red, _red, _green, _green,
        _red, _red, _green, _green,
        _blue, _blue, _white, _white,
        _blue, _blue, _white, _white
    }));
}

TEST(Bitmap, BlitShrinksTheSourceToTheNearestPixel) {
    auto dst = Bytes(2 * 1 * 4, 0);
    auto const src = _Pixels({ _red, _green, _blue, _white });
    BlitBitmap(_Options(dst, Vec2{ 2.0f, 1.0f }, src, Vec2{ 4.0f, 1.0f }));
    EXPECT_EQ(dst, _Pixels({ _green, _white }));
}

TEST(Bitmap, BlitClipsTheRectToTheDestination) {
    auto dst = Bytes(4 * 4 * 4, 0);
    auto const src = _Pixels({ _red, _green, _blue, _white });
    auto options = _Options(dst, Vec2{ 4.0f, 4.0f }, src, Vec2{ 2.0f, 2.0f });
    options.dstRect = Vec4{ 2.0f, 2.0f, 4.0f, 4.0f };
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({
        _clear, _clear, _clear, _clear,
        _clear, _clear, _clear, _clear,
        _clear, _clear, _red, _red,
        _clear, _clear, _red, _red
    }));

    auto other = Bytes(2 * 2 * 4, 0);
    auto negative = _Options(other, Vec2{ 2.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
    negative.dstRect = Vec4{ -1.0f, -1.0f, 2.0f, 2.0f };
    BlitBitmap(negative);
    EXPECT_EQ(other, _Pixels({ _white, _clear, _clear, _clear }));
}

TEST(Bitmap, BlitTakesTheSourceRect) {
    auto dst = Bytes(2 * 1 * 4, 0);
    auto const src = _Pixels({ _red, _green, _blue });
    auto options = _Options(dst, Vec2{ 2.0f, 1.0f }, src, Vec2{ 3.0f, 1.0f });
    options.srcRect = Vec4{ 1.0f, 0.0f, 2.0f, 1.0f };
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({ _green, _blue }));
}

TEST(Bitmap, BlitDoesNothingForAnEmptySourceRect) {
    auto dst = _Pixels({ _white });
    auto const src = _Pixels({ _red });
    auto options = _Options(dst, Vec2{ 1.0f, 1.0f }, src, Vec2{ 1.0f, 1.0f });
    options.srcRect = Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({ _white }));
}

TEST(Bitmap, BlitDoesNothingWhenNoColumnIsReached) {
    auto const src = _Pixels({ _red, _green });

    auto outside = _Pixels({ _white, _white });
    auto outsideOptions = _Options(outside, Vec2{ 2.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    outsideOptions.dstRect = Vec4{ 5.0f, 0.0f, 2.0f, 1.0f };
    outsideOptions.blend = BitmapBlend::Replace;
    BlitBitmap(outsideOptions);
    EXPECT_EQ(outside, _Pixels({ _white, _white }));

    auto shifted = _Pixels({ _white, _white });
    auto shiftedOptions = _Options(shifted, Vec2{ 2.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    shiftedOptions.positionX = BitmapPosition::Start;
    shiftedOptions.offsetX = 10.0f;
    shiftedOptions.blend = BitmapBlend::Replace;
    BlitBitmap(shiftedOptions);
    EXPECT_EQ(shifted, _Pixels({ _white, _white }));
}

TEST(Bitmap, BlitCopiesRowsIntoPartOfTheRect) {
    auto dst = Bytes(4 * 2 * 4, 0);
    auto const src = _Pixels({ _red, Pixel{ 0, 255, 0, 0 }, _blue, _white });
    auto options = _Options(dst, Vec2{ 4.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
    options.positionX = BitmapPosition::Center;
    options.blend = BitmapBlend::Replace;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({
        _clear, _red, Pixel{ 0, 255, 0, 0 }, _clear,
        _clear, _blue, _white, _clear
    }));
}

TEST(Bitmap, BlitCopiesFormatsWithoutAlphaWithNormal) {
    auto dst = Bytes{
        0, 0, 0, 0, 0, 0, 77,
        0, 0, 0, 0, 0, 0, 77
    };
    auto const src = Bytes{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    auto options = _Options(dst, Vec2{ 2.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
    options.dstFormat = BitmapFormat::RGB;
    options.dstBytesPerRow = 7;
    options.srcFormat = BitmapFormat::RGB;
    BlitBitmap(options);
    EXPECT_EQ(dst, (Bytes{
        1, 2, 3, 4, 5, 6, 77,
        7, 8, 9, 10, 11, 12, 77
    }));
}

TEST(Bitmap, BlitDrawsNothingForNonFiniteOrOutOfRangeGeometry) {
    auto const nan = std::numeric_limits<float>::quiet_NaN();
    auto const infinity = std::numeric_limits<float>::infinity();
    auto const src = _Pixels({ _red, _green, _blue, _white });

    for (auto const& change : std::initializer_list<void (*)(BitmapBlitOptions&, float, float)>{
        [](BitmapBlitOptions& options, float nan, float) { options.offsetX = nan; },
        [](BitmapBlitOptions& options, float, float infinity) { options.offsetY = infinity; options.repeatY = true; },
        [](BitmapBlitOptions& options, float, float) { options.srcRect = Vec4{ 1e20f, 0.0f, 2.0f, 2.0f }; },
        [](BitmapBlitOptions& options, float nan, float) { options.dstRect = Vec4{ nan, 0.0f, 2.0f, 2.0f }; },
        [](BitmapBlitOptions& options, float nan, float) { options.dstSize = Vec2{ nan, 2.0f }; },
        [](BitmapBlitOptions& options, float, float infinity) { options.srcRect = Vec4{ -infinity, 0.0f, 1.0f, 1.0f }; }
    }) {
        auto dst = _Pixels({ _white, _white, _white, _white });
        auto options = _Options(dst, Vec2{ 2.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
        change(options, nan, infinity);
        BlitBitmap(options);
        EXPECT_EQ(dst, _Pixels({ _white, _white, _white, _white }));
    }
}

TEST(Bitmap, BlitStaysInsideTheBitmapsForHugeOrNonFiniteGeometry) {
    auto const nan = std::numeric_limits<float>::quiet_NaN();
    auto const src = _Pixels({ _red, _green, _blue, _white });

    auto huge = Bytes(2 * 2 * 4, 0);
    auto hugeOptions = _Options(huge, Vec2{ 2.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
    hugeOptions.dstRect = Vec4{ -1e20f, -1e20f, 2e20f, 2e20f };
    BlitBitmap(hugeOptions);
    EXPECT_EQ(huge.size(), 16u);

    auto patched = Bytes(2 * 2 * 4, 0);
    auto patchedOptions = _Options(patched, Vec2{ 2.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
    patchedOptions.nPatch = Vec4{ nan, nan, nan, nan };
    patchedOptions.filter = BitmapFilter::Linear;
    BlitBitmap(patchedOptions);
    EXPECT_EQ(patched.size(), 16u);
}

TEST(Bitmap, BlitAlignsTheSourceAtItsOwnSize) {
    auto const src = _Pixels({ _red, _green });

    for (auto const& [ position, expected ] : {
        std::pair{ BitmapPosition::Start, _Pixels({ _red, _green, _clear, _clear, _clear, _clear }) },
        std::pair{ BitmapPosition::Center, _Pixels({ _clear, _clear, _red, _green, _clear, _clear }) },
        std::pair{ BitmapPosition::End, _Pixels({ _clear, _clear, _clear, _clear, _red, _green }) }
    }) {
        auto dst = Bytes(6 * 1 * 4, 0);
        auto options = _Options(dst, Vec2{ 6.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
        options.positionX = position;
        BlitBitmap(options);
        EXPECT_EQ(dst, expected);
    }

    auto dst = Bytes(1 * 4 * 4, 0);
    auto const column = _Pixels({ _red });
    auto options = _Options(dst, Vec2{ 1.0f, 4.0f }, column, Vec2{ 1.0f, 1.0f });
    options.positionY = BitmapPosition::End;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({ _clear, _clear, _clear, _red }));
}

TEST(Bitmap, BlitShiftsTheSourceByTheOffset) {
    auto dst = Bytes(4 * 2 * 4, 0);
    auto const src = _Pixels({ _red });
    auto options = _Options(dst, Vec2{ 4.0f, 2.0f }, src, Vec2{ 1.0f, 1.0f });
    options.positionX = BitmapPosition::Start;
    options.positionY = BitmapPosition::Start;
    options.offsetX = 2.0f;
    options.offsetY = 1.0f;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({
        _clear, _clear, _clear, _clear,
        _clear, _clear, _red, _clear
    }));
}

TEST(Bitmap, BlitRepeatsTheSource) {
    auto const src = _Pixels({ _red, _green });

    auto dst = Bytes(6 * 1 * 4, 0);
    auto options = _Options(dst, Vec2{ 6.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    options.positionX = BitmapPosition::Start;
    options.repeatX = true;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({ _red, _green, _red, _green, _red, _green }));

    auto shifted = Bytes(6 * 1 * 4, 0);
    auto shiftedOptions = _Options(shifted, Vec2{ 6.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    shiftedOptions.positionX = BitmapPosition::Start;
    shiftedOptions.repeatX = true;
    shiftedOptions.offsetX = 5.0f;
    BlitBitmap(shiftedOptions);
    EXPECT_EQ(shifted, _Pixels({ _green, _red, _green, _red, _green, _red }));

    auto column = Bytes(1 * 3 * 4, 0);
    auto const blue = _Pixels({ _blue });
    auto columnOptions = _Options(column, Vec2{ 1.0f, 3.0f }, blue, Vec2{ 1.0f, 1.0f });
    columnOptions.positionY = BitmapPosition::Start;
    columnOptions.repeatY = true;
    BlitBitmap(columnOptions);
    EXPECT_EQ(column, _Pixels({ _blue, _blue, _blue }));
}

TEST(Bitmap, BlitFlipsTheSource) {
    auto const row = _Pixels({ _red, _green, _blue });
    auto dst = Bytes(3 * 1 * 4, 0);
    auto options = _Options(dst, Vec2{ 3.0f, 1.0f }, row, Vec2{ 3.0f, 1.0f });
    options.flipX = true;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({ _blue, _green, _red }));

    auto column = Bytes(1 * 3 * 4, 0);
    auto columnOptions = _Options(column, Vec2{ 1.0f, 3.0f }, row, Vec2{ 1.0f, 3.0f });
    columnOptions.flipY = true;
    BlitBitmap(columnOptions);
    EXPECT_EQ(column, _Pixels({ _blue, _green, _red }));

    auto sliced = Bytes(2 * 1 * 4, 0);
    auto slicedOptions = _Options(sliced, Vec2{ 2.0f, 1.0f }, row, Vec2{ 3.0f, 1.0f });
    slicedOptions.srcRect = Vec4{ 1.0f, 0.0f, 2.0f, 1.0f };
    slicedOptions.flipX = true;
    BlitBitmap(slicedOptions);
    EXPECT_EQ(sliced, _Pixels({ _blue, _green }));
}

TEST(Bitmap, BlitKeepsNPatchBordersAtTheirSize) {
    auto const src = _Pixels({ _red, _green, _blue });

    auto stretched = Bytes(7 * 1 * 4, 0);
    BlitBitmap(_Options(stretched, Vec2{ 7.0f, 1.0f }, src, Vec2{ 3.0f, 1.0f }));
    EXPECT_EQ(stretched, _Pixels({ _red, _red, _green, _green, _green, _blue, _blue }));

    auto patched = Bytes(7 * 1 * 4, 0);
    auto options = _Options(patched, Vec2{ 7.0f, 1.0f }, src, Vec2{ 3.0f, 1.0f });
    options.nPatch = Vec4{ 1.0f, 0.0f, 1.0f, 0.0f };
    BlitBitmap(options);
    EXPECT_EQ(patched, _Pixels({ _red, _green, _green, _green, _green, _green, _blue }));
}

TEST(Bitmap, BlitTintsTheSourceWithTheColor) {
    auto dst = Bytes(2 * 1 * 4, 0);
    auto const src = Bytes{ 255, 255, 255, 128, 0, 0, 0, 0 };
    auto options = _Options(dst, Vec2{ 2.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    options.blend = BitmapBlend::Replace;
    options.color = Vec4{ 1.0f, 0.0f, 0.0f, 0.5f };
    BlitBitmap(options);
    EXPECT_EQ(dst, (Bytes{ 255, 0, 0, 64, 0, 0, 0, 0 }));

    auto untinted = Bytes(1 * 1 * 4, 0);
    auto untintedOptions = _Options(untinted, Vec2{ 1.0f, 1.0f }, src, Vec2{ 1.0f, 1.0f });
    untintedOptions.blend = BitmapBlend::Replace;
    untintedOptions.color = Vec4{ 1.0f, 0.0f, 0.0f, 0.0f };
    BlitBitmap(untintedOptions);
    EXPECT_EQ(untinted, (Bytes{ 255, 255, 255, 128 }));
}

TEST(Bitmap, BlitInterpolatesWithTheLinearFilter) {
    auto dst = Bytes(4 * 1 * 4, 0);
    auto const src = _Pixels({ Pixel{ 0, 0, 0, 255 }, _white });
    auto options = _Options(dst, Vec2{ 4.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    options.filter = BitmapFilter::Linear;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({
        Pixel{ 0, 0, 0, 255 },
        Pixel{ 64, 64, 64, 255 },
        Pixel{ 191, 191, 191, 255 },
        _white
    }));
}

TEST(Bitmap, BlitInterpolatesWithoutBleedingTheColorOfTransparentPixels) {
    auto dst = Bytes(4 * 1 * 4, 0);
    auto const src = _Pixels({ _red, Pixel{ 0, 255, 0, 0 } });
    auto options = _Options(dst, Vec2{ 4.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    options.filter = BitmapFilter::Linear;
    options.blend = BitmapBlend::Replace;
    BlitBitmap(options);
    EXPECT_EQ(_PixelAt(dst, 4, 1, 0), (Pixel{ 255, 0, 0, 191 }));
}

TEST(Bitmap, BlitInterpolatesOnlyInsideTheSourceRect) {
    auto dst = Bytes(2 * 1 * 4, 0);
    auto const src = _Pixels({ _red, _green, _blue });
    auto options = _Options(dst, Vec2{ 2.0f, 1.0f }, src, Vec2{ 3.0f, 1.0f });
    options.srcRect = Vec4{ 1.0f, 0.0f, 1.0f, 1.0f };
    options.filter = BitmapFilter::Linear;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({ _green, _green }));
}

TEST(Bitmap, BlitBlendsNormallyByDefault) {
    EXPECT_EQ(_BlendOne(_blue, (Pixel{ 255, 0, 0, 128 }), BitmapBlend::Normal), (Pixel{ 128, 0, 127, 255 }));
    EXPECT_EQ(_BlendOne(_clear, (Pixel{ 255, 0, 0, 128 }), BitmapBlend::Normal), (Pixel{ 255, 0, 0, 128 }));
    EXPECT_EQ(_BlendOne(_blue, (Pixel{ 255, 255, 255, 0 }), BitmapBlend::Normal), _blue);

    auto dst = _Pixels({ _blue });
    auto const src = _Pixels({ Pixel{ 255, 0, 0, 128 } });
    BlitBitmap(_Options(dst, Vec2{ 1.0f, 1.0f }, src, Vec2{ 1.0f, 1.0f }));
    EXPECT_EQ(dst, _Pixels({ Pixel{ 128, 0, 127, 255 } }));
}

TEST(Bitmap, BlitReplacesWithReplace) {
    EXPECT_EQ(_BlendOne(_blue, (Pixel{ 255, 255, 255, 0 }), BitmapBlend::Replace), (Pixel{ 255, 255, 255, 0 }));
    EXPECT_EQ(_BlendOne(_blue, (Pixel{ 255, 0, 0, 128 }), BitmapBlend::Replace), (Pixel{ 255, 0, 0, 128 }));
}

TEST(Bitmap, BlitBlendsWithEveryMode) {
    auto const backdrop = Pixel{ 102, 102, 102, 255 };
    auto const source = Pixel{ 153, 153, 153, 255 };
    auto const gray = [](std::uint8_t value) {
        return Pixel{ value, value, value, 255 };
    };

    EXPECT_EQ(_BlendOne(backdrop, source, BitmapBlend::Multiply), gray(61));
    EXPECT_EQ(_BlendOne(backdrop, source, BitmapBlend::Screen), gray(194));
    EXPECT_EQ(_BlendOne(backdrop, source, BitmapBlend::Overlay), gray(122));
    EXPECT_EQ(_BlendOne(source, backdrop, BitmapBlend::Overlay), gray(133));
    EXPECT_EQ(_BlendOne(backdrop, source, BitmapBlend::Add), gray(255));
    EXPECT_EQ(_BlendOne(backdrop, source, BitmapBlend::Darken), gray(102));
    EXPECT_EQ(_BlendOne(backdrop, source, BitmapBlend::Lighten), gray(153));
}

TEST(Bitmap, BlitBlendModesLeaveTheDestinationUnderATransparentSource) {
    for (auto const blend : {
        BitmapBlend::Normal,
        BitmapBlend::Multiply,
        BitmapBlend::Screen,
        BitmapBlend::Overlay,
        BitmapBlend::Add,
        BitmapBlend::Darken,
        BitmapBlend::Lighten
    }) {
        EXPECT_EQ(_BlendOne(_blue, (Pixel{ 255, 255, 255, 0 }), blend), _blue);
    }
}

TEST(Bitmap, BlitLeavesATransparentDestinationUnderATransparentSource) {
    for (auto const blend : {
        BitmapBlend::Normal,
        BitmapBlend::Multiply,
        BitmapBlend::Add
    }) {
        EXPECT_EQ(_BlendOne((Pixel{ 10, 20, 30, 0 }), (Pixel{ 255, 255, 255, 0 }), blend), (Pixel{ 10, 20, 30, 0 }));
    }
}

TEST(Bitmap, BlitBlendModesDrawOverATransparentDestinationAsNormal) {
    for (auto const blend : {
        BitmapBlend::Multiply,
        BitmapBlend::Screen,
        BitmapBlend::Overlay,
        BitmapBlend::Add,
        BitmapBlend::Darken,
        BitmapBlend::Lighten
    }) {
        EXPECT_EQ(_BlendOne(_clear, _green, blend), _green);
    }
}

TEST(Bitmap, BlitTreatsADestinationWithoutAlphaAsOpaque) {
    auto dst = Bytes{ 0, 0, 255 };
    auto const src = _Pixels({ Pixel{ 255, 0, 0, 128 } });
    auto options = _Options(dst, Vec2{ 1.0f, 1.0f }, src, Vec2{ 1.0f, 1.0f });
    options.dstFormat = BitmapFormat::RGB;
    BlitBitmap(options);
    EXPECT_EQ(dst, (Bytes{ 128, 0, 127 }));

    auto bgr = Bytes{ 255, 0, 0 };
    auto bgrOptions = _Options(bgr, Vec2{ 1.0f, 1.0f }, src, Vec2{ 1.0f, 1.0f });
    bgrOptions.dstFormat = BitmapFormat::BGR;
    BlitBitmap(bgrOptions);
    EXPECT_EQ(bgr, (Bytes{ 127, 0, 128 }));
}

TEST(Bitmap, BlitWritesLuminanceIntoAGrayscaleDestination) {
    auto dst = Bytes{ 0, 0 };
    auto const src = _Pixels({ _red, _white });
    auto options = _Options(dst, Vec2{ 2.0f, 1.0f }, src, Vec2{ 2.0f, 1.0f });
    options.dstFormat = BitmapFormat::Grayscale;
    BlitBitmap(options);
    EXPECT_EQ(dst, (Bytes{ 54, 255 }));
}

TEST(Bitmap, BlitReadsEverySourceFormat) {
    for (auto const& [ format, bytes ] : {
        std::pair{ BitmapFormat::Grayscale, Bytes{ 128 } },
        std::pair{ BitmapFormat::RGB, Bytes{ 128, 128, 128 } },
        std::pair{ BitmapFormat::BGR, Bytes{ 128, 128, 128 } },
        std::pair{ BitmapFormat::RGBA, Bytes{ 128, 128, 128, 255 } },
        std::pair{ BitmapFormat::BGRA, Bytes{ 128, 128, 128, 255 } }
    }) {
        auto dst = Bytes(4, 0);
        auto options = _Options(dst, Vec2{ 1.0f, 1.0f }, bytes, Vec2{ 1.0f, 1.0f });
        options.srcFormat = format;
        BlitBitmap(options);
        EXPECT_EQ(dst, (Bytes{ 128, 128, 128, 255 }));
    }

    auto dst = Bytes(4, 0);
    auto const bgra = Bytes{ 1, 2, 3, 255 };
    auto options = _Options(dst, Vec2{ 1.0f, 1.0f }, bgra, Vec2{ 1.0f, 1.0f });
    options.srcFormat = BitmapFormat::BGRA;
    options.dstFormat = BitmapFormat::RGBA;
    BlitBitmap(options);
    EXPECT_EQ(dst, (Bytes{ 3, 2, 1, 255 }));
}

TEST(Bitmap, BlitReadsAndWritesPaddedRows) {
    auto dst = Bytes{
        0, 0, 0, 0, 0, 0, 0, 0, 77, 77, 77, 77,
        0, 0, 0, 0, 0, 0, 0, 0, 77, 77, 77, 77
    };
    auto const src = Bytes{
        255, 0, 0, 255, 0, 255, 0, 255, 99, 99,
        0, 0, 255, 255, 255, 255, 255, 255, 99, 99
    };
    auto options = _Options(dst, Vec2{ 2.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
    options.dstBytesPerRow = 12;
    options.srcBytesPerRow = 10;
    BlitBitmap(options);
    EXPECT_EQ(dst, (Bytes{
        255, 0, 0, 255, 0, 255, 0, 255, 77, 77, 77, 77,
        0, 0, 255, 255, 255, 255, 255, 255, 77, 77, 77, 77
    }));
}

TEST(Bitmap, BlitReadsAndWritesRowsStoredBottomFirst) {
    auto dst = Bytes(1 * 2 * 4, 0);
    auto const src = _Pixels({ _green, _red });
    auto options = _Options(dst, Vec2{ 1.0f, 2.0f }, src, Vec2{ 1.0f, 2.0f });
    options.dstData = (dst.data() + 4);
    options.dstBytesPerRow = -4;
    options.srcData = (src.data() + 4);
    options.srcBytesPerRow = -4;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({ _green, _red }));
}

TEST(Bitmap, BlitWritesIntoARegionOfALargerBitmap) {
    auto dst = Bytes(3 * 3 * 4, 0);
    auto const src = _Pixels({ _red, _green, _blue, _white });
    auto options = _Options(dst, Vec2{ 2.0f, 2.0f }, src, Vec2{ 2.0f, 2.0f });
    options.dstData = (dst.data() + 16);
    options.dstBytesPerRow = 12;
    BlitBitmap(options);
    EXPECT_EQ(dst, _Pixels({
        _clear, _clear, _clear,
        _clear, _red, _green,
        _clear, _blue, _white
    }));
}

TEST(Bitmap, BlitWithinTheSameBufferReadsTheSourceAsItWas) {
    auto bytes = _Pixels({ _red, _green, _blue, _white });
    auto options = _Options(bytes, Vec2{ 4.0f, 1.0f }, bytes, Vec2{ 4.0f, 1.0f });
    options.srcRect = Vec4{ 0.0f, 0.0f, 3.0f, 1.0f };
    options.dstRect = Vec4{ 1.0f, 0.0f, 3.0f, 1.0f };
    BlitBitmap(options);
    EXPECT_EQ(bytes, _Pixels({ _red, _red, _green, _blue }));
}

TEST(Bitmap, BlitThrowsWithoutData) {
    auto bytes = _Pixels({ _red });
    auto withoutDestination = _Options(bytes, Vec2{ 1.0f, 1.0f }, bytes, Vec2{ 1.0f, 1.0f });
    withoutDestination.dstData = nullptr;
    EXPECT_THROW(BlitBitmap(withoutDestination), std::runtime_error);

    auto withoutSource = _Options(bytes, Vec2{ 1.0f, 1.0f }, bytes, Vec2{ 1.0f, 1.0f });
    withoutSource.srcData = nullptr;
    EXPECT_THROW(BlitBitmap(withoutSource), std::runtime_error);
}
