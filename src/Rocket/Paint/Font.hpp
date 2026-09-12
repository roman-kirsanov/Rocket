/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

/** Typographic style of a font face. */
enum class FontStyle {
    Normal,
    Italic
};

/**
 * Typographic weight of a font face. Enumerator values are declaration
 * order, not the CSS numbers; they map to the CSS 100-900 scale semantically
 * (Thin=100 through Black=900; Normal is 400).
 */
enum class FontWeight {
    Normal,
    Thin,
    ExtraLight,
    Light,
    Medium,
    SemiBold,
    Bold,
    ExtraBold,
    Black
};

/** Vertical metrics for a font at a given pixel size, derived from the font face metrics. */
struct FontLine {
    /** Distance from the baseline to the font's ascender line, in pixels. */
    float ascent;
    /** Distance from the baseline to the font's descender line, in pixels. */
    float descent;
    /** Line gap (extra space between lines), in pixels. */
    float leading;
    /** Underline position relative to baseline, in pixels. */
    float underline;
    /** Strikeout position relative to baseline, in pixels; not computed, currently always 0. */
    float strikeout;
    /** Recommended underline stroke thickness, in pixels. */
    float underlineThickness;
    /** Recommended strikeout stroke thickness, in pixels; not computed, currently always 0. */
    float strikeoutThickness;
};

/**
 * Rasterized glyph descriptor.
 *
 * The bitmap is packed into a shared GPU atlas image. imageSlice gives the
 * sub-region within that atlas. Glyphs that rasterize to no pixels (e.g.
 * space) have image == nullptr; whitespace is derived from the codepoint
 * alone and is independent of whether a bitmap exists.
 */
struct FontGlyph {
    /** Unicode codepoint this glyph represents. */
    std::uint32_t codepoint;
    /** Atlas image containing the rasterized bitmap, or nullptr when the glyph rasterized to no pixels. */
    Image const* image;
    /** Sub-region within the atlas image (x, y, width, height) in pixels. */
    Vec4 imageSlice;
    /** Horizontal and vertical bearing (bitmap offset from the glyph origin). */
    Vec2 bearing;
    /** Horizontal advance width in pixels. */
    float advance;
    /** True when the codepoint is a whitespace character with no visible pixels. */
    bool whitespace;
};

/**
 * A font face with glyph atlas caching.
 *
 * Non-copyable and non-movable. Glyphs are rasterized on demand and packed
 * into 1024x1024 GPU atlas images. Supports grayscale, monochrome, and
 * color bitmap formats.
 *
 * Use Find/Add to manage a program-wide font registry keyed by family name,
 * weight, and style; GetDefault returns built-in faces from a separate cache
 * that is not part of that registry.
 */
class Font {
public:
    ~Font();

    /**
     * Constructs a font from a file path; throws std::runtime_error if the
     * file does not exist or is not a valid font.
     *
     * @param path  Path to the font file.
     * @param index Face index within a TTC/OTC collection (default 0).
     */
    Font(std::string const& path, std::size_t index = 0);

    /**
     * Constructs a font from an in-memory buffer; throws std::runtime_error
     * if the data is not a valid font.
     *
     * @param data  Font file bytes.
     * @param size  Size of data in bytes.
     * @param index Face index within a TTC/OTC collection (default 0).
     */
    Font(std::uint8_t const* data, std::size_t size, std::size_t index = 0);

    Font(Font &&) = delete;
    Font(Font const&) = delete;
    Font& operator=(Font &&) = delete;
    Font& operator=(Font const&) = delete;

    /**
     * Returns the cached vertical metrics for the given pixel size, computing
     * them on first access.
     *
     * @param size Font size in pixels.
     */
    FontLine const& getLine(float size) const;

    /**
     * Returns the cached glyph for the given Unicode codepoint and pixel
     * size, rasterizing on first access.
     *
     * @param codepoint Unicode codepoint to rasterize.
     * @param size      Font size in pixels.
     */
    FontGlyph const& getGlyph(std::uint32_t codepoint, float size) const;

    /**
     * Returns the kerning adjustment between two consecutive codepoints at the given size.
     *
     * Returns 0 if the face has no kerning table.
     *
     * @param codepoint1 The preceding codepoint.
     * @param codepoint2 The following codepoint.
     * @param size       Font size in pixels.
     */
    float const& getKerning(std::uint32_t codepoint1, std::uint32_t codepoint2, float size) const;

    /** Returns the size adjustment in pixels added to every requested font size. */
    float const& getSizeAdjust() const;

    /** Returns true when monochrome (1-bit) rendering is enabled. */
    bool getMonochrome() const;

    /** Returns true when monochrome hinting is enabled. */
    bool getMonohinting() const;

    /**
     * Enables or disables monochrome (1-bit) bitmap rendering.
     *
     * @param monochrome True to enable monochrome rendering.
     */
    void setMonochrome(bool monochrome);

    /**
     * Enables or disables monochrome hinting.
     *
     * @param monohinting True to enable monochrome hinting.
     */
    void setMonohinting(bool monohinting);

    /**
     * Sets a size adjustment in pixels added to every requested font size
     * before glyph rasterization and metric computation.
     *
     * @param sizeAdjust Size delta in pixels (may be negative).
     */
    void setSizeAdjust(float sizeAdjust);

    /**
     * Returns the built-in default font (Inter) for the given weight and style, lazily loading it on first call.
     *
     * Weight selects the matching named instance of the embedded variable font;
     * FontStyle::Italic switches to the embedded Inter Italic companion file.
     *
     * @param weight Typographic weight to select (default FontWeight::Normal).
     * @param style  Typographic style to select (default FontStyle::Normal).
     */
    static Font const& GetDefault(FontWeight weight = FontWeight::Normal, FontStyle style = FontStyle::Normal);

    /**
     * Looks up a registered font by family name (case-insensitive), weight, and style.
     *
     * Returns nullptr if no matching font has been registered.
     *
     * @param family Font family name, matched case-insensitively.
     * @param weight Typographic weight to match (default FontWeight::Medium).
     * @param style  Typographic style to match (default FontStyle::Normal).
     */
    static Font const* Find(std::string const& family, FontWeight weight = FontWeight::Medium, FontStyle style = FontStyle::Normal);

    /**
     * Registers a font face from a file path under the given family name,
     * weight, and style, replacing any existing entry for that combination.
     *
     * Returns the registered font, allowing further tweaks (e.g. setSizeAdjust).
     *
     * @param family Font family name to register under.
     * @param weight Typographic weight to register under.
     * @param style  Typographic style to register under.
     * @param path   Path to the font file.
     * @param index  Face index within a TTC/OTC collection.
     */
    static Font& Add(std::string const& family, FontWeight weight, FontStyle style, std::string const& path, std::size_t index);

    /**
     * Registers a font face from an in-memory buffer under the given family
     * name, weight, and style, replacing any existing entry for that
     * combination.
     *
     * Returns the registered font, allowing further tweaks (e.g. setSizeAdjust).
     *
     * @param family Font family name to register under.
     * @param weight Typographic weight to register under.
     * @param style  Typographic style to register under.
     * @param data   Font file bytes.
     * @param size   Size of data in bytes.
     * @param index  Face index within a TTC/OTC collection.
     */
    static Font& Add(std::string const& family, FontWeight weight, FontStyle style, std::uint8_t const* data, std::size_t size, std::size_t index);
private:
    struct _Font;
    mutable std::vector<std::uint8_t> _data;
    mutable std::vector<std::unique_ptr<Image>> _images;
    mutable std::unordered_map<std::uint32_t, FontLine> _lineCache;
    mutable std::unordered_map<std::uint64_t, FontGlyph> _glyphCache;
    mutable std::unordered_map<std::uint64_t, float> _kerningCache;
    mutable std::unordered_map<std::uint32_t, std::uint32_t> _charIndexCache;
    mutable std::uint32_t _pixelSize;
    mutable std::int64_t _imageXOffset;
    mutable std::int64_t _imageYOffset;
    mutable std::int64_t _imageLineHeight;
    float _sizeAdjust;
    bool _monochrome;
    bool _monohinting;
    bool _hasKerning;
    int _fontIndex;
    _Font* _impl;

    std::uint32_t _getCharIndex(std::uint32_t) const;
    void _setPixelSize(std::uint32_t) const;
    void _initLib();
    void _initFont();
};

} /* namespace Rocket */
