/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <array>
#include <format>
#include <fstream>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_OTSVG_H
#include <lunasvg.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/String.hpp>
#include "./Fonts/Inter.hpp"
#include "./Fonts/InterItalic.hpp"
#include <Rocket/Paint/Font.hpp>

#define FT_Init_FreeType_E(...) { \
    if (auto const error = ::FT_Init_FreeType(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to initialize the FreeType library: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_New_Memory_Face_E(...) { \
    if (auto const error = ::FT_New_Memory_Face(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to create a freetype font face from memory: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_Done_Face_E(...) { \
    if (auto const error = ::FT_Done_Face(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to finalize a freetype font face: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_Load_Glyph_E(...) { \
    if (auto const error = ::FT_Load_Glyph(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to load a freetype glyph: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_Render_Glyph_E(...) { \
    if (auto const error = ::FT_Render_Glyph(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to render a freetype glyph: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_Set_Pixel_Sizes_E(...) { \
    if (auto const error = ::FT_Set_Pixel_Sizes(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to set a freetype glyph pixel size: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_Get_Kerning_E(...) { \
    if (auto const error = ::FT_Get_Kerning(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to get a freetype glyph kerning: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_Select_Charmap_E(...) { \
    if (auto const error = ::FT_Select_Charmap(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to select a freetype charmap: {}", ::FT_Error_String(error))); \
    } \
}

#define FT_Property_Set_E(...) { \
    if (auto const error = ::FT_Property_Set(__VA_ARGS__); error != 0) { \
        throw std::runtime_error(std::format("Failed to set a freetype property: {}", ::FT_Error_String(error))); \
    } \
}

namespace Rocket {

struct _FontKey {
    std::string family;
    FontWeight weight;
    FontStyle style;

    bool operator==(_FontKey const&) const = default;
};

struct _FontKeyHash {
    std::size_t operator()(_FontKey const& key) const noexcept {
        auto seed = std::hash<std::string>{}(key.family);
        seed ^= static_cast<std::size_t>(key.weight) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= static_cast<std::size_t>(key.style) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};

struct _FontSvgState {
    ::FT_Error err;
    lunasvg::Matrix matrix;
    std::unique_ptr<lunasvg::Document> svg;
};

struct _Font {
    ::FT_Face face;
};

static auto constexpr _fontImageWidth = 1024;
static auto constexpr _fontImageHeight = 1024;
static auto constexpr _fontImagePadding = 1;

static auto _freeTypeLibrary = ::FT_Library{};
static auto _freeTypeLibraryInited = false;
static auto _fontRegistry = std::unordered_map<_FontKey, Font, _FontKeyHash>();

static std::uint32_t _SizeToPixels(float size) {
    return static_cast<std::uint32_t>(std::max(size, 1.0f));
}

static void _FontSvgFree(::FT_Pointer* state) {
    PROFILE

    delete *reinterpret_cast<_FontSvgState**>(state);
}

static ::FT_Error _FontSvgNew(::FT_Pointer* state) {
    PROFILE

    *state = new _FontSvgState{};

    return ::FT_Err_Ok;
}

static ::FT_Error _FontSvgRender(::FT_GlyphSlot slot, ::FT_Pointer* state) {
    PROFILE

    auto svgState = *reinterpret_cast<_FontSvgState**>(state);

    if (svgState->err != ::FT_Err_Ok) {
        return svgState->err;
    }

    auto bitmap = lunasvg::Bitmap(
        reinterpret_cast<std::uint8_t*>(slot->bitmap.buffer),
        slot->bitmap.width,
        slot->bitmap.rows,
        slot->bitmap.pitch
    );

    svgState->svg->render(bitmap, svgState->matrix);
    svgState->err = ::FT_Err_Ok;

    return svgState->err;
}

static ::FT_Error _FontSvgPresetSlot(::FT_GlyphSlot slot, ::FT_Bool cache, ::FT_Pointer* state) {
    PROFILE

    auto svgState = *reinterpret_cast<_FontSvgState**>(state);
    auto document = reinterpret_cast<::FT_SVG_Document>(slot->other);

    if (cache != 0) {
        return svgState->err;
    }

    svgState->svg = lunasvg::Document::loadFromData(
        reinterpret_cast<const char*>(document->svg_document),
        document->svg_document_length
    );

    if (svgState->svg == nullptr) {
        svgState->err = ::FT_Err_Invalid_SVG_Document;
        return svgState->err;
    }

    auto box = svgState->svg->boundingBox();

    auto const scale =  std::min(document->metrics.x_ppem / box.w, document->metrics.y_ppem / box.h);
    auto const xx    =  (double)document->transform.xx / (1 << 16);
    auto const xy    = -(double)document->transform.xy / (1 << 16);
    auto const yx    = -(double)document->transform.yx / (1 << 16);
    auto const yy    =  (double)document->transform.yy / (1 << 16);
    auto const x0    =  (double)document->delta.x / 64.0 * box.w / document->metrics.x_ppem;
    auto const y0    = -(double)document->delta.y / 64.0 * box.h / document->metrics.y_ppem;

    svgState->matrix = lunasvg::Matrix::translated(-box.x, -box.y);
    svgState->matrix.multiply(lunasvg::Matrix(xx, xy, yx, yy, x0, y0));
    svgState->matrix.scale(scale, scale);
    box.transform(svgState->matrix);

    slot->bitmap_left        = ::FT_Int(box.x);
    slot->bitmap_top         = ::FT_Int(-box.y);
    slot->bitmap.rows        = (unsigned int)std::ceil(box.h);
    slot->bitmap.width       = (unsigned int)std::ceil(box.w);
    slot->bitmap.pitch       = (int)(slot->bitmap.width * 4);
    slot->bitmap.pixel_mode  = ::FT_PIXEL_MODE_BGRA;

    auto const vertBearingX = slot->metrics.horiBearingX / 64.0 - slot->metrics.horiAdvance / 64.0 / 2.0;
    auto const vertBearingY = (slot->metrics.vertAdvance / 64.0 - slot->metrics.height / 64.0) / 2.0;
    slot->metrics.width        = ::FT_Pos(std::round(box.w * 64.0));
    slot->metrics.height       = ::FT_Pos(std::round(box.h * 64.0));
    slot->metrics.horiBearingX = ::FT_Pos(box.x * 64.0);
    slot->metrics.horiBearingY = ::FT_Pos(-box.y * 64.0);
    slot->metrics.vertBearingX = ::FT_Pos(vertBearingX * 64.0);
    slot->metrics.vertBearingY = ::FT_Pos(vertBearingY * 64.0);

    if (slot->metrics.vertAdvance == 0) {
        slot->metrics.vertAdvance = ::FT_Pos(box.h * 1.2 * 64.0);
    }

    svgState->err = ::FT_Err_Ok;
    return svgState->err;
}

Font::Font(std::string const& path, std::size_t index)
    : _impl(new _Font{})
    , _data()
    , _images()
    , _lineCache()
    , _glyphCache()
    , _kerningCache()
    , _charIndexCache()
    , _pixelSize(0)
    , _imageXOffset(0)
    , _imageYOffset(0)
    , _imageLineHeight(0)
    , _sizeAdjust(0.0f)
    , _monochrome(false)
    , _monohinting(false)
    , _hasKerning(false)
    , _fontIndex(index)
{
    if (std::filesystem::is_regular_file(path)) {
        auto file = std::ifstream(path, std::ios::binary);
        auto data = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
        _data = std::move(data);
    } else {
        throw std::runtime_error("Failed to create a font object, file not found");
    }

    _initLib();
    _initFont();
}

Font::Font(std::uint8_t const* data, std::size_t size, std::size_t index)
    : _impl(new _Font{})
    , _data(data, data + size)
    , _images()
    , _lineCache()
    , _glyphCache()
    , _kerningCache()
    , _charIndexCache()
    , _pixelSize(0)
    , _imageXOffset(0)
    , _imageYOffset(0)
    , _imageLineHeight(0)
    , _sizeAdjust(0.0f)
    , _monochrome(false)
    , _monohinting(false)
    , _hasKerning(false)
    , _fontIndex(index)
{
    PROFILE

    assert(data != nullptr);
    assert(size > 0);

    _initLib();
    _initFont();
}

Font::~Font() {
    PROFILE

    ::FT_Done_Face(_impl->face);
    delete _impl;
}

float const& Font::getSizeAdjust() const {
    PROFILE

    return _sizeAdjust;
}

bool Font::getMonochrome() const {
    PROFILE

    return _monochrome;
}

bool Font::getMonohinting() const {
    PROFILE

    return _monohinting;
}

void Font::setMonochrome(bool monochrome) {
    PROFILE

    _monochrome = monochrome;
}

void Font::setMonohinting(bool monohinting) {
    PROFILE

    _monohinting = monohinting;
}

void Font::setSizeAdjust(float sizeAdjust) {
    PROFILE

    _sizeAdjust = sizeAdjust;
}

FontLine const& Font::getLine(float size) const {
    PROFILE

    auto sizeKey = _SizeToPixels(size + _sizeAdjust);

    auto it = _lineCache.find(sizeKey);

    if (it == _lineCache.end()) {
        _setPixelSize(sizeKey);

        auto ascent = std::abs(_impl->face->size->metrics.ascender >> 6);
        auto descent = std::abs(_impl->face->size->metrics.descender >> 6);
        auto height = std::abs(_impl->face->size->metrics.height >> 6);
        auto underline = ((::FT_MulFix(_impl->face->underline_position, _impl->face->size->metrics.y_scale) + 32) >> 6);
        auto underlineThickness = ((::FT_MulFix(_impl->face->underline_thickness, _impl->face->size->metrics.y_scale) + 32) >> 6);

        it = _lineCache.insert({ sizeKey, FontLine{
            .ascent = static_cast<float>(ascent),
            .descent = static_cast<float>(descent),
            .leading = static_cast<float>(height - descent - ascent),
            .underline = static_cast<float>(underline),
            .strikeout = 0.0f,
            .underlineThickness = static_cast<float>(underlineThickness),
            .strikeoutThickness = 0.0f
        }}).first;
    }

    return it->second;
}

FontGlyph const& Font::getGlyph(std::uint32_t codepoint, float size) const {
    PROFILE

    static thread_local auto _tempBitmap = std::vector<std::uint8_t>();

    auto sizeKey = _SizeToPixels(size + _sizeAdjust);
    auto key = ((static_cast<std::uint64_t>(sizeKey) << 32) | codepoint);

    auto it = _glyphCache.find(key);

    if (it == _glyphCache.end()) {
        auto index = _getCharIndex(codepoint);
        auto loadFlags = FT_LOAD_NO_HINTING | FT_LOAD_COLOR;
        auto renderFlags = ::FT_RENDER_MODE_LIGHT;

        if (_monochrome) {
            loadFlags |= FT_LOAD_MONOCHROME;
            renderFlags = ::FT_RENDER_MODE_MONO;
        }

        if (_monohinting) {
            loadFlags |= FT_LOAD_TARGET_MONO;
        }

        _setPixelSize(sizeKey);

        FT_Load_Glyph_E(_impl->face, index, loadFlags);
        FT_Render_Glyph_E(_impl->face->glyph, renderFlags);

        auto width = static_cast<int>(_impl->face->glyph->bitmap.width);
        auto height = static_cast<int>(_impl->face->glyph->bitmap.rows);
        auto advance = (_impl->face->glyph->metrics.horiAdvance / 64.0f);
        auto bearing = Vec2{
            static_cast<float>(_impl->face->glyph->bitmap_left),
            static_cast<float>(_impl->face->glyph->bitmap_top)
        };

        auto bitmapFormat = ImageDataFormat::Grayscale;
        auto bitmap = _impl->face->glyph->bitmap.buffer;

        if (_impl->face->glyph->bitmap.buffer != nullptr) {
            if (_impl->face->glyph->bitmap.pixel_mode == ::FT_PIXEL_MODE_BGRA) {
                bitmapFormat = ImageDataFormat::RGBA;
                _tempBitmap.resize(width * height * 4);
                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        auto src = _impl->face->glyph->bitmap.buffer + y * _impl->face->glyph->bitmap.pitch + x * 4;
                        auto dst = _tempBitmap.data() + (y * width + x) * 4;
                        dst[0] = src[2];
                        dst[1] = src[1];
                        dst[2] = src[0];
                        dst[3] = src[3];
                    }
                }
                bitmap = _tempBitmap.data();
            } else if (_impl->face->glyph->bitmap.pixel_mode == ::FT_PIXEL_MODE_MONO) {
                static auto const _monoLut = []{
                    auto lut = std::array<std::array<std::uint8_t, 8>, 256>{};
                    for (auto byte = 0; byte < 256; byte++) {
                        for (auto bit = 0; bit < 8; bit++) {
                            lut[byte][bit] = ((((byte >> (7 - bit)) & 1) != 0) ? 255 : 0);
                        }
                    }
                    return lut;
                }();

                _tempBitmap.resize(width * height);

                for (auto y = 0; y < height; y++) {
                    auto src = (_impl->face->glyph->bitmap.buffer + y * _impl->face->glyph->bitmap.pitch);
                    auto dst = (_tempBitmap.data() + y * width);
                    for (auto x = 0; x < width; x += 8) {
                        std::memcpy(dst + x, _monoLut[src[x / 8]].data(), std::min(8, width - x));
                    }
                }
                bitmap = _tempBitmap.data();
            } else if (_impl->face->glyph->bitmap.pitch != width) {
                _tempBitmap.resize(width * height);
                for (auto y = 0; y < height; y++) {
                    std::memcpy(_tempBitmap.data() + y * width, _impl->face->glyph->bitmap.buffer + y * _impl->face->glyph->bitmap.pitch, width);
                }
                bitmap = _tempBitmap.data();
            }
        }

        if (bitmap != nullptr) {
            auto availableWidth = (_fontImageWidth - _imageXOffset);
            auto availableHeight = (_fontImageHeight - _imageYOffset);

            if (
                (availableWidth < width) ||
                (availableHeight < height)
            ) {
                _imageXOffset = 0;
                _imageYOffset += _imageLineHeight;
                _imageLineHeight = 0;

                availableWidth = (_fontImageWidth - _imageXOffset);
                availableHeight = (_fontImageHeight - _imageYOffset);

                if (
                    (availableWidth < width) ||
                    (availableHeight < height)
                ) {
                    _imageXOffset = 0;
                    _imageYOffset = 0;
                    _imageLineHeight = 0;

                    _images.push_back(
                        std::make_unique<Image>(Vec2{ _fontImageWidth, _fontImageHeight })
                    );
                }
            }

            auto slice = Vec4{
                static_cast<float>(_imageXOffset),
                static_cast<float>(_imageYOffset),
                static_cast<float>(width),
                static_cast<float>(height)
            };

            _images.back()->setData(slice.size, bitmap, bitmapFormat, slice.origin);

            it = _glyphCache.insert({ key, FontGlyph{
                .codepoint = codepoint,
                .image = _images.back().get(),
                .imageSlice = slice,
                .bearing = bearing,
                .advance = advance,
                .whitespace = CodepointIsWhitespace(codepoint)
            }}).first;

            _imageXOffset += (width + _fontImagePadding);
            _imageLineHeight = std::max<std::int64_t>(_imageLineHeight, height + _fontImagePadding);
        } else {
            it = _glyphCache.insert({ key, FontGlyph{
                .codepoint = codepoint,
                .image = nullptr,
                .imageSlice = {},
                .bearing = bearing,
                .advance = advance,
                .whitespace = CodepointIsWhitespace(codepoint)
            }}).first;
        }
    }

    return it->second;
}

float const& Font::getKerning(std::uint32_t codepoint1, std::uint32_t codepoint2, float size) const {
    PROFILE

    static auto constexpr _noKerning = 0.0f;

    if (_hasKerning == false) {
        return _noKerning;
    }

    auto sizeKey = _SizeToPixels(size + _sizeAdjust);
    auto key = ((static_cast<std::uint64_t>(sizeKey) << 42)
        | (static_cast<std::uint64_t>(codepoint1 & 0x1FFFFF) << 21)
        | (static_cast<std::uint64_t>(codepoint2 & 0x1FFFFF)));

    auto it = _kerningCache.find(key);

    if (it == _kerningCache.end()) {
        auto value = ::FT_Vector{};
        auto index1 = _getCharIndex(codepoint1);
        auto index2 = _getCharIndex(codepoint2);

        _setPixelSize(sizeKey);

        FT_Get_Kerning_E(_impl->face, index1, index2, ::FT_KERNING_DEFAULT, &value);

        it = _kerningCache.insert({ key, (value.x / 64.0f) }).first;
    }

    return it->second;
}

Font const& Font::GetDefault(FontWeight weight, FontStyle style) {
    PROFILE

    static auto _defaultFonts = std::unordered_map<int, Font>();

    auto const instance = (
        weight == FontWeight::Thin ? 1 :
        weight == FontWeight::ExtraLight ? 2 :
        weight == FontWeight::Light ? 3 :
        weight == FontWeight::Medium ? 5 :
        weight == FontWeight::SemiBold ? 6 :
        weight == FontWeight::Bold ? 7 :
        weight == FontWeight::ExtraBold ? 8 :
        weight == FontWeight::Black ? 9 :
                                      4
    );

    auto const italic = (style == FontStyle::Italic);
    auto const key = (italic ? (instance | 0x100) : instance);

    auto it = _defaultFonts.find(key);

    if (it == _defaultFonts.end()) {
        it = _defaultFonts.try_emplace(
            key,
            (italic ? FONT_INTER_ITALIC_DATA : FONT_INTER_DATA),
            (italic ? FONT_INTER_ITALIC_SIZE : FONT_INTER_SIZE),
            (instance << 16)
        ).first;
    }

    return it->second;
}

Font const* Font::Find(std::string const& family, FontWeight weight, FontStyle style) {
    PROFILE

    auto const key = _FontKey{ StringToLower(family), weight, style };

    auto const it = _fontRegistry.find(key);

    if (it != _fontRegistry.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

Font& Font::Add(std::string const& family, FontWeight weight, FontStyle style, std::string const& path, std::size_t index) {
    PROFILE

    auto const key = _FontKey{ StringToLower(family), weight, style };

    _fontRegistry.erase(key);
    return _fontRegistry.try_emplace(key, path, index).first->second;
}

Font& Font::Add(std::string const& family, FontWeight weight, FontStyle style, std::uint8_t const* data, std::size_t size, std::size_t index) {
    PROFILE

    auto const key = _FontKey{ StringToLower(family), weight, style };

    _fontRegistry.erase(key);
    return _fontRegistry.try_emplace(key, data, size, index).first->second;
}

void Font::_initLib() {
    PROFILE

    if (_freeTypeLibraryInited == false) {
        _freeTypeLibraryInited = true;

        auto const svgHooks = ::SVG_RendererHooks{ _FontSvgNew, _FontSvgFree, _FontSvgRender, _FontSvgPresetSlot };
        auto const noStemDarkening = ::FT_Bool(0); // 0 means stem darkening is enabled

        FT_Init_FreeType_E(&_freeTypeLibrary);
        FT_Property_Set_E(_freeTypeLibrary, "ot-svg", "svg-hooks", &svgHooks);
        FT_Property_Set_E(_freeTypeLibrary, "cff", "no-stem-darkening", &noStemDarkening);
        FT_Property_Set_E(_freeTypeLibrary, "autofitter", "no-stem-darkening", &noStemDarkening);
    }
}

void Font::_initFont() {
    PROFILE

    _lineCache.reserve(128);
    _glyphCache.reserve(128);
    _charIndexCache.reserve(128);
    _images.emplace_back(
        std::make_unique<Image>(Vec2{ _fontImageWidth, _fontImageHeight })
    );

    FT_New_Memory_Face_E(_freeTypeLibrary, _data.data(), _data.size(), _fontIndex, &_impl->face);
    FT_Select_Charmap_E(_impl->face, ::FT_ENCODING_UNICODE);

    _hasKerning = FT_HAS_KERNING(_impl->face);
}

void Font::_setPixelSize(std::uint32_t size) const {
    PROFILE

    if (_pixelSize != size) {
        _pixelSize = size;

        FT_Set_Pixel_Sizes_E(_impl->face, 0, size);
    }
}

std::uint32_t Font::_getCharIndex(std::uint32_t codepoint) const {
    PROFILE

    auto it = _charIndexCache.find(codepoint);

    if (it == _charIndexCache.end()) {
        it = _charIndexCache.insert({ codepoint, ::FT_Get_Char_Index(_impl->face, codepoint) }).first;
    }

    return it->second;
}

} /* namespace Rocket */
