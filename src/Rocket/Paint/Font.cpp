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
#include <lunasvg.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/String.hpp>
#include <Rocket/Paint/Font.hpp>
#include "./Fonts/Inter.hpp"
#include "./Fonts/InterItalic.hpp"

#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_OTSVG_H

#define FT_Init_FreeType_E(...) { \
    if (auto __err = ::FT_Init_FreeType(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to initialize the FreeType library: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_New_Memory_Face_E(...) { \
    if (auto __err = ::FT_New_Memory_Face(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to create a freetype font face from memory: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_Done_Face_E(...) { \
    if (auto __err = ::FT_Done_Face(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to finalize a freetype font face: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_Load_Glyph_E(...) { \
    if (auto __err = ::FT_Load_Glyph(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to load a freetype glyph: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_Render_Glyph_E(...) { \
    if (auto __err = ::FT_Render_Glyph(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to render a freetype glyph: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_Set_Pixel_Sizes_E(...) { \
    if (auto __err = ::FT_Set_Pixel_Sizes(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to set a freetype glyph pixel size: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_Get_Kerning_E(...) { \
    if (auto __err = ::FT_Get_Kerning(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to get a freetype glyph kerning: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_Select_Charmap_E(...) { \
    if (auto __err = ::FT_Select_Charmap(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to select a freetype charmap: {}", ::FT_Error_String(__err))); \
    } \
}

#define FT_Property_Set_E(...) { \
    if (auto __err = ::FT_Property_Set(__VA_ARGS__)) { \
        throw std::runtime_error(std::format("Failed to set a freetype property: {}", ::FT_Error_String(__err))); \
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
        std::size_t seed = std::hash<std::string>{}(key.family);
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

static auto constexpr _FONT_IMAGE_WIDTH = 1024;
static auto constexpr _FONT_IMAGE_HEIGHT = 1024;
static auto constexpr _FONT_IMAGE_PADDING = 1;

static auto _freeTypeLibrary = ::FT_Library{};
static auto _freeTypeLibraryInited = false;
static auto _fontRegistry = std::unordered_map<_FontKey, Font, _FontKeyHash>();

static std::uint32_t _SizeToPixels(float size) {
    return static_cast<std::uint32_t>(std::max(size, 1.0f));
}

static void _FontSvgFree(FT_Pointer* state) {
    PROFILE

    delete *reinterpret_cast<_FontSvgState**>(state);
}

static FT_Error _FontSvgNew(FT_Pointer* state) {
    PROFILE

    *state = new _FontSvgState{};

    return FT_Err_Ok;
}

static FT_Error _FontSvgRender(FT_GlyphSlot slot, FT_Pointer* _state) {
    PROFILE

    auto* state = *reinterpret_cast<_FontSvgState**>(_state);
    if (state->err != FT_Err_Ok) {
        return state->err;
    }

    auto bitmap = lunasvg::Bitmap(
        reinterpret_cast<std::uint8_t*>(slot->bitmap.buffer),
        slot->bitmap.width,
        slot->bitmap.rows,
        slot->bitmap.pitch
    );

    state->svg->render(bitmap, state->matrix);
    state->err = FT_Err_Ok;

    return state->err;
}

static FT_Error _FontSvgPresetSlot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* _state) {
    PROFILE

    auto state    = *reinterpret_cast<_FontSvgState**>(_state);
    auto document = reinterpret_cast<FT_SVG_Document>(slot->other);
    auto& metrics = document->metrics;

    if (cache) {
        return state->err;
    }

    state->svg = lunasvg::Document::loadFromData(
        reinterpret_cast<const char*>(document->svg_document),
        document->svg_document_length
    );

    if (state->svg == nullptr) {
        state->err = FT_Err_Invalid_SVG_Document;
        return state->err;
    }

    auto box = state->svg->boundingBox();

    double scale =  std::min(metrics.x_ppem / box.w, metrics.y_ppem / box.h);
    double xx    =  (double)document->transform.xx / (1 << 16);
    double xy    = -(double)document->transform.xy / (1 << 16);
    double yx    = -(double)document->transform.yx / (1 << 16);
    double yy    =  (double)document->transform.yy / (1 << 16);
    double x0    =  (double)document->delta.x / 64.0 * box.w / metrics.x_ppem;
    double y0    = -(double)document->delta.y / 64.0 * box.h / metrics.y_ppem;

    state->matrix = lunasvg::Matrix::translated(-box.x, -box.y);
    state->matrix.multiply(lunasvg::Matrix(xx, xy, yx, yy, x0, y0));
    state->matrix.scale(scale, scale);
    box.transform(state->matrix);

    slot->bitmap_left        = FT_Int(box.x);
    slot->bitmap_top         = FT_Int(-box.y);
    slot->bitmap.rows        = (unsigned int)std::ceil(box.h);
    slot->bitmap.width       = (unsigned int)std::ceil(box.w);
    slot->bitmap.pitch       = (int)(slot->bitmap.width * 4);
    slot->bitmap.pixel_mode  = FT_PIXEL_MODE_BGRA;

    double horiBearingX = box.x;
    double horiBearingY = -box.y;
    double vertBearingX = slot->metrics.horiBearingX / 64.0 - slot->metrics.horiAdvance / 64.0 / 2.0;
    double vertBearingY = (slot->metrics.vertAdvance / 64.0 - slot->metrics.height / 64.0) / 2.0;
    slot->metrics.width        = FT_Pos(std::round(box.w * 64.0));
    slot->metrics.height       = FT_Pos(std::round(box.h * 64.0));
    slot->metrics.horiBearingX = FT_Pos(horiBearingX * 64);
    slot->metrics.horiBearingY = FT_Pos(horiBearingY * 64);
    slot->metrics.vertBearingX = FT_Pos(vertBearingX * 64);
    slot->metrics.vertBearingY = FT_Pos(vertBearingY * 64);
    if (slot->metrics.vertAdvance == 0) {
        slot->metrics.vertAdvance = FT_Pos(box.h * 1.2 * 64.0);
    }

    state->err = FT_Err_Ok;
    return state->err;
}

struct Font::_Font {
    ::FT_Face face;
};

Font::Font(std::string const& path, std::size_t index)
    : _data()
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
    , _impl(new _Font{})
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
    : _data(data, data + size)
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
    , _impl(new _Font{})
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

    static thread_local auto tempBitmap = std::vector<std::uint8_t>();

    auto sizeKey = _SizeToPixels(size + _sizeAdjust);
    auto key = ((static_cast<std::uint64_t>(sizeKey) << 32) | codepoint);

    auto it = _glyphCache.find(key);
    if (it == _glyphCache.end()) {
        auto index = _getCharIndex(codepoint);
        auto loadFlags = FT_LOAD_NO_HINTING | FT_LOAD_COLOR;
        auto renderFlags = FT_RENDER_MODE_LIGHT;

        if (_monochrome) {
            loadFlags |= FT_LOAD_MONOCHROME;
            renderFlags = FT_RENDER_MODE_MONO;
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
        auto bitmapRaw = _impl->face->glyph->bitmap.buffer;
        auto bitmap = bitmapRaw;

        if (_impl->face->glyph->bitmap.buffer != nullptr) {
            auto pitch = _impl->face->glyph->bitmap.pitch;
            if (_impl->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
                bitmapFormat = ImageDataFormat::RGBA; /* the copy loop below already swizzles BGRA to RGBA while de-pitching */
                tempBitmap.resize(width * height * 4);
                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        auto src = bitmapRaw + y * pitch + x * 4;
                        auto dst = tempBitmap.data() + (y * width + x) * 4;
                        dst[0] = src[2]; // R
                        dst[1] = src[1]; // G
                        dst[2] = src[0]; // B
                        dst[3] = src[3]; // A
                    }
                }
                bitmap = tempBitmap.data();
            } else if (_impl->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                static auto const _MONO_LUT = []{
                    auto lut = std::array<std::array<std::uint8_t, 8>, 256>{};
                    for (auto byte = 0; byte < 256; byte++) {
                        for (auto bit = 0; bit < 8; bit++) {
                            lut[byte][bit] = ((byte >> (7 - bit)) & 1) ? 255 : 0;
                        }
                    }
                    return lut;
                }();

                tempBitmap.resize(width * height);

                for (auto y = 0; y < height; y++) {
                    auto src = (bitmapRaw + y * pitch);
                    auto dst = (tempBitmap.data() + y * width);
                    for (auto x = 0; x < width; x += 8) {
                        std::memcpy(dst + x, _MONO_LUT[src[x / 8]].data(), std::min(8, width - x));
                    }
                }
                bitmap = tempBitmap.data();
            } else if (pitch != width) {
                tempBitmap.resize(width * height);
                for (auto y = 0; y < height; y++) {
                    std::memcpy(tempBitmap.data() + y * width, bitmapRaw + y * pitch, width);
                }
                bitmap = tempBitmap.data();
            }
        }

        if (bitmap != nullptr) {
            auto availableWidth = (_FONT_IMAGE_WIDTH - _imageXOffset);
            auto availableHeight = (_FONT_IMAGE_HEIGHT - _imageYOffset);

            if (
                (availableWidth < width) ||
                (availableHeight < height)
            ) {
                _imageXOffset = 0;
                _imageYOffset += _imageLineHeight;
                _imageLineHeight = 0;

                availableWidth = (_FONT_IMAGE_WIDTH - _imageXOffset);
                availableHeight = (_FONT_IMAGE_HEIGHT - _imageYOffset);

                if (
                    (availableWidth < width) ||
                    (availableHeight < height)
                ) {
                    _imageXOffset = 0;
                    _imageYOffset = 0;
                    _imageLineHeight = 0;

                    _images.push_back(
                        std::make_unique<Image>(Vec2{ _FONT_IMAGE_WIDTH, _FONT_IMAGE_HEIGHT })
                    );
                }
            }

            auto& image = _images.back();

            auto slice = Vec4{
                static_cast<float>(_imageXOffset),
                static_cast<float>(_imageYOffset),
                static_cast<float>(width),
                static_cast<float>(height)
            };

            image->setData(slice.size, bitmap, bitmapFormat, slice.origin);

            it = _glyphCache.insert({ key, FontGlyph{
                .codepoint = codepoint,
                .image = image.get(),
                .imageSlice = slice,
                .bearing = bearing,
                .advance = advance,
                .whitespace = CodepointIsWhitespace(codepoint)
            }}).first;

            _imageXOffset += (width + _FONT_IMAGE_PADDING);
            _imageLineHeight = std::max<std::int64_t>(_imageLineHeight, height + _FONT_IMAGE_PADDING);
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

    static auto constexpr _NO_KERNING = 0.0f;

    if (_hasKerning == false) {
        return _NO_KERNING;
    }

    auto sizeKey = _SizeToPixels(size + _sizeAdjust);
    auto key = ((static_cast<std::uint64_t>(sizeKey) << 42)
        | (static_cast<std::uint64_t>(codepoint1 & 0x1FFFFF) << 21)
        | (static_cast<std::uint64_t>(codepoint2 & 0x1FFFFF)));

    auto it = _kerningCache.find(key);
    if (it == _kerningCache.end()) {
        auto value = FT_Vector{};
        auto index1 = _getCharIndex(codepoint1);
        auto index2 = _getCharIndex(codepoint2);

        _setPixelSize(sizeKey);

        FT_Get_Kerning_E(_impl->face, index1, index2, FT_KERNING_DEFAULT, &value);

        it = _kerningCache.insert({ key, (value.x / 64.0f) }).first;
    }

    return it->second;
}

Font const& Font::GetDefault(FontWeight weight, FontStyle style) {
    PROFILE

    static auto _defaultFonts = std::unordered_map<int, Font>();

    auto instance = 4; // Regular
    switch (weight) {
        case FontWeight::Thin:       instance = 1; break;
        case FontWeight::ExtraLight: instance = 2; break;
        case FontWeight::Light:      instance = 3; break;
        case FontWeight::Normal:     instance = 4; break;
        case FontWeight::Medium:     instance = 5; break;
        case FontWeight::SemiBold:   instance = 6; break;
        case FontWeight::Bold:       instance = 7; break;
        case FontWeight::ExtraBold:  instance = 8; break;
        case FontWeight::Black:      instance = 9; break;
    }

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

        auto const svgHooks = SVG_RendererHooks{ _FontSvgNew, _FontSvgFree, _FontSvgRender, _FontSvgPresetSlot };
        auto const noStemDarkening = FT_Bool(0); // 0 means stem darkening is enabled

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
        std::make_unique<Image>(Vec2{ _FONT_IMAGE_WIDTH, _FONT_IMAGE_HEIGHT })
    );

    FT_New_Memory_Face_E(_freeTypeLibrary, _data.data(), _data.size(), _fontIndex, &_impl->face);
    FT_Select_Charmap_E(_impl->face, FT_ENCODING_UNICODE);

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
