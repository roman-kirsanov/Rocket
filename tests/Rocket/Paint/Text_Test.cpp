/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>
#include <functional>
#include <Rocket/Base/String.hpp>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Text.hpp>
#include <Rocket/Paint/Font.hpp>
#define ROCKET_SNAPSHOT_MODULE "Paint"
#include "../../_Snapshots/Snapshot.hpp"
#include <gtest/gtest.h>

using namespace Rocket;

static bool runCase(std::string const& name, int width, int height, std::function<void(Painter&, Image&)> const& render) {
    return _RunSnapshotCase(name, width, height, render);
}

/**
 * Paints a Text object's rendered image at the given origin, using the same
 * pattern as App's text compositing (App.cpp _renderNode), except Nearest
 * filtering is used instead of Linear for crisp, deterministic pixel
 * comparisons in these snapshots. Defensively guards against a null image
 * (e.g. an empty string) rather than assuming getImage() is always non-null.
 */
static void paintText(Painter& painter, Text& text, Vec2 const& origin) {
    auto const image = text.getImage();

    if (image == nullptr) {
        return;
    }

    auto const textRect = Vec4{ origin, text.getSize() };

    painter.paint(QuadShape{ textRect }, ImageBrush{
        .image     = image,
        .positionX = ImagePosition::Start,
        .positionY = ImagePosition::Start,
        .filterMag = ImageFilter::Nearest,
        .filterMin = ImageFilter::Nearest
    });
}

/**
 * Returns a layout-space point inside the left half of glyph `index`'s
 * advance cell, i.e. a point that places the caret AT that glyph's index
 * (a point past a glyph's horizontal midpoint would resolve to index + 1).
 */
static Vec2 pointAtGlyph(Text const& text, std::size_t index) {
    auto const& glyph = text.getGlyphs().at(index);
    return {
        glyph.rect.x + (glyph.rect.width * 0.25f),
        glyph.rect.y + (glyph.rect.height * 0.5f)
    };
}

static auto const pangram = std::string("The quick brown fox");
static auto const red     = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f };

/* --- sanity asserts (metrics, not snapshots) --- */

/* Each ImageDataFormat maps its channels into RGBA correctly on upload
   (alpha 255 sidesteps premultiplication). This pins the contract the font
   atlas relies on: a buffer tagged BGRA is swizzled exactly once. */
TEST(Image, SetDataChannelMappingRoundTrips) {
    auto const check = [](ImageDataFormat format, std::vector<std::uint8_t> const& input, std::vector<std::uint8_t> const& expected) {
        auto image = Image{ Vec2{ 1.0f, 1.0f } };
        image.setData(Vec2{ 1.0f, 1.0f }, input.data(), format);

        auto output = std::vector<std::uint8_t>();
        image.getData(output);
        ASSERT_TRUE(output == expected);
    };

    check(ImageDataFormat::RGBA,      { 10, 20, 30, 255 }, { 10, 20, 30, 255 });
    check(ImageDataFormat::BGRA,      { 30, 20, 10, 255 }, { 10, 20, 30, 255 });
    check(ImageDataFormat::RGB,       { 10, 20, 30 },      { 10, 20, 30, 255 });
    check(ImageDataFormat::BGR,       { 30, 20, 10 },      { 10, 20, 30, 255 });
    check(ImageDataFormat::Grayscale, { 200 },             { 0, 0, 0, 200 });
}

/* Uploaded pixels are premultiplied by alpha: c' = (c * a + 127) / 255. */
TEST(Image, SetDataPremultipliesAlpha) {
    auto image = Image{ Vec2{ 1.0f, 1.0f } };

    auto const input = std::vector<std::uint8_t>{ 255, 100, 0, 128 };
    image.setData(Vec2{ 1.0f, 1.0f }, input.data(), ImageDataFormat::RGBA);

    auto output = std::vector<std::uint8_t>();
    image.getData(output);
    ASSERT_TRUE((output == std::vector<std::uint8_t>{ 128, 50, 0, 128 }));
}

/* A visible glyph rasterizes into the atlas; a space produces no bitmap and
   is flagged as whitespace (the two properties are independent). */
TEST(Font, GlyphBitmapAndWhitespaceFlags) {
    auto const& font = Font::GetDefault(FontWeight::Normal, FontStyle::Normal);

    auto const& visible = font.getGlyph(U'A', 20.0f);
    ASSERT_TRUE(visible.image != nullptr);
    ASSERT_TRUE(visible.whitespace == false);

    auto const& space = font.getGlyph(U' ', 20.0f);
    ASSERT_TRUE(space.image == nullptr);
    ASSERT_TRUE(space.whitespace == true);
}

/* Underline metrics are converted from font units to pixels: for a sane face
   at 20px both the position and thickness must be a small fraction of the
   size (the pre-FT_MulFix bug produced values ~1024x too large). */
TEST(Font, UnderlineMetricsAreInPixels) {
    auto const& font = Font::GetDefault(FontWeight::Normal, FontStyle::Normal);
    auto const& line = font.getLine(20.0f);
    ASSERT_TRUE(std::fabs(line.underline) > 0.0f);
    ASSERT_TRUE(std::fabs(line.underline) < 20.0f);
    ASSERT_TRUE(line.underlineThickness > 0.0f);
    ASSERT_TRUE(line.underlineThickness < 20.0f);
}


/* Same string/style configured on two separate Text objects yields identical sizes. */
TEST(Text, SameStringStyleYieldsIdenticalSizes) {
    Text a;
    Text b;
    a.setString(pangram);
    a.setStyle(TextStyle{ .fontSize = 20.0f });
    b.setString(pangram);
    b.setStyle(TextStyle{ .fontSize = 20.0f });
    ASSERT_TRUE(a.getSize() == b.getSize());
}

/* A larger fontSize yields a strictly larger size (width and/or height) for the same string. */
TEST(Text, LargerFontSizeYieldsLargerSize) {
    Text small;
    Text large;
    small.setString(pangram);
    small.setStyle(TextStyle{ .fontSize = 12.0f });
    large.setString(pangram);
    large.setStyle(TextStyle{ .fontSize = 48.0f });
    ASSERT_TRUE(
        (large.getSize().width > small.getSize().width) ||
        (large.getSize().height > small.getSize().height)
    );
}

/* Word-wrapping a long string onto 2+ lines yields a strictly greater height than the same string unwrapped. */
TEST(Text, WordWrappingYieldsGreaterHeight) {
    auto const longString = std::string("The quick brown fox jumps over the lazy dog and runs into the forest");

    Text wrapped;
    Text unwrapped;
    wrapped.setString(longString);
    wrapped.setMaxWidth(150.0f);
    unwrapped.setString(longString);

    ASSERT_TRUE(wrapped.getSize().height > unwrapped.getSize().height);
}

/* --- layout regressions: explicit '\n' must not drag words to the next line --- */

TEST(Text, LayoutRegressionsExplicitNewlineBreak) {
    /* "aa bb\n cc": the break is exactly after the '\n' at index 5; "bb"
       stays on line 0. */
    Text text;
    text.setString("aa bb\n cc");
    ASSERT_TRUE(text.getLines().size() == 2);
    ASSERT_TRUE(text.getLines().at(0).endIndex == 6);
    ASSERT_TRUE(text.getLines().at(1).startIndex == 6);
}

TEST(Text, LayoutRegressionsBlankMiddleLine) {
    /* "a\n\nb": three lines, the middle one holds just the second '\n'. */
    Text text;
    text.setString("a\n\nb");
    ASSERT_TRUE(text.getLines().size() == 3);
    ASSERT_TRUE(text.getLines().at(1).rect.height > 0.0f);
    ASSERT_TRUE(text.getGlyphs().at(3).rowIndex == 2); /* 'b' sits on line 2 */
}

/* --- basic --- */

TEST(Text, TextBasic) {
    EXPECT_TRUE(runCase("text-basic", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- size --- */

TEST(Text, TextSize12) {
    EXPECT_TRUE(runCase("text-size-12", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontSize = 12.0f });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextSize24) {
    EXPECT_TRUE(runCase("text-size-24", 320, 80, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontSize = 24.0f });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextSize48) {
    EXPECT_TRUE(runCase("text-size-48", 640, 100, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontSize = 48.0f });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- weight (all 9 bundled Inter weights; italic is skipped, see note below) --- */

/*
 * NOTE: italic is intentionally not covered here. Font::GetDefault()
 * (src/Rocket/Paint/Font.cpp) ignores its FontStyle parameter entirely,
 * and no italic face is registered under any family name, so
 * Font::Find() always misses and falls back to the upright default.
 * Setting fontStyle = FontStyle::Italic on the bundled default font
 * therefore has zero visual effect. Adding a "text-italic" case would
 * misleadingly imply italic is supported when it currently is not.
 */

TEST(Text, TextWeightThin) {
    EXPECT_TRUE(runCase("text-weight-thin", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::Thin });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightExtralight) {
    EXPECT_TRUE(runCase("text-weight-extralight", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::ExtraLight });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightLight) {
    EXPECT_TRUE(runCase("text-weight-light", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::Light });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightNormal) {
    EXPECT_TRUE(runCase("text-weight-normal", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::Normal });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightMedium) {
    EXPECT_TRUE(runCase("text-weight-medium", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::Medium });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightSemibold) {
    EXPECT_TRUE(runCase("text-weight-semibold", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::SemiBold });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightBold) {
    EXPECT_TRUE(runCase("text-weight-bold", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::Bold });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightExtrabold) {
    EXPECT_TRUE(runCase("text-weight-extrabold", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::ExtraBold });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWeightBlack) {
    EXPECT_TRUE(runCase("text-weight-black", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .fontWeight = FontWeight::Black });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- color --- */

TEST(Text, TextColorRed) {
    EXPECT_TRUE(runCase("text-color-red", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .color = red });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextColorSemitransparent) {
    EXPECT_TRUE(runCase("text-color-semitransparent", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString(pangram);
        text.setStyle(TextStyle{ .color = Vec4{ 1.0f, 0.0f, 0.0f, 0.4f } });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- line spacing (Text exposes no line-height multiplier API; these two
 * cases instead demonstrate the font's default natural line spacing at
 * two different font sizes, named/commented to reflect that honestly) --- */

TEST(Text, TextLineheightTight) {
    EXPECT_TRUE(runCase("text-lineheight-tight", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString("Line one\nLine two");
        text.setStyle(TextStyle{ .fontSize = 12.0f });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextLineheightLoose) {
    EXPECT_TRUE(runCase("text-lineheight-loose", 256, 100, [&](Painter& painter, Image&) {
        Text text;
        text.setString("Line one\nLine two");
        text.setStyle(TextStyle{ .fontSize = 32.0f });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- wrap --- */

TEST(Text, TextWrap) {
    EXPECT_TRUE(runCase("text-wrap", 200, 160, [&](Painter& painter, Image&) {
        Text text;
        text.setString("The quick brown fox jumps over the lazy dog and runs into the forest");
        text.setMaxWidth(150.0f);
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextWrapLongword) {
    EXPECT_TRUE(runCase("text-wrap-longword", 200, 120, [&](Painter& painter, Image&) {
        /* A single unbroken "word" longer than maxWidth. Text's layout has an
         * "emergency wrap" path (Text.cpp _updateLayout) that force-breaks at
         * a character boundary once a non-whitespace glyph would overflow a
         * line that already has content on it — so this actually wraps onto
         * multiple lines rather than overflowing the box. */
        Text text;
        text.setString("Supercalifragilisticexpialidocious");
        text.setMaxWidth(100.0f);
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextOverflow) {
    EXPECT_TRUE(runCase("text-overflow", 100, 64, [&](Painter& painter, Image&) {
        /* setWidth is used as the same wrap limit as setMaxWidth internally
         * (Text.cpp: "limit = _width.value_or(_maxWidth.value_or(FLT_MAX))"),
         * so a fixed width smaller than the natural content width causes the
         * content to word-wrap within that width rather than being clipped. */
        Text text;
        text.setString("Hello World");
        text.setWidth(50.0f);
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- multiline --- */

TEST(Text, TextMultiline) {
    EXPECT_TRUE(runCase("text-multiline", 256, 100, [&](Painter& painter, Image&) {
        Text text;
        text.setString("Line one\nLine two\nLine three");
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- alignment (Justify is skipped: reserved/not yet implemented per Text.hpp) --- */

TEST(Text, TextAlignStart) {
    EXPECT_TRUE(runCase("text-align-start", 220, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString("Hello");
        text.setWidth(180.0f);
        text.setAlignment(TextAlignment::Start);
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextAlignCenter) {
    EXPECT_TRUE(runCase("text-align-center", 220, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString("Hello");
        text.setWidth(180.0f);
        text.setAlignment(TextAlignment::Center);
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

TEST(Text, TextAlignEnd) {
    EXPECT_TRUE(runCase("text-align-end", 220, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString("Hello");
        text.setWidth(180.0f);
        text.setAlignment(TextAlignment::End);
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- styled runs --- */

TEST(Text, TextRuns) {
    EXPECT_TRUE(runCase("text-runs", 256, 64, [&](Painter& painter, Image&) {
        /* "Hello World" — codepoints [6, 11) are "World"; a later-added style
         * range layers bold + red over just that sub-range. */
        Text text;
        text.setString("Hello World");
        text.addStyle(6, 11, TextStyle{ .fontWeight = FontWeight::Bold, .color = red });
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- unicode --- */

TEST(Text, TextUnicode) {
    EXPECT_TRUE(runCase("text-unicode", 320, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString("café ЩЁЛКНИ Ελληνικά");
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* --- empty --- */

TEST(Text, TextEmpty) {
    EXPECT_TRUE(runCase("text-empty", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString("");
        paintText(painter, text, { 8.0f, 8.0f });
    }));
}

/* Single-line mode sanitizes input line breaks into spaces and maps the
   vertical caret moves to the line edges; multi-line (the unset default)
   keeps both behaviours unchanged. */
TEST(Text, SingleLineModeSanitizesInputAndClampsVerticalMoves) {
    Text text;
    text.setEditable(true);
    text.setMultiLine(false);

    text.input("a\nb\r\nc\rd");
    ASSERT_TRUE(text.getString() == "a b c d");

    text.moveUp();
    text.input("X");
    ASSERT_TRUE(text.getString() == "Xa b c d");

    text.moveDown();
    text.input("Y");
    ASSERT_TRUE(text.getString() == "Xa b c dY");

    text.setMultiLine(std::nullopt);
    text.input("\n");
    ASSERT_TRUE(text.getString() == "Xa b c dY\n");
}

/* --- editing visuals: selection under text, caret over text --- */

TEST(Text, TextSelectionHighlight) {
    EXPECT_TRUE(runCase("text-selection-highlight", 256, 64, [&](Painter& painter, Image&) {
        auto const origin = Vec2{ 8.0f, 8.0f };

        Text text;
        text.setString(pangram);

        text.setEditable(true);
        text.moveWordRight(true);
        text.moveWordRight(true); /* "The quick" selected */

        /* selection rects under the text, then the text — the same stacking
           the document uses for editable nodes */
        for (auto const& rect : text.getSelectionRects()) {
            painter.paint(
                QuadShape{ Vec4{ (origin + rect.origin), rect.size } },
                ColorBrush{ .color = Vec4{ 0.4f, 0.6f, 1.0f, 0.4f } }
            );
        }

        paintText(painter, text, origin);
    }));
}

TEST(Text, TextCaret) {
    EXPECT_TRUE(runCase("text-caret", 256, 64, [&](Painter& painter, Image&) {
        auto const origin = Vec2{ 8.0f, 8.0f };

        Text text;
        text.setString(pangram);

        text.setEditable(true);
        text.moveWordRight();
        text.moveWordRight(); /* collapsed caret after "quick" */

        /* text first, caret quad over it */
        paintText(painter, text, origin);

        auto const& rect = text.getCaretRect();
        painter.paint(
            QuadShape{ Vec4{ (origin + rect.origin), rect.size } },
            ColorBrush{ .color = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f } }
        );
    }));
}

/* --- clipped scrolled content: partial glyphs at the clip edges --- */

TEST(Text, TextClippedScroll) {
    EXPECT_TRUE(runCase("text-clipped-scroll", 256, 64, [&](Painter& painter, Image&) {
        Text text;
        text.setString("Pack my box with five dozen liquor jugs and judge my vow");
        text.setMaxWidth(200.0f);

        auto const image = text.getImage();
        ASSERT_TRUE(image != nullptr);

        /* scrolled up 10px and left 12px inside a clip window: glyph rows and
           columns are cut mid-shape at every clip edge */
        painter.paint(
            QuadShape{ Vec4{ Vec2{ -4.0f, -2.0f }, text.getSize() } },
            ImageBrush{
                .image     = image,
                .positionX = ImagePosition::Start,
                .positionY = ImagePosition::Start,
                .filterMag = ImageFilter::Nearest,
                .filterMin = ImageFilter::Nearest
            },
            PaintOptions{ .scissor = Vec4{ 8.0f, 8.0f, 200.0f, 40.0f } }
        );
    }));
}

/* ===================== interactive editing ===================== */

/* --- fresh state --- */

TEST(Text, EditingFreshState) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);
    ASSERT_TRUE(text.getCodepoints().size() == 5);
}

/* --- setString resets, empty string clamps caret to 0 --- */

TEST(Text, EditingSetStringResetsEmptyStringClampsCaret) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveRight();
    text.moveRight();
    text.setString("");
    ASSERT_TRUE(text.getString() == "");
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);
    ASSERT_TRUE(text.getCodepoints().empty());
}

/* --- input into empty text --- */

TEST(Text, EditingInputIntoEmptyText) {
    Text text;
    text.setString("x");

    text.setEditable(true);
    text.setString("");
    text.input("hi");
    ASSERT_TRUE(text.getString() == "hi");
    ASSERT_TRUE(text.getCaretPosition() == 2);
    ASSERT_TRUE(text.getAnchorPosition() == 2);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- input in the middle --- */

TEST(Text, EditingInputInTheMiddle) {
    Text text;
    text.setString("ac");

    text.setEditable(true);
    text.moveRight();
    text.input("b");
    ASSERT_TRUE(text.getString() == "abc");
    ASSERT_TRUE(text.getCaretPosition() == 2);
    ASSERT_TRUE(text.getAnchorPosition() == 2);
}

/* --- input replacing a selection --- */

TEST(Text, EditingInputReplacingSelection) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 1);
    ASSERT_TRUE(text.getCaretPosition() == 4);
    ASSERT_TRUE(text.isSelectedRange() == true);

    text.input("ipp");
    ASSERT_TRUE(text.getString() == "hippo");
    ASSERT_TRUE(text.getCaretPosition() == 4);
    ASSERT_TRUE(text.getAnchorPosition() == 4);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- moveLeft / moveRight with clamping --- */

TEST(Text, EditingMoveLeftMoveRightWithClamping) {
    Text text;
    text.setString("ab");

    text.setEditable(true);
    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.moveRight();
    text.moveRight();
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 2);
    ASSERT_TRUE(text.getAnchorPosition() == 2);

    text.moveLeft(true);
    ASSERT_TRUE(text.getCaretPosition() == 1);
    ASSERT_TRUE(text.getAnchorPosition() == 2);
    ASSERT_TRUE(text.isSelectedRange() == true);
}

/* --- selection: extend, rects, collapse on plain move --- */

TEST(Text, EditingSelectionExtendRectsCollapseOnPlainMove) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 3);
    ASSERT_TRUE(text.isSelectedRange() == true);
    ASSERT_TRUE(text.getSelectionRects().empty() == false);

    /* Plain moveRight collapses to max(caret, anchor) with no extra step. */
    text.moveRight();
    ASSERT_TRUE(text.isSelectedRange() == false);
    ASSERT_TRUE(text.getCaretPosition() == 3);
    ASSERT_TRUE(text.getAnchorPosition() == 3);
    ASSERT_TRUE(text.getSelectionRects().empty() == true);
}

/* --- selection collapse: moveLeft to min, moveRight to max, incl. buffer edges --- */

TEST(Text, EditingSelectionCollapseToMinMaxInclBufferEdges) {
    Text text;
    text.setString("abc");

    text.setEditable(true);
    text.selectAll();
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 3);

    /* Collapse to max even when the caret already sits at the buffer end. */
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 3);
    ASSERT_TRUE(text.getAnchorPosition() == 3);
    ASSERT_TRUE(text.isSelectedRange() == false);

    text.selectAll();
    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);

    /* Reversed selection (caret < anchor) collapses the same way. */
    text.moveRight();
    text.moveRight();
    text.moveLeft(true);
    text.moveLeft(true);
    ASSERT_TRUE(text.getAnchorPosition() == 2);
    ASSERT_TRUE(text.getCaretPosition() == 0);
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 2);
    ASSERT_TRUE(text.getAnchorPosition() == 2);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- selection collapse: word moves collapse to an edge, then move --- */

TEST(Text, EditingSelectionCollapseWordMovesCollapseToEdge) {
    Text text;
    text.setString("hello world");

    text.setEditable(true);
    /* Selection 6..11 ("world"); plain moveWordLeft collapses to min (6) first,
       then moves one word left from there. */
    text.moveWordRight();
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 6);
    text.moveWordRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 6);
    ASSERT_TRUE(text.getCaretPosition() == 11);
    text.moveWordLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);

    /* Selection 0..5 ("hello"); plain moveWordRight collapses to max (5) first. */
    text.moveWordRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 5);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 11);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- selection collapse: line and vertical moves collapse to an edge, then move --- */

TEST(Text, EditingSelectionCollapseLineMovesCollapseToEdge) {
    Text text;
    text.setString("one\ntwo");

    text.setEditable(true);
    /* Selection 1..5; plain moveLineEnd collapses to max (5, on line two) first. */
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 1);
    ASSERT_TRUE(text.getCaretPosition() == 5);
    text.moveLineEnd();
    ASSERT_TRUE(text.getCaretPosition() == 7);
    ASSERT_TRUE(text.isSelectedRange() == false);

    /* Selection 1..5 again; plain moveLineStart collapses to min (1, line one). */
    text.moveDocumentStart();
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 1);
    ASSERT_TRUE(text.getCaretPosition() == 5);
    text.moveLineStart();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

TEST(Text, EditingSelectionCollapseVerticalMovesCollapseToEdge) {
    Text text;
    text.setString("one\ntwo\nthree");

    text.setEditable(true);
    /* Selection 4..8; plain moveUp collapses to min (4) then moves up. */
    text.moveRight();
    text.moveRight();
    text.moveRight();
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 4);
    ASSERT_TRUE(text.getCaretPosition() == 8);
    text.moveUp();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);

    /* Selection 4..8; plain moveDown collapses to max (8) then moves down. */
    text.moveRight();
    text.moveRight();
    text.moveRight();
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 4);
    ASSERT_TRUE(text.getCaretPosition() == 8);
    text.moveDown();
    ASSERT_TRUE(text.getCaretPosition() == 13); /* last line: down goes to the text end */
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- moveWordRight / moveWordLeft --- */

TEST(Text, EditingMoveWordRightMoveWordLeft) {
    /* "hello world  foo": word ends at 5, 11 (then 2 spaces), 16. */
    Text text;
    text.setString("hello world  foo");

    text.setEditable(true);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 5);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 11);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 16);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 16); /* no-op at end */

    text.moveWordLeft();
    ASSERT_TRUE(text.getCaretPosition() == 13); /* start of "foo" */
    text.moveWordLeft();
    ASSERT_TRUE(text.getCaretPosition() == 6);  /* start of "world" */
    text.moveWordLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    text.moveWordLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);  /* no-op at start */

    /* From mid-word. */
    text.moveRight();
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 2);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 5);
    text.moveLeft();
    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 3);
    text.moveWordLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);

    /* Word movement with selection keeps the anchor. */
    text.moveWordRight(true);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 5);
    ASSERT_TRUE(text.isSelectedRange() == true);
}

/* --- word ops: punctuation is its own run --- */

TEST(Text, EditingWordOpsPunctuationIsItsOwnRun) {
    /* "foo, bar": f0 o1 o2 ,3 sp4 b5 a6 r7. */
    Text text;
    text.setString("foo, bar");

    text.setEditable(true);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 3); /* end of "foo", before the ',' */
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 4); /* past the punctuation run */
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 8); /* end of "bar" */

    text.moveWordLeft();
    ASSERT_TRUE(text.getCaretPosition() == 5); /* start of "bar" */

    /* deleteWordBackward from the end deletes only "bar". */
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 8);
    text.deleteWordBackward();
    ASSERT_TRUE(text.getString() == "foo, ");
    ASSERT_TRUE(text.getCaretPosition() == 5);

    /* Another deleteWordBackward removes the ", " punctuation run. */
    text.deleteWordBackward();
    ASSERT_TRUE(text.getString() == "foo");
    ASSERT_TRUE(text.getCaretPosition() == 3);
}

TEST(Text, EditingWordOpsPunctuationWithoutWhitespace) {
    /* "foo.bar": no whitespace, the '.' is still a run boundary. */
    Text text;
    text.setString("foo.bar");

    text.setEditable(true);
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 3); /* end of "foo" */
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 4); /* past the '.' */
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 7); /* end of "bar" */
}

/* --- moveLineStart / moveLineEnd --- */

TEST(Text, EditingMoveLineStartMoveLineEnd) {
    /* "one\ntwo\nthree": '\n' at 3 and 7, end at 13. */
    Text text;
    text.setString("one\ntwo\nthree");

    text.setEditable(true);
    text.moveRight();
    text.moveRight();
    text.moveLineEnd();
    ASSERT_TRUE(text.getCaretPosition() == 3); /* before the '\n' */
    text.moveLineStart();
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.moveLineEnd();
    text.moveRight(); /* onto line two */
    ASSERT_TRUE(text.getCaretPosition() == 4);
    text.moveLineEnd();
    ASSERT_TRUE(text.getCaretPosition() == 7);
    text.moveLineStart();
    ASSERT_TRUE(text.getCaretPosition() == 4);

    text.moveLineEnd();
    text.moveRight(); /* onto line three */
    ASSERT_TRUE(text.getCaretPosition() == 8);
    text.moveLineEnd();
    ASSERT_TRUE(text.getCaretPosition() == 13); /* no trailing '\n' on the last line */
}

/* --- moveUp / moveDown --- */

TEST(Text, EditingMoveUpMoveDown) {
    Text text;
    text.setString("one\ntwo\nthree");

    text.setEditable(true);
    text.moveDown();
    ASSERT_TRUE(text.getCaretPosition() == 4); /* start of line two */
    text.moveDown();
    ASSERT_TRUE(text.getCaretPosition() == 8); /* start of line three */
    text.moveDown();
    ASSERT_TRUE(text.getCaretPosition() == 13); /* last line: down goes to the text end */

    text.moveLineStart();
    ASSERT_TRUE(text.getCaretPosition() == 8);
    text.moveUp();
    ASSERT_TRUE(text.getCaretPosition() == 4);
    text.moveUp();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    text.moveUp();
    ASSERT_TRUE(text.getCaretPosition() == 0); /* first line: up goes to 0 */
}

/* --- moveUp / moveDown goal column --- */

TEST(Text, EditingMoveUpMoveDownGoalColumn) {
    /* "abcdefgh\nxy\nabcdefgh": '\n' at 8 and 11, line 2 starts at 12. */
    Text text;
    text.setString("abcdefgh\nxy\nabcdefgh");

    text.setEditable(true);
    for (int i = 0; i < 6; i += 1) {
        text.moveRight();
    }
    ASSERT_TRUE(text.getCaretPosition() == 6); /* line 0, column 6 */

    text.moveDown();
    ASSERT_TRUE(text.getCaretPosition() == 11); /* clamped to the end of short line "xy" */

    text.moveDown();
    ASSERT_TRUE(text.getCaretPosition() == 18); /* goal column 6 restored on line 2 */

    /* A horizontal move resets the goal column. */
    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 17); /* line 2, column 5 */
    text.moveUp();
    ASSERT_TRUE(text.getCaretPosition() == 11); /* clamped on "xy" again */
    text.moveUp();
    ASSERT_TRUE(text.getCaretPosition() == 5); /* new goal column is 5, not 6 */
}

/* --- deleteForward / deleteBackward --- */

TEST(Text, EditingDeleteForwardDeleteBackward) {
    Text text;
    text.setString("abc");

    text.setEditable(true);
    text.deleteBackward(); /* no-op at start */
    ASSERT_TRUE(text.getString() == "abc");
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.moveRight();
    text.deleteBackward();
    ASSERT_TRUE(text.getString() == "bc");
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.deleteForward();
    ASSERT_TRUE(text.getString() == "c");
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.moveRight();
    text.deleteForward(); /* no-op at end */
    ASSERT_TRUE(text.getString() == "c");
    ASSERT_TRUE(text.getCaretPosition() == 1);
}

/* --- deleteForward / deleteBackward with a selection --- */

TEST(Text, EditingDeleteForwardDeleteBackwardWithSelection) {
    Text text;
    text.setString("abcde");

    text.setEditable(true);
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true);
    text.deleteForward(); /* deletes "bc" */
    ASSERT_TRUE(text.getString() == "ade");
    ASSERT_TRUE(text.getCaretPosition() == 1);
    ASSERT_TRUE(text.isSelectedRange() == false);

    text.moveRight(true);
    text.deleteBackward(); /* deletes "d" */
    ASSERT_TRUE(text.getString() == "ae");
    ASSERT_TRUE(text.getCaretPosition() == 1);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- deleteWordForward / deleteWordBackward --- */

TEST(Text, EditingDeleteWordForwardDeleteWordBackward) {
    Text text;
    text.setString("hello world");

    text.setEditable(true);
    text.deleteWordForward();
    ASSERT_TRUE(text.getString() == " world");
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.setString("hello world");
    text.moveWordRight();
    text.moveWordRight();
    ASSERT_TRUE(text.getCaretPosition() == 11);
    text.deleteWordBackward();
    ASSERT_TRUE(text.getString() == "hello ");
    ASSERT_TRUE(text.getCaretPosition() == 6);

    /* With a selection, only the selection is deleted. */
    text.setString("hello world");
    text.moveDocumentStart();
    text.moveRight(true);
    text.moveRight(true);
    text.deleteWordForward();
    ASSERT_TRUE(text.getString() == "llo world");
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- selectAll + copy + cut round trip --- */

TEST(Text, EditingSelectAllCopyCutRoundTrip) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.selectAll();
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 5);
    ASSERT_TRUE(text.isSelectedRange() == true);

    auto copied = std::string();
    text.copy(copied);
    ASSERT_TRUE(copied == "hello");
    ASSERT_TRUE(text.getString() == "hello"); /* copy does not modify the text */
    ASSERT_TRUE(text.isSelectedRange() == true);

    auto cutOut = std::string();
    text.cut(cutOut);
    ASSERT_TRUE(cutOut == "hello");
    ASSERT_TRUE(text.getString() == "");
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);

    /* copy with no selection clears the out string. */
    auto stale = std::string("stale");
    text.copy(stale);
    ASSERT_TRUE(stale == "");
}

/* --- copy of a partial selection --- */

TEST(Text, EditingCopyOfPartialSelection) {
    Text text;
    text.setString("hello world");

    text.setEditable(true);
    text.moveWordRight();
    text.moveRight();
    text.moveWordRight(true); /* selects "world" */
    ASSERT_TRUE(text.getAnchorPosition() == 6);
    ASSERT_TRUE(text.getCaretPosition() == 11);

    auto copied = std::string();
    text.copy(copied);
    ASSERT_TRUE(copied == "world");
    ASSERT_TRUE(text.getString() == "hello world");
}

/* --- paste replacing a selection --- */

TEST(Text, EditingPasteReplacingSelection) {
    Text text;
    text.setString("abc");

    text.setEditable(true);
    text.moveRight();
    text.moveRight(true); /* selects "b" */
    text.paste("XY");
    ASSERT_TRUE(text.getString() == "aXYc");
    ASSERT_TRUE(text.getCaretPosition() == 3);
    ASSERT_TRUE(text.getAnchorPosition() == 3);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- paste of an empty string is a no-op, even over a selection --- */

TEST(Text, EditingPasteOfEmptyStringIsNoOp) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.selectAll();
    text.paste("");
    ASSERT_TRUE(text.getString() == "hello");
    ASSERT_TRUE(text.isSelectedRange() == true);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 5);
}

/* --- unicode: codepoints, not bytes --- */

TEST(Text, EditingUnicodeCodepointsNotBytes) {
    Text text;
    text.setString("café");

    text.setEditable(true);
    ASSERT_TRUE(text.getCodepoints().size() == 4); /* 4 codepoints, 5 UTF-8 bytes */
    ASSERT_TRUE(text.getCodepoints().at(3) == 0x00E9);

    text.moveRight();
    text.moveRight();
    text.moveRight();
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 4);
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 4); /* clamped at 4 codepoints, not 5 bytes */

    text.deleteBackward(); /* removes the whole 'é' codepoint */
    ASSERT_TRUE(text.getString() == "caf");
    ASSERT_TRUE(text.getCaretPosition() == 3);

    text.input("é");
    ASSERT_TRUE(text.getString() == "café"); /* UTF-8 round-trips */
    ASSERT_TRUE(text.getCaretPosition() == 4);
}

/* --- grapheme clusters: combining marks move and delete as one unit --- */

TEST(Text, EditingGraphemeClustersCombiningMarks) {
    /* "é" as U+0065 U+0301: two codepoints, one grapheme cluster. The
       combining mark is spelled as an escape so editors cannot silently
       normalize the literal to the precomposed form. */
    Text text;
    text.setString("e\u0301");

    text.setEditable(true);
    ASSERT_TRUE(text.getCodepoints().size() == 2);

    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 2); /* one step crosses both codepoints */
    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.moveRight();
    text.deleteBackward(); /* deletes the whole cluster */
    ASSERT_TRUE(text.getString() == "");
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.setString("e\u0301");
    text.deleteForward(); /* forward delete also takes the whole cluster */
    ASSERT_TRUE(text.getString() == "");
    ASSERT_TRUE(text.getCaretPosition() == 0);
}

/* --- grapheme clusters: ZWJ emoji sequences and flag pairs --- */

TEST(Text, EditingGraphemeClustersZwjEmojiSequences) {
    /* Family emoji U+1F468 U+200D U+1F469 U+200D U+1F467: 5 codepoints. */
    auto const family = std::string("\U0001F468\u200D\U0001F469\u200D\U0001F467");

    Text text;
    text.setString("a" + family + "b");

    text.setEditable(true);
    ASSERT_TRUE(text.getCodepoints().size() == 7);

    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 1);
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 6); /* one step crosses all 5 codepoints */
    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 7);

    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 6);
    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 1);
    text.moveLeft();
    ASSERT_TRUE(text.getCaretPosition() == 0);

    text.moveRight();
    text.moveRight();
    text.deleteBackward(); /* removes all 5 codepoints of the cluster */
    ASSERT_TRUE(text.getString() == "ab");
    ASSERT_TRUE(text.getCaretPosition() == 1);
}

TEST(Text, EditingGraphemeClustersFlagPair) {
    /* Flag pair U+1F1FA U+1F1F8 (regional indicators): 2 codepoints, one cluster. */
    Text text;
    text.setString("\U0001F1FA\U0001F1F8");

    text.setEditable(true);
    ASSERT_TRUE(text.getCodepoints().size() == 2);

    text.moveRight();
    ASSERT_TRUE(text.getCaretPosition() == 2);

    text.deleteBackward();
    ASSERT_TRUE(text.getString() == "");
    ASSERT_TRUE(text.getCaretPosition() == 0);
}

/* --- UTF-8 round trip: emoji survive getString() after edits --- */

TEST(Text, EditingUtf8RoundTripEmojiSurviveEdits) {
    auto const family = std::string("\U0001F468\u200D\U0001F469\u200D\U0001F467");

    Text text;
    text.setString("x");

    text.setEditable(true);
    text.setString("");
    text.input(family);
    ASSERT_TRUE(text.getString() == family); /* supplementary-plane UTF-8 round-trips */
    ASSERT_TRUE(text.getCaretPosition() == 5);

    text.input("a");
    ASSERT_TRUE(text.getString() == family + "a");

    text.deleteBackward(); /* removes the 'a' */
    ASSERT_TRUE(text.getString() == family);

    text.deleteBackward(); /* removes the whole emoji cluster */
    ASSERT_TRUE(text.getString() == "");
    ASSERT_TRUE(text.getCaretPosition() == 0);
}

/* --- moveDocumentStart / moveDocumentEnd --- */

TEST(Text, EditingMoveDocumentStartMoveDocumentEnd) {
    Text text;
    text.setString("one\ntwo");

    text.setEditable(true);
    text.moveDocumentEnd();
    ASSERT_TRUE(text.getCaretPosition() == 7);
    ASSERT_TRUE(text.getAnchorPosition() == 7);
    ASSERT_TRUE(text.isSelectedRange() == false);

    text.moveDocumentStart();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);

    /* With selection = true the anchor stays (shift-extend). */
    text.moveRight();
    text.moveRight();
    text.moveRight();
    text.moveDocumentEnd(true);
    ASSERT_TRUE(text.getAnchorPosition() == 3);
    ASSERT_TRUE(text.getCaretPosition() == 7);
    ASSERT_TRUE(text.isSelectedRange() == true);

    text.moveDocumentStart(true);
    ASSERT_TRUE(text.getAnchorPosition() == 3);
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == true);

    /* With an active selection a plain call collapses and moves. */
    text.moveDocumentStart();
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);

    text.selectAll();
    text.moveDocumentEnd();
    ASSERT_TRUE(text.getCaretPosition() == 7);
    ASSERT_TRUE(text.getAnchorPosition() == 7);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- caret rect sanity --- */

TEST(Text, EditingCaretRectSanity) {
    Text text;
    text.setString("abc");

    text.setEditable(true);
    auto const rectAt0 = text.getCaretRect();
    ASSERT_TRUE(rectAt0.width > 0.0f);

    text.moveRight();
    auto const rectAt1 = text.getCaretRect();
    ASSERT_TRUE(rectAt1.width > 0.0f);
    ASSERT_TRUE(rectAt1.x > rectAt0.x);
}

/* --- trailing newline creates a visible empty last line --- */

TEST(Text, EditingTrailingNewlineCreatesVisibleEmptyLastLine) {
    Text text;
    text.setString("abc\n");

    text.setEditable(true);
    ASSERT_TRUE(text.getLines().size() == 2); /* second line is empty but present */

    text.moveDocumentEnd();
    ASSERT_TRUE(text.getCaretPosition() == 4);
    ASSERT_TRUE(text.getCaretRect().y == text.getLines().at(1).rect.y);
    ASSERT_TRUE(text.getCaretRect().x == text.getLines().at(1).rect.x);

    text.input("x");
    ASSERT_TRUE(text.getString() == "abc\nx");
    ASSERT_TRUE(text.getGlyphs().at(4).rowIndex == 1); /* the 'x' lands on line 1 */
}

/* --- soft-wrap caret affinity at the visual line end --- */

TEST(Text, EditingSoftWrapCaretAffinityAtVisualLineEnd) {
    Text text;
    text.setString("aaa bbb ccc ddd eee");
    auto const unwrappedWidth = text.getSize().x;
    text.setMaxWidth(unwrappedWidth * 0.4f);
    ASSERT_TRUE(text.getLines().size() >= 2); /* the max width forces a soft wrap */

    text.setEditable(true);
    text.moveRight();
    text.moveRight(); /* caret mid-first-visual-line */

    text.moveLineEnd();
    auto const lineEndIndex = static_cast<std::int64_t>(text.getLines().at(0).endIndex);
    ASSERT_TRUE(text.getCaretPosition() == lineEndIndex);
    /* Affinity keeps the caret rendered on line 0, not at the start of line 1. */
    ASSERT_TRUE(text.getCaretRect().y == text.getLines().at(0).rect.y);

    /* A second moveLineEnd does not advance further. */
    text.moveLineEnd();
    ASSERT_TRUE(text.getCaretPosition() == lineEndIndex);
    ASSERT_TRUE(text.getCaretRect().y == text.getLines().at(0).rect.y);

    /* moveLineStart returns to line 0's start (affinity kept the row). */
    text.moveLineStart();
    ASSERT_TRUE(text.getCaretPosition() == static_cast<std::int64_t>(text.getLines().at(0).startIndex));
}

/* --- caret geometry re-derives itself after an external layout change --- */

TEST(Text, EditingCaretRectRecomputesAfterLayoutChange) {
    Text text;
    text.setString("aaaa bbbb");

    text.setEditable(true);
    text.moveDocumentEnd();
    ASSERT_TRUE(text.getCaretPosition() == 9);
    auto const rectBefore = text.getCaretRect();

    /* Force a re-wrap; the caret geometry re-derives itself on the next read,
       with no explicit refresh call. */
    auto const unwrappedWidth = text.getSize().x;
    text.setMaxWidth(unwrappedWidth * 0.6f);
    ASSERT_TRUE(text.getLines().size() >= 2);

    auto const rectAfter = text.getCaretRect();
    ASSERT_TRUE(rectAfter.y > rectBefore.y); /* the caret's word wrapped to a lower line */
}

/* --- mouse: down places caret, drag selects, up ends the drag --- */

TEST(Text, EditingMouseDownPlacesCaretDragSelectsUpEndsDrag) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.mouseDown(pointAtGlyph(text, 1));
    ASSERT_TRUE(text.getCaretPosition() == 1);
    ASSERT_TRUE(text.getAnchorPosition() == 1);
    ASSERT_TRUE(text.isSelectedRange() == false);

    text.mouseMove(pointAtGlyph(text, 3));
    ASSERT_TRUE(text.getCaretPosition() == 3);
    ASSERT_TRUE(text.getAnchorPosition() == 1);
    ASSERT_TRUE(text.isSelectedRange() == true);
    ASSERT_TRUE(text.getSelectionRects().empty() == false);

    text.mouseUp(pointAtGlyph(text, 3));
    text.mouseMove(pointAtGlyph(text, 0)); /* no drag in progress: caret unchanged */
    ASSERT_TRUE(text.getCaretPosition() == 3);
    ASSERT_TRUE(text.getAnchorPosition() == 1);
}

/* --- mouse: glyph midpoint decides which side the caret lands on --- */

TEST(Text, EditingMouseGlyphMidpointDecidesCaretSide) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    auto const& glyph = text.getGlyphs().at(2);
    auto const centerY = glyph.rect.y + (glyph.rect.height * 0.5f);

    /* Past the midpoint rounds to after the glyph. */
    text.mouseDown({glyph.rect.x + (glyph.rect.width * 0.9f), centerY});
    text.mouseUp({glyph.rect.x + (glyph.rect.width * 0.9f), centerY});
    ASSERT_TRUE(text.getCaretPosition() == 3);

    /* Before the midpoint rounds to the glyph itself. */
    text.mouseDown({glyph.rect.x + (glyph.rect.width * 0.1f), centerY});
    text.mouseUp({glyph.rect.x + (glyph.rect.width * 0.1f), centerY});
    ASSERT_TRUE(text.getCaretPosition() == 2);
}

/* --- mouse: clicking beyond a line's right edge stops before the '\n' --- */

TEST(Text, EditingMouseClickBeyondLineRightEdgeStopsBeforeNewline) {
    Text text;
    text.setString("one\ntwo");

    text.setEditable(true);
    auto const& line = text.getLines().at(0);
    auto const point = Vec2{
        line.rect.x + line.rect.width + 50.0f,
        line.rect.y + (line.rect.height * 0.5f)
    };
    text.mouseDown(point);
    text.mouseUp(point);
    ASSERT_TRUE(text.getCaretPosition() == 3); /* before the '\n', not 4 */
}

/* --- mouse: extend keeps the anchor (shift-click) --- */

TEST(Text, EditingMouseExtendKeepsAnchor) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.mouseDown(pointAtGlyph(text, 1));
    text.mouseUp(pointAtGlyph(text, 1));
    ASSERT_TRUE(text.getCaretPosition() == 1);
    ASSERT_TRUE(text.getAnchorPosition() == 1);

    text.mouseDown(pointAtGlyph(text, 4), /*extend=*/true);
    ASSERT_TRUE(text.getAnchorPosition() == 1);
    ASSERT_TRUE(text.getCaretPosition() == 4);
    ASSERT_TRUE(text.isSelectedRange() == true);
    text.mouseUp(pointAtGlyph(text, 4));
}

/* --- selection rects: no zero-width rect at a line boundary --- */

TEST(Text, EditingSelectionRectsNoZeroWidthRectAtLineBoundary) {
    Text text;
    text.setString("one\ntwo");

    text.setEditable(true);
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true);
    text.moveRight(true); /* anchor 0, caret 4: selection ends exactly at a line boundary */
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 4);
    ASSERT_TRUE(text.getSelectionRects().empty() == false);
    for (auto const& rect : text.getSelectionRects()) {
        ASSERT_TRUE(rect.width > 0.0f);
    }
}

/* --- snapshot / restore: text and caret --- */

TEST(Text, EditingSnapshotRestoreTextAndCaret) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveRight();
    text.moveRight();

    auto const snapshot = text.getSnapshot();
    text.input("XY");
    ASSERT_TRUE(text.getString() == "heXYllo");
    ASSERT_TRUE(text.getCaretPosition() == 4);
    ASSERT_TRUE(text.getCodepoints() != snapshot.codepoints);

    text.restoreSnapshot(snapshot);
    ASSERT_TRUE(text.getString() == "hello");
    ASSERT_TRUE(text.getCaretPosition() == 2);
    ASSERT_TRUE(text.getAnchorPosition() == 2);
    ASSERT_TRUE(text.isSelectedRange() == false);
}

/* --- snapshot / restore: the selection (anchor and caret) --- */

TEST(Text, EditingSnapshotRestoreSelection) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true); /* selection 1..3 */

    auto const snapshot = text.getSnapshot();
    text.deleteBackward(); /* deletes the selection */
    ASSERT_TRUE(text.getString() == "hlo");
    ASSERT_TRUE(text.isSelectedRange() == false);

    text.restoreSnapshot(snapshot);
    ASSERT_TRUE(text.getString() == "hello");
    ASSERT_TRUE(text.getAnchorPosition() == 1);
    ASSERT_TRUE(text.getCaretPosition() == 3);
    ASSERT_TRUE(text.isSelectedRange() == true);
}

/* --- snapshot: a no-op edit leaves the codepoints equal --- */

TEST(Text, EditingSnapshotNoOpEditLeavesCodepointsEqual) {
    Text text;
    text.setString("ab");

    text.setEditable(true);
    auto const snapshot = text.getSnapshot();

    text.deleteBackward(); /* caret at 0: no-op */
    ASSERT_TRUE(text.getCodepoints() == snapshot.codepoints);
    ASSERT_TRUE(text.getString() == "ab");

    text.paste(""); /* empty paste: no-op */
    ASSERT_TRUE(text.getCodepoints() == snapshot.codepoints);
    ASSERT_TRUE(text.getString() == "ab");
}

/* --- snapshot / restore: cut round trip --- */

TEST(Text, EditingSnapshotRestoreCutRoundTrip) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.selectAll();

    auto const snapshot = text.getSnapshot();
    auto string = std::string();
    text.cut(string);
    ASSERT_TRUE(string == "hello");
    ASSERT_TRUE(text.getString() == "");

    text.restoreSnapshot(snapshot);
    ASSERT_TRUE(text.getString() == "hello");
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getCaretPosition() == 5);
    ASSERT_TRUE(text.isSelectedRange() == true);
}

/* --- editing: style ranges follow edits --- */

/* Typing before a bounded style range shifts the range, so the style stays
   on the characters it was applied to. */
TEST(Text, EditingStyleRangeShiftsOnInsertBefore) {
    Text text;
    text.setString("abcdef");
    text.addStyle(2, 4, TextStyle{ .color = red }); /* "cd" */

    text.setEditable(true);
    text.input("XY"); /* "XYabcdef", styled run is still "cd" */

    auto const styles = text.getSnapshot().styles;
    ASSERT_TRUE(styles.size() == 1);
    ASSERT_TRUE(styles.at(0).startIndex == 4);
    ASSERT_TRUE(styles.at(0).endIndex == 6);

    auto const& glyphs = text.getGlyphs();
    ASSERT_TRUE((glyphs.at(3).color == red) == false);
    ASSERT_TRUE(glyphs.at(4).color == red);
    ASSERT_TRUE(glyphs.at(5).color == red);
    ASSERT_TRUE((glyphs.at(6).color == red) == false);
}

/* Typing at the end of a styled run continues the run (inherit-left), typing
   exactly at its start is pushed out of it. */
TEST(Text, EditingStyleRangeInheritsLeftAtBoundaries) {
    Text atEnd;
    atEnd.setString("abcdef");
    atEnd.addStyle(0, 3, TextStyle{ .color = red }); /* "abc" */
    atEnd.setEditable(true);
    atEnd.moveRight();
    atEnd.moveRight();
    atEnd.moveRight();
    atEnd.input("Z"); /* "abcZdef" */
    ASSERT_TRUE(atEnd.getSnapshot().styles.at(0).endIndex == 4);
    ASSERT_TRUE(atEnd.getGlyphs().at(3).color == red);

    Text atStart;
    atStart.setString("abcdef");
    atStart.addStyle(2, 4, TextStyle{ .color = red }); /* "cd" */
    atStart.setEditable(true);
    atStart.moveRight();
    atStart.moveRight();
    atStart.input("Z"); /* "abZcdef" */
    ASSERT_TRUE(atStart.getSnapshot().styles.at(0).startIndex == 3);
    ASSERT_TRUE(atStart.getSnapshot().styles.at(0).endIndex == 5);
    ASSERT_TRUE((atStart.getGlyphs().at(2).color == red) == false);
    ASSERT_TRUE(atStart.getGlyphs().at(3).color == red);
}

/* At index 0 there is no character to inherit from, so a range starting at 0
   absorbs text typed at the very start instead of being pushed past it. */
TEST(Text, EditingStyleRangeAtDocumentStartAbsorbsTypingAtZero) {
    Text text;
    text.setString("abcdef");
    text.addStyle(0, 3, TextStyle{ .color = red }); /* "abc" */

    text.setEditable(true);
    text.input("Z"); /* "Zabcdef" */

    auto const styles = text.getSnapshot().styles;
    ASSERT_TRUE(styles.at(0).startIndex == 0);
    ASSERT_TRUE(styles.at(0).endIndex == 4);
    ASSERT_TRUE(text.getGlyphs().at(0).color == red);
}

/* Deleting inside a range shrinks it; a delete overlapping its edges clamps
   it to the surviving styled characters. */
TEST(Text, EditingStyleRangeShrinksAndClampsOnDelete) {
    Text inside;
    inside.setString("abcdef");
    inside.addStyle(2, 5, TextStyle{ .color = red }); /* "cde" */
    inside.setEditable(true);
    inside.moveRight();
    inside.moveRight();
    inside.moveRight();
    inside.deleteForward(); /* removes "d" → "abcef" */
    ASSERT_TRUE(inside.getSnapshot().styles.at(0).startIndex == 2);
    ASSERT_TRUE(inside.getSnapshot().styles.at(0).endIndex == 4);

    Text overlap;
    overlap.setString("abcdef");
    overlap.addStyle(2, 5, TextStyle{ .color = red }); /* "cde" */
    overlap.setEditable(true);
    overlap.moveRight(true);
    overlap.moveRight(true);
    overlap.moveRight(true);
    overlap.deleteForward(); /* removes "abc" → "def", styled run is now "de" */
    ASSERT_TRUE(overlap.getString() == "def");
    ASSERT_TRUE(overlap.getSnapshot().styles.at(0).startIndex == 0);
    ASSERT_TRUE(overlap.getSnapshot().styles.at(0).endIndex == 2);
    ASSERT_TRUE(overlap.getGlyphs().at(0).color == red);
    ASSERT_TRUE(overlap.getGlyphs().at(1).color == red);
    ASSERT_TRUE((overlap.getGlyphs().at(2).color == red) == false);
}

/* Deleting every character a range covers drops the range entirely. */
TEST(Text, EditingStyleRangeDroppedWhenFullyDeleted) {
    Text text;
    text.setString("abcdef");
    text.addStyle(2, 4, TextStyle{ .color = red }); /* "cd" */

    text.setEditable(true);
    text.moveRight();
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true);
    text.deleteForward(); /* removes the selected "cd" → "abef" */

    ASSERT_TRUE(text.getString() == "abef");
    ASSERT_TRUE(text.getSnapshot().styles.empty());

    for (auto const& glyph : text.getGlyphs()) {
        ASSERT_TRUE((glyph.color == red) == false);
    }
}

/* Whole-string styles (no bounds) keep covering the entire text through any
   edit, including typing at position 0. */
TEST(Text, EditingWholeStringStyleSurvivesEdits) {
    Text text;
    text.setString("abc");
    text.setStyle(TextStyle{ .color = red });

    text.setEditable(true);
    text.input("XY");          /* at 0 */
    text.moveDocumentEnd();
    text.input("Z");           /* at end */
    text.moveLeft();
    text.deleteBackward();     /* in the middle */

    for (auto const& glyph : text.getGlyphs()) {
        ASSERT_TRUE(glyph.color == red);
    }
}

/* Snapshots capture the style ranges and restoreSnapshot brings them back
   together with the text. */
TEST(Text, EditingSnapshotRestoresStyles) {
    Text text;
    text.setString("abcdef");
    text.addStyle(2, 4, TextStyle{ .color = red }); /* "cd" */

    text.setEditable(true);

    auto const snapshot = text.getSnapshot();
    ASSERT_TRUE(snapshot.styles.size() == 1);
    ASSERT_TRUE(snapshot.styles.at(0).startIndex == 2);
    ASSERT_TRUE(snapshot.styles.at(0).endIndex == 4);
    ASSERT_TRUE(snapshot.styles.at(0).style.color == red);

    text.selectAll();
    text.deleteBackward(); /* empties the text and drops the range */
    ASSERT_TRUE(text.getSnapshot().styles.empty());

    text.restoreSnapshot(snapshot);
    ASSERT_TRUE(text.getString() == "abcdef");
    ASSERT_TRUE(text.getSnapshot().styles.size() == 1);
    ASSERT_TRUE(text.getSnapshot().styles.at(0).startIndex == 2);
    ASSERT_TRUE(text.getSnapshot().styles.at(0).endIndex == 4);
    ASSERT_TRUE(text.getGlyphs().at(2).color == red);
    ASSERT_TRUE((text.getGlyphs().at(4).color == red) == false);
}

/* --- setString: re-syncs the codepoint buffer and resets the edit state --- */

TEST(Text, EditingSetStringResyncsCodepointBufferAndClampsCaret) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveDocumentEnd();

    text.setString("hi");
    ASSERT_TRUE(text.getCodepoints().size() == 2);
    ASSERT_TRUE(text.getCaretPosition() == 2); /* a real change clamps the caret to the new length */
    ASSERT_TRUE(text.getAnchorPosition() == 2);

    text.input("!");
    ASSERT_TRUE(text.getString() == "hi!");

    text.moveLeft();
    text.setString("HI!");
    ASSERT_TRUE(text.getCaretPosition() == 2); /* a same-length rewrite keeps the caret in place */
}

/* --- setString: re-setting the same value leaves the caret alone, which is
       what lets a layout pass re-assert the current content while typing --- */

TEST(Text, EditingSetStringWithSameValueKeepsCaret) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveRight();
    text.moveRight();

    text.setString("hello");
    ASSERT_TRUE(text.getCaretPosition() == 2);

    text.input("!");
    ASSERT_TRUE(text.getString() == "he!llo");
}

/* --- secure: rendering masks, string and editing stay real --- */

TEST(Text, EditingSecureRenderingMasksStringAndEditingStayReal) {
    Text text;
    text.setString("secret");
    text.setSecure(true);
    ASSERT_TRUE(text.getSecure() == true);
    ASSERT_TRUE(text.getString() == "secret"); /* the raw string is untouched */

    auto const& codepoints = text.getCodepoints();
    ASSERT_TRUE(codepoints.size() == 6);
    ASSERT_TRUE(codepoints.at(0) == U's'); /* the codepoint buffer is untouched */

    auto const& glyphs = text.getGlyphs();
    ASSERT_TRUE(glyphs.size() == 6);
    for (auto const& glyph : glyphs) {
        ASSERT_TRUE(glyph.codepoint == 0x2022u); /* every glyph renders as a bullet */
    }

    text.setEditable(true);
    ASSERT_TRUE(text.getCodepoints().at(0) == U's'); /* the editor sees the real text */

    text.moveDocumentEnd();
    text.input("!");
    ASSERT_TRUE(text.getString() == "secret!");
    ASSERT_TRUE(text.getCodepoints().size() == 7);
    ASSERT_TRUE(text.getCodepoints().at(6) == U'!');
    ASSERT_TRUE(text.getGlyphs().size() == 7);
    ASSERT_TRUE(text.getGlyphs().at(6).codepoint == 0x2022u);

    text.setSecure(false);
    ASSERT_TRUE(text.getCodepoints().at(0) == U's');
    ASSERT_TRUE(text.getGlyphs().at(0).codepoint == U's'); /* unmasking restores the glyphs */
}

/* --- not editable: every editing method is a silent no-op --- */

TEST(Text, EditingNotEditableIsSilentNoOp) {
    Text text;
    text.setString("hello");

    ASSERT_TRUE(text.getEditable() == false);
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.isSelectedRange() == false);
    ASSERT_TRUE(text.getSelectionRects().empty());
    ASSERT_TRUE((text.getCaretRect() == Vec4{ 0.0f, 0.0f, 0.0f, 0.0f }));
    ASSERT_TRUE(text.getSnapshot().codepoints.empty());

    text.input("X");
    text.moveRight();
    text.selectAll();
    text.deleteBackward();
    text.deleteWordForward();
    text.paste("Y");
    text.mouseDown({ 0.0f, 0.0f });
    ASSERT_TRUE(text.getString() == "hello"); /* the string is untouched */

    auto string = std::string("stale");
    text.copy(string);
    ASSERT_TRUE(string.empty()); /* copy clears when there is nothing selected */

    string = "stale";
    text.cut(string);
    ASSERT_TRUE(string.empty());
    ASSERT_TRUE(text.getString() == "hello");
}

/* --- toggling editable off discards the caret and selection --- */

TEST(Text, EditingDisableThenEnableResetsState) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.selectAll();
    ASSERT_TRUE(text.isSelectedRange() == true);

    text.setEditable(false);
    ASSERT_TRUE(text.getEditable() == false);
    ASSERT_TRUE(text.isSelectedRange() == false);

    text.setEditable(true);
    ASSERT_TRUE(text.getCaretPosition() == 0);
    ASSERT_TRUE(text.getAnchorPosition() == 0);
    ASSERT_TRUE(text.getString() == "hello"); /* the string survives the round trip */
}

/* --- setEditable is idempotent: re-enabling does not clobber the caret --- */

TEST(Text, EditingSetEditableIsIdempotent) {
    Text text;
    text.setString("hello");

    text.setEditable(true);
    text.moveRight();
    text.moveRight();

    text.setEditable(true);
    ASSERT_TRUE(text.getCaretPosition() == 2);
}



/* --- an empty editable text reserves a line, like an empty <input> --- */

TEST(Text, EditingEmptyEditableReservesOneLine) {
    /* non-editable empty text collapses, the way an empty box does on the web */
    Text plain;
    plain.setStyle(TextStyle{ .fontSize = 15.0f });
    plain.setString("");
    ASSERT_TRUE(plain.getSize().height == 0.0f);

    /* editable empty text keeps one line, so the field does not collapse */
    Text field;
    field.setStyle(TextStyle{ .fontSize = 15.0f });
    field.setString("");
    field.setEditable(true);
    ASSERT_TRUE(field.getSize().height > 0.0f);

    /* and it is exactly the height one line of real text would take */
    Text filled;
    filled.setStyle(TextStyle{ .fontSize = 15.0f });
    filled.setString("x");
    ASSERT_TRUE(field.getSize().height == filled.getSize().height);
}

/* --- the caret in an empty field is a full line tall, not the font size --- */

TEST(Text, EditingEmptyCaretIsOneLineTall) {
    Text empty;
    empty.setStyle(TextStyle{ .fontSize = 15.0f });
    empty.setString("");
    empty.setEditable(true);

    Text filled;
    filled.setStyle(TextStyle{ .fontSize = 15.0f });
    filled.setString("x");
    filled.setEditable(true);

    ASSERT_TRUE(empty.getCaretRect().height == filled.getCaretRect().height);

    /* and it sits inside the reserved line rather than hanging out the bottom */
    ASSERT_TRUE(empty.getCaretRect().getMaxY() <= empty.getSize().height);
}

/* --- toggling editable re-runs layout, since it changes the reserved height --- */

TEST(Text, EditingToggleEditableUpdatesEmptyHeight) {
    Text text;
    text.setStyle(TextStyle{ .fontSize = 15.0f });
    text.setString("");

    auto const collapsed = text.getSize().height;

    text.setEditable(true);
    ASSERT_TRUE(text.getSize().height > collapsed);

    text.setEditable(false);
    ASSERT_TRUE(text.getSize().height == collapsed);
}


/* --- re-enabling editing rebuilds the caret geometry --- */

TEST(Text, EditingCaretSurvivesDisableEnableCycle) {
    Text text;
    text.setStyle(TextStyle{ .fontSize = 15.0f });
    text.setString("hi");

    text.setEditable(true);

    auto const first = text.getCaretRect();
    ASSERT_TRUE(first.width > 0.0f);
    ASSERT_TRUE(first.height > 0.0f);

    /* setEditable allocates a fresh state with zeroed geometry; the memo that
       gates the recompute has to be cleared alongside it, or the caret stays
       zero-sized and never paints. */
    text.setEditable(false);
    text.setEditable(true);

    auto const second = text.getCaretRect();
    ASSERT_TRUE(second.width == first.width);
    ASSERT_TRUE(second.height == first.height);
}

/* --- invalidation: read, mutate one input, read again ---
 *
 * Every test below first reads a derived output (populating the lazy caches),
 * then changes exactly one input, then reads again and asserts the output
 * reflects the change. This is the pattern that catches a missed cache
 * invalidation: tests that configure everything before the first read pass
 * even when invalidation is broken. Pinning these behaviors guards the
 * dependency-tracking machinery against refactoring regressions. */

/* setString after a read invalidates string, layout, and glyphs. */
TEST(Text, InvalidationSetStringAfterRead) {
    Text text;
    text.setString("aa");
    auto const sizeBefore = text.getSize();
    ASSERT_TRUE(text.getGlyphs().size() == 2);

    text.setString("aaaa");
    ASSERT_TRUE(text.getString() == "aaaa");
    ASSERT_TRUE(text.getGlyphs().size() == 4);
    ASSERT_TRUE(text.getSize().width > sizeBefore.width);
}

/* Style mutations after a read re-style already-shaped glyphs. */
TEST(Text, InvalidationStyleAfterRead) {
    Text text;
    text.setString("abcdef");
    ASSERT_TRUE((text.getGlyphs().at(0).color == red) == false);

    text.addStyle(0, 3, TextStyle{ .color = red });
    ASSERT_TRUE(text.getGlyphs().at(0).color == red);
    ASSERT_TRUE((text.getGlyphs().at(3).color == red) == false);

    text.clearStyle();
    ASSERT_TRUE((text.getGlyphs().at(0).color == red) == false);
}

/* Geometry setters after a read re-run layout: wrap width, fixed size, and
   padding. */
TEST(Text, InvalidationGeometryAfterRead) {
    Text text;
    text.setString("aaaa bbbb");
    auto const unwrapped = text.getSize();
    ASSERT_TRUE(text.getLines().size() == 1);

    text.setMaxWidth(unwrapped.width * 0.6f);
    ASSERT_TRUE(text.getLines().size() >= 2);

    text.setMaxWidth(std::nullopt);
    ASSERT_TRUE(text.getLines().size() == 1);
    ASSERT_TRUE(text.getSize() == unwrapped);

    text.setPaddingLeft(10.0f);
    text.setPaddingTop(7.0f);
    ASSERT_TRUE(text.getSize().width == (unwrapped.width + 10.0f));
    ASSERT_TRUE(text.getSize().height == (unwrapped.height + 7.0f));

    text.setPaddingLeft(std::nullopt);
    text.setPaddingTop(std::nullopt);
    text.setWidth(400.0f);
    text.setHeight(250.0f);
    ASSERT_TRUE(text.getSize() == Vec2(400.0f, 250.0f));
}

/* --- padding --- */

/* Content-sized text grows by the padding; glyphs shift to the padded origin. */
TEST(Text, PaddingGrowsContentSizeAndOffsetsGlyphs) {
    Text plain;
    plain.setString("hello");
    auto const contentSize = plain.getSize();

    Text padded;
    padded.setString("hello");
    padded.setPaddingTop(4.0f);
    padded.setPaddingLeft(8.0f);
    padded.setPaddingRight(16.0f);
    padded.setPaddingBottom(32.0f);

    ASSERT_TRUE(padded.getSize().width == (contentSize.width + 8.0f + 16.0f));
    ASSERT_TRUE(padded.getSize().height == (contentSize.height + 4.0f + 32.0f));
    ASSERT_TRUE(padded.getGlyphs().at(0).rect.x == (plain.getGlyphs().at(0).rect.x + 8.0f));
    ASSERT_TRUE(padded.getGlyphs().at(0).rect.y == (plain.getGlyphs().at(0).rect.y + 4.0f));
    ASSERT_TRUE(padded.getLines().at(0).rect.x == (plain.getLines().at(0).rect.x + 8.0f));
    ASSERT_TRUE(padded.getLines().at(0).rect.y == (plain.getLines().at(0).rect.y + 4.0f));
}

/* With a fixed width, horizontal padding narrows the wrap limit instead of
   growing the box. */
TEST(Text, PaddingNarrowsWrapLimitAtFixedWidth) {
    Text plain;
    plain.setString("aaaa bbbb");
    auto const unwrappedWidth = plain.getSize().width;

    /* Wide enough for one line without padding... */
    Text fixed;
    fixed.setString("aaaa bbbb");
    fixed.setWidth(unwrappedWidth + 10.0f);
    ASSERT_TRUE(fixed.getLines().size() == 1);

    /* ...but the padding eats the slack and forces a wrap. */
    fixed.setPaddingLeft(20.0f);
    fixed.setPaddingRight(20.0f);
    ASSERT_TRUE(fixed.getLines().size() >= 2);
    ASSERT_TRUE(fixed.getSize().width == std::ceil(unwrappedWidth + 10.0f));
}

/* End alignment and End justify position content against the padded edges,
   not the box edges. */
TEST(Text, PaddingRespectedByAlignmentAndJustify) {
    Text text;
    text.setString("hi");
    text.setWidth(300.0f);
    text.setHeight(300.0f);
    text.setPaddingRight(24.0f);
    text.setPaddingBottom(24.0f);
    text.setAlignment(TextAlignment::End);
    text.setJustify(TextJustify::End);

    auto const& line = text.getLines().at(0);
    ASSERT_TRUE(line.rect.getMaxX() <= (300.0f - 24.0f));
    ASSERT_TRUE(line.rect.getMaxX() > (300.0f - 24.0f - line.rect.width - 1.0f));
    ASSERT_TRUE(line.rect.getMaxY() <= (300.0f - 24.0f));
    ASSERT_TRUE(line.rect.getMaxY() > (300.0f - 24.0f - line.rect.height - 1.0f));
}

/* The caret respects padding: offset for empty text, and the caret in padded
   text tracks its (shifted) glyphs. */
TEST(Text, PaddingOffsetsCaret) {
    Text empty;
    empty.setString("");
    empty.setPaddingTop(6.0f);
    empty.setPaddingLeft(12.0f);
    empty.setEditable(true);
    ASSERT_TRUE(empty.getCaretRect().x == 12.0f);
    ASSERT_TRUE(empty.getCaretRect().y == 6.0f);

    Text text;
    text.setString("hello");
    text.setPaddingTop(6.0f);
    text.setPaddingLeft(12.0f);
    text.setEditable(true);
    ASSERT_TRUE(text.getCaretRect().x == text.getGlyphs().at(0).rect.x);
    ASSERT_TRUE(text.getCaretRect().x >= 12.0f);
    ASSERT_TRUE(text.getCaretRect().y >= 6.0f);
}

/* Alignment and justify after a read reposition glyphs within fixed bounds. */
TEST(Text, InvalidationAlignmentJustifyAfterRead) {
    Text text;
    text.setString("hi");
    text.setWidth(300.0f);
    text.setHeight(300.0f);

    auto const startX = text.getGlyphs().at(0).rect.x;
    auto const startY = text.getGlyphs().at(0).rect.y;

    text.setAlignment(TextAlignment::End);
    ASSERT_TRUE(text.getGlyphs().at(0).rect.x > startX);

    text.setJustify(TextJustify::End);
    ASSERT_TRUE(text.getGlyphs().at(0).rect.y > startY);
}

/* Scale after a read re-shapes glyphs at the new pixel size. */
TEST(Text, InvalidationScaleAfterRead) {
    Text text;
    text.setString("hello");
    auto const sizeBefore = text.getSize();

    text.setScale(2.0f);
    ASSERT_TRUE(text.getSize().width > (sizeBefore.width * 1.5f));
    ASSERT_TRUE(text.getSize().height > (sizeBefore.height * 1.5f));
}

/* Secure after a read swaps glyph shapes to bullets while the string stays real. */
TEST(Text, InvalidationSecureAfterRead) {
    Text text;
    text.setString("WWWW");
    auto const sizeBefore = text.getSize();

    text.setSecure(true);
    ASSERT_TRUE(text.getSize().width != sizeBefore.width);
    ASSERT_TRUE(text.getString() == "WWWW");

    text.setSecure(false);
    ASSERT_TRUE(text.getSize().width == sizeBefore.width);
}

/* Caret movement and selection changes after a read refresh the edit overlay. */
TEST(Text, InvalidationEditOverlayAfterRead) {
    Text text;
    text.setString("hello world");

    text.setEditable(true);
    auto const rectAtStart = text.getCaretRect();
    ASSERT_TRUE(text.getSelectionRects().empty());

    text.moveDocumentEnd();
    ASSERT_TRUE(text.getCaretRect().x > rectAtStart.x);

    text.selectAll();
    ASSERT_TRUE(text.getSelectionRects().empty() == false);
}

/* restoreSnapshot writes the edit state directly (bypassing the caret and
   anchor setters), so it must invalidate the edit overlay on its own. */
TEST(Text, InvalidationRestoreSnapshotAfterRead) {
    Text text;
    text.setString("hello world");

    text.setEditable(true);
    text.moveDocumentEnd();
    auto const rectAtEnd = text.getCaretRect();
    auto const snapshot = text.getSnapshot();

    text.moveDocumentStart();
    ASSERT_TRUE(text.getCaretRect().x < rectAtEnd.x);

    text.restoreSnapshot(snapshot);
    auto const rectRestored = text.getCaretRect();
    ASSERT_TRUE(rectRestored.x == rectAtEnd.x);
    ASSERT_TRUE(rectRestored.y == rectAtEnd.y);
}

/* The rendered image tracks layout changes made after it was first produced. */
TEST(Text, InvalidationImageAfterRead) {
    Text text;
    text.setString("hello");
    text.setWidth(200.0f);
    text.setHeight(100.0f);

    auto const imageBefore = text.getImage();
    ASSERT_TRUE(imageBefore != nullptr);
    ASSERT_TRUE(imageBefore->getSize() == Vec2(200.0f, 100.0f));

    text.setWidth(300.0f);
    auto const imageAfter = text.getImage();
    ASSERT_TRUE(imageAfter != nullptr);
    ASSERT_TRUE(imageAfter->getSize() == Vec2(300.0f, 100.0f));
}

/* Editing mutations after a read invalidate every derived output at once:
   string, layout, glyph styling, and the edit overlay. */
TEST(Text, InvalidationEditMutationAfterRead) {
    Text text;
    text.setString("abc");
    text.addStyle(0, 3, TextStyle{ .color = red });

    text.setEditable(true);
    auto const sizeBefore = text.getSize();
    auto const rectBefore = text.getCaretRect();
    ASSERT_TRUE(text.getGlyphs().size() == 3);

    text.moveDocumentEnd();
    text.input("def");
    ASSERT_TRUE(text.getString() == "abcdef");
    ASSERT_TRUE(text.getGlyphs().size() == 6);
    ASSERT_TRUE(text.getSize().width > sizeBefore.width);
    ASSERT_TRUE(text.getCaretRect().x > rectBefore.x);
    ASSERT_TRUE(text.getGlyphs().at(3).color == red); /* typed at the styled run's end */
}

/* =========================================================================
 * Adversarial editing tests, written against the behaviour of a macOS text
 * view and of a browser <input> / <textarea>; the divergences they found
 * were fixed alongside them.
 * ========================================================================= */

static void _Setup(Text& text, std::string const& string, std::optional<bool> multiLine = std::nullopt) {
    text.setString(string);
    text.setMultiLine(multiLine);
    text.setEditable(true);
}

static std::vector<std::uint32_t> _Codepoints(std::string const& string) {
    auto codepoints = std::vector<std::uint32_t>();
    StringToCodepoints(string, codepoints);
    return codepoints;
}

/* ------------------------------ word semantics ---------------------------- */

/* Option+Left/Right on macOS: leftwards, skip whitespace then a word;
   rightwards, skip whitespace then a word. Punctuation forms its own run
   ("hello, world|" -> 7 -> 5 -> 0), which is also what Chrome does. */
TEST(Text, EditingWordMovesMatchMacOS) {
    Text text;
    _Setup(text, "foo  bar baz");
    text.moveDocumentEnd();

    text.moveWordLeft();
    EXPECT_EQ(text.getCaretPosition(), 9);
    text.moveWordLeft();
    EXPECT_EQ(text.getCaretPosition(), 5);
    text.moveWordLeft();
    EXPECT_EQ(text.getCaretPosition(), 0);
    text.moveWordLeft();
    EXPECT_EQ(text.getCaretPosition(), 0);

    text.moveWordRight();
    EXPECT_EQ(text.getCaretPosition(), 3);
    text.moveWordRight();
    EXPECT_EQ(text.getCaretPosition(), 8);
    text.moveWordRight();
    EXPECT_EQ(text.getCaretPosition(), 12);
    text.moveWordRight();
    EXPECT_EQ(text.getCaretPosition(), 12);
}

/* Option+Backspace after a trailing space removes the space and the word
   before it ("foo bar |" -> "foo "); Option+Delete before leading spaces
   removes them and the word after ("foo|  bar" -> "foo"). Both verified
   against Chrome on macOS. */
TEST(Text, EditingWordDeletesSwallowAdjacentWhitespace) {
    Text text;
    _Setup(text, "foo bar ");
    text.moveDocumentEnd();
    text.deleteWordBackward();
    EXPECT_EQ(text.getString(), "foo ");
    EXPECT_EQ(text.getCaretPosition(), 4);

    Text other;
    _Setup(other, "foo  bar");
    other.moveWordRight();
    EXPECT_EQ(other.getCaretPosition(), 3);
    other.deleteWordForward();
    EXPECT_EQ(other.getString(), "foo");
    EXPECT_EQ(other.getCaretPosition(), 3);
}

/* A word delete with a selection present deletes exactly the selection. */
TEST(Text, EditingWordDeleteWithSelectionDeletesOnlySelection) {
    Text text;
    _Setup(text, "one two three");
    text.moveWordRight();
    text.moveWordRight(true);
    ASSERT_TRUE(text.isSelectedRange());
    text.deleteWordBackward();
    EXPECT_EQ(text.getString(), "one three");
    EXPECT_EQ(text.getCaretPosition(), 3);
}

/* Non-ASCII letters are word characters, so Option+Left over "naïve café"
   stops at the word starts, not inside the words. */
TEST(Text, EditingWordMovesTreatNonAsciiLettersAsWordChars) {
    Text text;
    _Setup(text, "naïve café");
    text.moveDocumentEnd();
    text.moveWordLeft();
    EXPECT_EQ(text.getCaretPosition(), 6);
    text.moveWordLeft();
    EXPECT_EQ(text.getCaretPosition(), 0);
}

/* ------------------------------ graphemes ------------------------------- */

/* Keycap sequences (digit + VS16 + U+20E3) are one cluster. */
TEST(Text, EditingKeycapSequenceIsOneGrapheme) {
    Text text;
    _Setup(text, "\x31\xEF\xB8\x8F\xE2\x83\xA3" "x"); /* 1️⃣x */
    ASSERT_EQ(text.getCodepoints().size(), 4u);

    text.moveRight();
    EXPECT_EQ(text.getCaretPosition(), 3);
    text.moveRight();
    EXPECT_EQ(text.getCaretPosition(), 4);
    text.moveLeft();
    text.deleteBackward();
    EXPECT_EQ(text.getString(), "x");
}

/* Emoji tag sequences (subdivision flags such as England, U+1F3F4 followed
   by tag characters U+E0067..U+E007F) are one grapheme cluster (UAX #29
   GB9, Extend includes tag characters): one Right press crosses the whole
   flag and one Backspace removes it entirely. */
TEST(Text, EditingTagSequenceFlagIsOneGrapheme) {
    Text text;
    _Setup(text, "\xF0\x9F\x8F\xB4\xF3\xA0\x81\xA7\xF3\xA0\x81\xA2\xF3\xA0\x81\xA5\xF3\xA0\x81\xAE\xF3\xA0\x81\xA7\xF3\xA0\x81\xBF");
    ASSERT_EQ(text.getCodepoints().size(), 7u);

    text.moveRight();
    EXPECT_EQ(text.getCaretPosition(), 7);
    text.deleteBackward();
    EXPECT_EQ(text.getString(), "");
}

/* Decomposed Hangul (choseong + jungseong + jongseong) is one cluster
   (UAX #29 GB6-GB8): one Right press crosses the syllable. */
TEST(Text, EditingHangulJamoSequenceIsOneGrapheme) {
    Text text;
    _Setup(text, "\xE1\x84\x92\xE1\x85\xA1\xE1\x86\xAB"); /* ᄒ + ᅡ + ᆫ = 한 */
    ASSERT_EQ(text.getCodepoints().size(), 3u);

    text.moveRight();
    EXPECT_EQ(text.getCaretPosition(), 3);
}

/* CR LF is one grapheme cluster (UAX #29 GB3). With multi-line content set
   programmatically to hold "\r\n", Backspace after it removes both
   codepoints, so no invisible lone CR is left behind. */
TEST(Text, EditingCrLfIsOneGraphemeOnBackspace) {
    Text text;
    _Setup(text, "a\r\nb", true);
    text.moveDocumentEnd();
    text.moveLeft();
    EXPECT_EQ(text.getCaretPosition(), 3);
    text.deleteBackward();
    EXPECT_EQ(text.getString(), "ab");
}

/* As a browser <textarea> does, multi-line editing normalises pasted and
   typed line endings to LF, so the value never contains "\r". */
TEST(Text, EditingMultiLineInputNormalisesLineEndings) {
    Text text;
    _Setup(text, "", true);
    text.paste("a\r\nb\rc");
    EXPECT_EQ(text.getString(), "a\nb\nc");
    EXPECT_EQ(text.getLines().size(), 3u);
}

/* ------------------------------ vertical moves ---------------------------- */

/* The goal column survives crossing a short line: from column 5 on the first
   line, Down lands on the empty middle line and a second Down returns to
   column 5 on the third line; Up retraces the path. */
TEST(Text, EditingGoalColumnSurvivesEmptyLine) {
    Text text;
    _Setup(text, "abcdef\n\nabcdef", true);
    for (auto i = 0; i < 5; i++) text.moveRight();
    ASSERT_EQ(text.getCaretPosition(), 5);

    text.moveDown();
    EXPECT_EQ(text.getCaretPosition(), 7);
    text.moveDown();
    EXPECT_EQ(text.getCaretPosition(), 13);
    text.moveUp();
    EXPECT_EQ(text.getCaretPosition(), 7);
    text.moveUp();
    EXPECT_EQ(text.getCaretPosition(), 5);
}

/* Shift+Up on the first line extends the selection to the start of the text
   and Shift+Down on the last line extends it to the end, as browsers do. */
TEST(Text, EditingShiftVerticalAtTextEdgesSelectsToEdge) {
    Text text;
    _Setup(text, "abc\ndef", true);
    text.moveRight();
    text.moveUp(true);
    EXPECT_EQ(text.getCaretPosition(), 0);
    EXPECT_EQ(text.getAnchorPosition(), 1);

    text.moveDocumentEnd();
    text.moveLeft();
    text.moveDown(true);
    EXPECT_EQ(text.getCaretPosition(), 7);
    EXPECT_EQ(text.getAnchorPosition(), 6);
}

/* After a horizontal move the goal column is the new caret x, not the one
   remembered from an earlier vertical move. */
TEST(Text, EditingHorizontalMoveResetsGoalColumn) {
    Text text;
    _Setup(text, "abcdef\nab\nabcdef", true);
    for (auto i = 0; i < 5; i++) text.moveRight();
    text.moveDown();               /* clamps to end of "ab" (index 9) */
    EXPECT_EQ(text.getCaretPosition(), 9);
    text.moveLeft();               /* index 8, goal column now 1 */
    text.moveDown();
    EXPECT_EQ(text.getCaretPosition(), 11);
}

/* ------------------------------ mouse hit-testing ------------------------- */

/* Points far outside the layout clamp to the nearest line and edge: above
   and left to the very start, right of a line to its end (before the line
   break), below to the last line. */
TEST(Text, EditingMouseHitTestingClampsOutsidePoints) {
    Text text;
    _Setup(text, "hello\nworld", true);

    text.mouseDown({ -1000.0f, -1000.0f });
    EXPECT_EQ(text.getCaretPosition(), 0);
    text.mouseUp({ -1000.0f, -1000.0f });

    text.mouseDown({ 1.0e6f, -1000.0f });
    EXPECT_EQ(text.getCaretPosition(), 5);
    text.mouseUp({ 1.0e6f, -1000.0f });

    text.mouseDown({ 1.0e6f, 1.0e6f });
    EXPECT_EQ(text.getCaretPosition(), 11);
    text.mouseUp({ 1.0e6f, 1.0e6f });

    text.mouseDown({ -1000.0f, 1.0e6f });
    EXPECT_EQ(text.getCaretPosition(), 6);
    text.mouseUp({ -1000.0f, 1.0e6f });
}

/* A drag that leaves the layout keeps extending the selection to the
   nearest edge instead of freezing or jumping back. */
TEST(Text, EditingMouseDragBeyondLayoutExtendsToEdges) {
    Text text;
    _Setup(text, "hello\nworld", true);

    text.mouseDown(pointAtGlyph(text, 2));
    text.mouseMove({ 1.0e6f, 1.0e6f });
    EXPECT_EQ(text.getAnchorPosition(), 2);
    EXPECT_EQ(text.getCaretPosition(), 11);
    text.mouseMove({ -1.0e6f, -1.0e6f });
    EXPECT_EQ(text.getCaretPosition(), 0);
    text.mouseUp({ -1.0e6f, -1.0e6f });
    EXPECT_EQ(text.getAnchorPosition(), 2);
    EXPECT_EQ(text.getCaretPosition(), 0);
}

/* A second mouseDown without a mouseUp (a lost release) starts a fresh
   click rather than extending the previous drag. */
TEST(Text, EditingMouseDownWithoutUpStartsFreshClick) {
    Text text;
    _Setup(text, "hello world");

    text.mouseDown(pointAtGlyph(text, 1));
    text.mouseMove(pointAtGlyph(text, 4));
    ASSERT_TRUE(text.isSelectedRange());

    text.mouseDown(pointAtGlyph(text, 8));
    EXPECT_FALSE(text.isSelectedRange());
    EXPECT_EQ(text.getCaretPosition(), 8);
    EXPECT_EQ(text.getAnchorPosition(), 8);
    text.mouseUp(pointAtGlyph(text, 8));
}

/* Clicking into empty editable text places the caret at 0 and paints one
   full-height caret. */
TEST(Text, EditingMouseOnEmptyTextIsSafe) {
    Text text;
    _Setup(text, "");
    text.mouseDown({ 10.0f, 5.0f });
    text.mouseMove({ 50.0f, 50.0f });
    text.mouseUp({ 50.0f, 50.0f });
    EXPECT_EQ(text.getCaretPosition(), 0);
    EXPECT_FALSE(text.isSelectedRange());
    EXPECT_GT(text.getCaretRect().height, 0.0f);
}

/* ------------------------------ secure mode ------------------------------- */

/* A password field reveals nothing about the words in its content: word
   moves and word deletes act on the masked bullets, which form a single
   run, so Option+Left jumps to the start (verified in Chrome: caret 21 -> 0
   in one press) and Option+Backspace clears the whole field. */
TEST(Text, EditingSecureWordOpsTreatContentAsOneRun) {
    Text text;
    _Setup(text, "correct horse battery");
    text.setSecure(true);
    text.moveDocumentEnd();

    text.moveWordLeft();
    EXPECT_EQ(text.getCaretPosition(), 0);

    text.moveDocumentEnd();
    text.deleteWordBackward();
    EXPECT_EQ(text.getString(), "");
}

/* Secure mode still edits the real string and mouse hit-testing works on
   the bullet layout. */
TEST(Text, EditingSecureEditingRoundTrip) {
    Text text;
    _Setup(text, "secret");
    text.setSecure(true);

    text.mouseDown(pointAtGlyph(text, 3));
    text.mouseUp(pointAtGlyph(text, 3));
    text.input("X");
    EXPECT_EQ(text.getString(), "secXret");

    text.selectAll();
    text.input("y");
    EXPECT_EQ(text.getString(), "y");
    EXPECT_EQ(text.getGlyphs().front().codepoint, 0x2022u);
}

/* ------------------------------ undo / redo ------------------------------- */

/* Typing forms undo groups split at word starts: undo removes " world",
   then "hello"; redo re-applies them in order. */
TEST(Text, EditingUndoGroupsTypingByWords) {
    Text text;
    _Setup(text, "");
    for (auto ch : std::string("hello world")) text.input(std::string(1, ch));
    ASSERT_EQ(text.getString(), "hello world");
    ASSERT_TRUE(text.canUndo());
    ASSERT_FALSE(text.canRedo());

    text.undo();
    EXPECT_EQ(text.getString(), "hello");
    EXPECT_EQ(text.getCaretPosition(), 5);
    text.undo();
    EXPECT_EQ(text.getString(), "");
    EXPECT_FALSE(text.canUndo());

    text.redo();
    EXPECT_EQ(text.getString(), "hello");
    text.redo();
    EXPECT_EQ(text.getString(), "hello world");
    EXPECT_FALSE(text.canRedo());
}

/* Moving the caret ends a typing group; consecutive backspaces form one
   group; a new edit after undo discards the redo history. */
TEST(Text, EditingUndoGroupsBreakOnCaretMoveAndDeletesCoalesce) {
    Text text;
    _Setup(text, "");
    text.input("a");
    text.input("b");
    text.moveLeft();
    text.moveRight();
    text.input("c");
    text.undo();
    EXPECT_EQ(text.getString(), "ab");

    text.deleteBackward();
    text.deleteBackward();
    EXPECT_EQ(text.getString(), "");
    text.undo();
    EXPECT_EQ(text.getString(), "ab");
    EXPECT_EQ(text.getCaretPosition(), 2);

    text.redo();
    EXPECT_EQ(text.getString(), "");
    text.undo();
    text.input("z");
    EXPECT_FALSE(text.canRedo());
    EXPECT_EQ(text.getString(), "abz");
}

/* Paste, cut and typing over a selection each undo as one step, restoring
   the selection's text; a programmatic setString clears the history. */
TEST(Text, EditingUndoRestoresReplacedSelectionsAndSetStringClears) {
    Text text;
    _Setup(text, "one two three");
    text.moveWordRight();
    text.moveWordRight(true);
    text.paste("PASTED");
    EXPECT_EQ(text.getString(), "onePASTED three");
    text.undo();
    EXPECT_EQ(text.getString(), "one two three");
    EXPECT_TRUE(text.isSelectedRange());

    auto cut = std::string();
    text.cut(cut);
    EXPECT_EQ(cut, " two");
    text.undo();
    EXPECT_EQ(text.getString(), "one two three");

    text.selectAll();
    text.input("x");
    text.undo();
    EXPECT_EQ(text.getString(), "one two three");

    text.setString("fresh");
    EXPECT_FALSE(text.canUndo());
    EXPECT_FALSE(text.canRedo());
}

/* Cmd+Backspace deletes to the start of the visual line, Cmd+Delete (and
   Ctrl+K) to its end; at a line end the break itself goes. */
TEST(Text, EditingLineDeletes) {
    Text text;
    _Setup(text, "one two\nthree", true);
    text.moveDocumentEnd();
    text.deleteLineBackward();
    EXPECT_EQ(text.getString(), "one two\n");
    text.moveDocumentStart();
    text.moveWordRight();
    text.deleteLineForward();
    EXPECT_EQ(text.getString(), "one\n");
    text.deleteLineForward();
    EXPECT_EQ(text.getString(), "one");
    text.undo();
    EXPECT_EQ(text.getString(), "one\n");
}

/* ------------------------------ multi-click ------------------------------- */

/* A double click selects the word under the point (or the whitespace run),
   a triple click the paragraph. */
TEST(Text, EditingDoubleClickSelectsWordTripleClickSelectsParagraph) {
    Text text;
    _Setup(text, "one two\nthree four", true);

    text.mouseDown(pointAtGlyph(text, 5), false, 2);
    EXPECT_EQ(text.getAnchorPosition(), 4);
    EXPECT_EQ(text.getCaretPosition(), 7);
    text.mouseUp(pointAtGlyph(text, 5));

    text.mouseDown(pointAtGlyph(text, 3), false, 2);
    EXPECT_EQ(text.getAnchorPosition(), 3);
    EXPECT_EQ(text.getCaretPosition(), 4);
    text.mouseUp(pointAtGlyph(text, 3));

    text.mouseDown(pointAtGlyph(text, 10), false, 3);
    EXPECT_EQ(text.getAnchorPosition(), 8);
    EXPECT_EQ(text.getCaretPosition(), 18);
    text.mouseUp(pointAtGlyph(text, 10));

    text.input("X");
    EXPECT_EQ(text.getString(), "one two\nX");
}

/* ------------------------------ single-line ------------------------------- */

/* Single-line editing never wraps, however narrow the width limit. */
TEST(Text, EditingSingleLineNeverWraps) {
    Text text;
    _Setup(text, "a value that is much wider than the field it lives in", false);
    text.setMaxWidth(40.0f);
    EXPECT_EQ(text.getLines().size(), 1u);
    text.setWidth(40.0f);
    EXPECT_EQ(text.getLines().size(), 1u);
    EXPECT_GT(text.getGlyphs().back().rect.getMaxX(), 40.0f);
}


/* Single-line content that (against the caller's contract) contains a line
   break must still be navigable without a crash, and vertical moves go to
   the visual line edges. */
TEST(Text, EditingSingleLineWithStrayLineBreakIsSafe) {
    Text text;
    _Setup(text, "ab\ncd", false);
    text.moveDocumentEnd();
    text.moveUp();
    EXPECT_EQ(text.getCaretPosition(), 3);
    text.moveDown();
    EXPECT_EQ(text.getCaretPosition(), 5);
    text.moveLeft();
    text.moveLeft();
    text.deleteBackward();
    EXPECT_EQ(text.getString(), "abcd");
}

/* ------------------------------ selection rects --------------------------- */

/* Selecting a run that ends exactly on a line break paints the tail of the
   first line and nothing for the (empty) start of the next. */
TEST(Text, EditingSelectionEndingAtLineBreakPaintsOneRect) {
    Text text;
    _Setup(text, "ab\ncd", true);
    text.moveRight();
    text.moveRight(true);
    text.moveRight(true); /* selection "b\n" */
    ASSERT_EQ(text.getCaretPosition(), 3);
    auto const& rects = text.getSelectionRects();
    ASSERT_EQ(rects.size(), 1u);
    EXPECT_GT(rects[0].width, 0.0f);
}

/* Selecting across three lines paints one rect per line, the middle one
   spanning the whole line. */
TEST(Text, EditingSelectionAcrossThreeLinesPaintsThreeRects) {
    Text text;
    _Setup(text, "abc\ndef\nghi", true);
    text.moveRight();
    for (auto i = 0; i < 8; i++) text.moveRight(true); /* 1..9 */
    auto const& rects = text.getSelectionRects();
    ASSERT_EQ(rects.size(), 3u);
    auto const& lines = text.getLines();
    EXPECT_FLOAT_EQ(rects[1].x, lines[1].rect.x);
    EXPECT_FLOAT_EQ(rects[1].width, lines[1].rect.width);
}

/* ------------------------------ random-operation soak --------------------- */

/* Thousands of random edits, moves, mouse gestures, layout changes and
   snapshot restores over a Unicode-heavy alphabet; after each operation the
   edit state must satisfy its invariants. This is the "no bugs" net for the
   engine: it must never crash, never leave caret or anchor outside the
   buffer, always re-encode its own string losslessly, and always produce a
   finite caret rect. */
TEST(Text, EditingRandomOperationSoakKeepsInvariants) {
    static auto const _pieces = std::vector<std::string>{
        "a", "b", " ", "  ", "\n", "\t", "x_y", "-", ",", "é", "e\xCC\x81",
        "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7", /* family ZWJ */
        "\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", /* US flag */
        "\x31\xEF\xB8\x8F\xE2\x83\xA3", /* keycap */
        "\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD", /* thumbs up + skin tone */
        "\xE6\xB1\x89\xE5\xAD\x97", "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D",
        "word another", "\r\n", "\r", "lorem ipsum dolor sit amet"
    };

    auto rng = std::mt19937(20260910u);
    auto pick = [&](std::int64_t lo, std::int64_t hi) {
        return std::uniform_int_distribution<std::int64_t>(lo, hi)(rng);
    };
    auto piece = [&]() { return _pieces[(std::size_t)pick(0, (std::int64_t)_pieces.size() - 1)]; };
    auto point = [&](Text const& text) {
        auto const& size = text.getSize();
        return Vec2{
            (float)pick(-40, (std::int64_t)size.width + 40),
            (float)pick(-40, (std::int64_t)size.height + 40)
        };
    };

    Text text;
    _Setup(text, "hello world\nsecond line", true);
    text.setMaxWidth(120.0f);

    auto snapshot = text.getSnapshot();
    auto clipboard = std::string();
    auto mouseIsDown = false;

    for (auto step = 0; step < 6000; step++) {
        auto const op = pick(0, 31);
        auto const sel = (pick(0, 1) == 1);

        switch (op) {
            case 0: case 1: case 2: text.input(piece()); break;
            case 3: text.moveLeft(sel); break;
            case 4: text.moveRight(sel); break;
            case 5: text.moveUp(sel); break;
            case 6: text.moveDown(sel); break;
            case 7: text.moveWordLeft(sel); break;
            case 8: text.moveWordRight(sel); break;
            case 9: text.moveLineStart(sel); break;
            case 10: text.moveLineEnd(sel); break;
            case 11: text.moveDocumentStart(sel); break;
            case 12: text.moveDocumentEnd(sel); break;
            case 13: text.deleteForward(); break;
            case 14: text.deleteBackward(); break;
            case 15: text.deleteWordForward(); break;
            case 16: text.deleteWordBackward(); break;
            case 17: text.selectAll(); break;
            case 18: text.mouseDown(point(text), sel); mouseIsDown = true; break;
            case 19: text.mouseMove(point(text)); break;
            case 20: text.mouseUp(point(text)); mouseIsDown = false; break;
            case 21: text.paste(clipboard); break;
            case 22: text.copy(clipboard); break;
            case 23: text.cut(clipboard); break;
            case 24: snapshot = text.getSnapshot(); break;
            case 25: text.restoreSnapshot(snapshot); break;
            case 26: text.setMaxWidth((pick(0, 3) == 0) ? std::nullopt : std::optional<float>((float)pick(20, 300))); break;
            case 27: text.setMultiLine(pick(0, 2) == 0 ? std::optional<bool>() : std::optional<bool>(pick(0, 1) == 1)); break;
            case 28: text.setSecure(pick(0, 1) == 1); break;
            case 29: text.setString(piece()); break;
            case 30: text.setScale((pick(0, 1) == 0) ? std::nullopt : std::optional<float>(2.0f)); break;
            case 31: text.setStyle(TextStyle{ .fontSize = (float)pick(8, 24) }); break;
        }

        (void)mouseIsDown;

        /* keep the buffer bounded so layout stays cheap */
        if (text.getCodepoints().size() > 400) {
            text.selectAll();
            text.input("reset");
        }

        auto const size = (std::int64_t)text.getCodepoints().size();
        ASSERT_GE(text.getCaretPosition(), 0) << "step " << step;
        ASSERT_LE(text.getCaretPosition(), size) << "step " << step;
        ASSERT_GE(text.getAnchorPosition(), 0) << "step " << step;
        ASSERT_LE(text.getAnchorPosition(), size) << "step " << step;
        ASSERT_EQ(_Codepoints(text.getString()), text.getCodepoints()) << "step " << step;

        auto const& caret = text.getCaretRect();
        ASSERT_TRUE(std::isfinite(caret.x) && std::isfinite(caret.y) && std::isfinite(caret.width) && std::isfinite(caret.height)) << "step " << step;
        ASSERT_GE(caret.x, -1.0f) << "step " << step;
        ASSERT_GE(caret.y, -1.0f) << "step " << step;
        ASSERT_LE(caret.x, text.getSize().width + 1.0f) << "step " << step;
        ASSERT_LE(caret.getMaxY(), text.getSize().height + 1.0f) << "step " << step;
        ASSERT_GT(caret.height, 0.0f) << "step " << step;

        if (text.isSelectedRange() && (size > 0)) {
            ASSERT_FALSE(text.getSelectionRects().empty()) << "step " << step;
            for (auto const& rect : text.getSelectionRects()) {
                ASSERT_TRUE(std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.width) && std::isfinite(rect.height)) << "step " << step;
                ASSERT_GE(rect.width, 0.0f) << "step " << step;
                ASSERT_GE(rect.height, 0.0f) << "step " << step;
            }
        } else {
            ASSERT_TRUE(text.getSelectionRects().empty()) << "step " << step;
        }

        for (auto const& glyph : text.getGlyphs()) {
            ASSERT_TRUE(std::isfinite(glyph.rect.x) && std::isfinite(glyph.rect.y)) << "step " << step;
        }
    }
}

/* Every edit is a full re-shape and re-raster, so long content gets slower
   per keystroke. This measures a keystroke on ~20k characters and fails only
   if it is clearly interactive-unfriendly; the timing is printed so it can
   be tracked. */
TEST(Text, EditingKeystrokeLatencyOnLargeText) {
    auto content = std::string();
    for (auto i = 0; i < 400; i++) {
        content += "The quick brown fox jumps over the lazy dog and keeps running.\n";
    }

    Text text;
    _Setup(text, content, true);
    text.setMaxWidth(600.0f);
    text.moveDocumentEnd();
    (void)text.getImage();

    auto const start = std::chrono::steady_clock::now();
    auto const keystrokes = 20;
    for (auto i = 0; i < keystrokes; i++) {
        text.input("x");
        (void)text.getImage();
        (void)text.getCaretRect();
    }
    auto const elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    auto const perKeystroke = elapsed / keystrokes;

    std::printf("[  METRIC  ] %zu codepoints: %.2f ms per keystroke (shape + raster)\n", text.getCodepoints().size(), perKeystroke);
    EXPECT_LT(perKeystroke, 100.0) << "a keystroke on ~25k characters takes " << perKeystroke << " ms";
}

/* Left/Right are O(n) per press because the previous grapheme boundary is
   found by walking from the start; on a long line this is still cheap. */
TEST(Text, EditingCaretMovesOnLongLineStayCheap) {
    auto content = std::string(20000, 'a');
    Text text;
    _Setup(text, content, false);
    text.moveDocumentEnd();

    auto const start = std::chrono::steady_clock::now();
    for (auto i = 0; i < 200; i++) {
        text.moveLeft();
    }
    auto const elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("[  METRIC  ] 200 Left presses at the end of 20k chars: %.2f ms\n", elapsed);
    EXPECT_LT(elapsed, 500.0);
    EXPECT_EQ(text.getCaretPosition(), 19800);
}
