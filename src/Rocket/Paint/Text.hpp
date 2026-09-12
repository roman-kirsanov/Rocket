/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Font.hpp>

namespace Rocket {

/** Default line height as a unitless multiplier of the font size (the web's `line-height: normal` ≈ 1.2). */
auto constexpr TEXT_DEFAULT_LINE_HEIGHT = 1.2f;

/** Default font size in pixels, applied when no style specifies one. */
auto constexpr TEXT_DEFAULT_FONT_SIZE = 12.0f;

/** Default font weight, applied when no style specifies one. */
auto constexpr TEXT_DEFAULT_FONT_WEIGHT = FontWeight::Normal;

/** Default font style, applied when no style specifies one. */
auto constexpr TEXT_DEFAULT_FONT_STYLE = FontStyle::Normal;

/** Default text color, applied when no style specifies one. */
auto constexpr TEXT_DEFAULT_COLOR = Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };

/** Default font family: the empty string matches no registered font, so Text falls back to Font::GetDefault() (the embedded default face). */
auto const TEXT_DEFAULT_FONT_FAMILY = std::string();

/**
 * Horizontal alignment of text within the layout width. Start is the leading
 * (left) edge, End is the trailing (right) edge. Justify is not yet
 * implemented and currently behaves like Start.
 */
enum class TextAlignment {
    Start,
    Center,
    Justify,
    End
};

/**
 * Vertical placement of text within the layout height when height exceeds
 * the text height. Start is the top edge, End is the bottom edge.
 */
enum class TextJustify {
    Start,
    Center,
    End
};

/** Layout information for a single wrapped line of text. */
struct TextLine {
    /** Bounding rectangle of the line (x, y, width, height) in layout space. */
    Vec4 rect;
    /** Zero-based row index of this line. */
    std::int64_t rowIndex;
    /** Index of the first glyph in this line within the glyphs array. */
    std::int64_t startIndex;
    /** One-past-the-end index of glyphs in this line. */
    std::int64_t endIndex;
    /** Maximum ascent across all glyphs on this line, in pixels; an empty line inherits the previous line's metrics. */
    float ascent;
    /** Maximum descent across all glyphs on this line, in pixels. */
    float descent;
    /** Maximum line gap across all glyphs on this line, in pixels. */
    float leading;
};

/** Layout and rendering information for a single shaped glyph. */
struct TextGlyph {
    /** Advance-width glyph cell in layout space (x, y, advance, line-height). */
    Vec4 rect;
    /** Actual bitmap position in layout space, offset by bearing. */
    Vec4 bitmapRect;
    /** Index of this glyph in the codepoint sequence. */
    std::int64_t index;
    /** Row (wrapped line) this glyph belongs to. */
    std::int64_t rowIndex;
    /** Column position within the row (reserved, currently 0). */
    std::int64_t colIndex;
    /** Rendered Unicode codepoint (U+2022 for every glyph while secure display is enabled). */
    std::uint32_t codepoint;
    /** Font used to render this glyph. */
    Font const* font;
    /** Foreground RGBA color. */
    Vec4 color;
    /** Background highlight RGBA color (alpha 0 means no highlight). */
    Vec4 marker;
    /** Font size in pixels used for this glyph. */
    float size;
    /** Underline flag from the style; not drawn by Text's renderer. */
    bool underline;
    /** Strikeout flag from the style; not drawn by Text's renderer. */
    bool strikeout;
    /** Glyph bearing (bitmap offset from glyph origin). */
    Vec2 bearing;
    /** Ascent of this glyph's font at its size, in pixels. */
    float ascent;
    /** Descent of this glyph's font at its size, in pixels. */
    float descent;
    /** Line gap of this glyph's font at its size, in pixels. */
    float leading;
    /** True when the codepoint is a whitespace character. */
    bool whitespace;
    /** Atlas image containing the glyph bitmap, or nullptr for whitespace. */
    Image const* image;
    /** Sub-region within the atlas image (x, y, width, height). */
    Vec4 imageSlice;
};

/** Per-run typographic style applied to a range of codepoints. */
struct TextStyle {
    /** Font family name looked up in the font registry. */
    std::optional<std::string> fontFamily;
    /** Font weight. */
    std::optional<FontWeight> fontWeight;
    /** Font style (normal or italic). */
    std::optional<FontStyle> fontStyle;
    /** Font size in pixels before scale is applied. */
    std::optional<float> fontSize;
    /** Foreground RGBA color. */
    std::optional<Vec4> color;
    /** Background highlight RGBA color. */
    std::optional<Vec4> marker;
    /** Underline flag; propagated to TextGlyph::underline for callers, not drawn by Text's renderer. */
    std::optional<bool> underline;
    /** Strikeout flag; propagated to TextGlyph::strikeout for callers, not drawn by Text's renderer. */
    std::optional<bool> strikeout;
    bool operator==(TextStyle const&) const;
    bool operator!=(TextStyle const&) const;
};

/** A styled codepoint range [startIndex, endIndex) used with Text::setStyles. */
struct TextStyleRange {
    /** Codepoint index where the range starts (inclusive). */
    std::int64_t startIndex;
    /** Codepoint index where the range ends (exclusive). */
    std::int64_t endIndex;
    /** The style applied to the range. */
    TextStyle style;
};

/** Full edit state captured by Text::getSnapshot(). */
struct TextEditSnapshot {
    /** Codepoint buffer mirroring the text at capture time. */
    std::vector<std::uint32_t> codepoints;
    /** Style ranges applied to the text at capture time. */
    std::vector<TextStyleRange> styles;
    /** Caret (insertion point) codepoint index. */
    std::int64_t caret;
    /** Anchor (fixed end of the selection) codepoint index. */
    std::int64_t anchor;
    /** Preferred caret x position for vertical movement, if any. */
    std::optional<float> caretXGoal;
    /** Whether the caret clings to the end of the previous visual line at a soft wrap. */
    bool caretAffinity;
};

/**
 * Lazy text shaping, layout, and rendering engine.
 *
 * Non-copyable and non-movable. The text is held as a sequence of Unicode
 * codepoints, split into styled segments, shaped into glyphs, word-wrapped
 * within the configured width, and rendered into a GPU-backed Image on demand.
 * The UTF-8 string, index, layout, image, and caret geometry are all derived
 * from that sequence and recomputed lazily behind per-stage dirty flags
 * (string, index, layout, image, edit), so the work only happens after an
 * input the stage depends on actually changes; the size, string, scale, and
 * secure setters compare before assigning and only mark the downstream stages
 * dirty on a real change, while the style setters always invalidate.
 *
 * Interactive editing is opt-in via setEditable(). While enabled the object
 * additionally owns a caret and an anchor position (which together define the
 * selection range) and exposes the caret and selection rectangles
 * (getCaretRect(), getSelectionRects()) for the caller to draw; editing
 * mutates the codepoints in place. The editing methods are no-ops while
 * editing is disabled.
 *
 * Style ranges follow edits. Inserting and deleting remaps every bounded
 * range so styles stay attached to the characters they were applied to:
 * insertions inherit the style of the character before the caret (a range
 * ending exactly at the caret grows to cover the new text, a range starting
 * exactly at the caret is pushed past it, and at index 0 — where there is no
 * character before — ranges starting at 0 absorb the new text instead), and
 * deletions shrink ranges overlapping the removed span, dropping ranges that
 * become empty. Whole-string ranges (setStyle, or addStyle without bounds)
 * always keep covering the entire text.
 */
class Text {
public:
    ~Text();
    Text();

    Text(Text &&) = delete;
    Text(Text const&) = delete;
    Text& operator=(Text &&) = delete;
    Text& operator=(Text const&) = delete;

    /** Returns the computed layout size (width × height) in pixels, triggering layout if needed. */
    Vec2 const& getSize() const;

    /** Returns the rendered GPU image, triggering layout and image update if needed. */
    Image const* getImage() const;

    /** Returns the laid-out line descriptors, triggering layout if needed. */
    std::vector<TextLine> const& getLines() const;

    /** Returns the laid-out glyph descriptors, triggering layout if needed. */
    std::vector<TextGlyph> const& getGlyphs() const;

    /** Returns the optional maximum layout width used for word-wrapping. */
    std::optional<float> const& getMaxWidth() const;

    /** Returns the optional maximum layout height (currently unused by layout). */
    std::optional<float> const& getMaxHeight() const;

    /** Returns the fixed layout width (overrides content width when set). */
    std::optional<float> const& getWidth() const;

    /** Returns the fixed layout height (overrides content height when set). */
    std::optional<float> const& getHeight() const;

    /** Returns the top padding in pixels. */
    std::optional<float> const& getPaddingTop() const;

    /** Returns the left padding in pixels. */
    std::optional<float> const& getPaddingLeft() const;

    /** Returns the right padding in pixels. */
    std::optional<float> const& getPaddingRight() const;

    /** Returns the bottom padding in pixels. */
    std::optional<float> const& getPaddingBottom() const;

    /** Returns the horizontal text alignment. */
    TextAlignment const& getAlignment() const;

    /** Returns the vertical text justification within the layout height. */
    TextJustify const& getJustify() const;

    /** Returns the text as UTF-8, re-encoding from the codepoints if editing has changed them. */
	std::string const& getString() const;

    /** Returns the optional display scale factor applied to all font sizes. */
    std::optional<float> const& getScale() const;

    /** Whether secure display is enabled (every codepoint renders as a bullet). */
    std::optional<bool> getSecure() const;

    /** Returns the multi-line editing mode, or std::nullopt when unset (behaves as true). */
    std::optional<bool> getMultiLine() const;

    /** Returns the Unicode codepoint sequence backing the text. */
    std::vector<std::uint32_t> const& getCodepoints() const;

    /**
     * Returns the glyph nearest to the given layout-space position.
     *
     * First finds the closest line, then the closest glyph within that line.
     * Returns std::nullopt when there are no glyphs.
     *
     * @param position The layout-space position to search near.
     */
    std::optional<TextGlyph> getGlyphAtPosition(Vec2 const& position) const;

    /** Returns the rectangles covering the current selection (one rect per wrapped line). */
    std::vector<Vec4> const& getSelectionRects() const;

    /** Returns the caret rectangle in layout space. */
    Vec4 const& getCaretRect() const;

    /** Returns the current caret (insertion point) codepoint index. */
    std::int64_t getCaretPosition() const;

    /** Returns the anchor (fixed end of the selection) codepoint index. */
    std::int64_t getAnchorPosition() const;

    /** Returns a snapshot of the full edit state (text, styles, caret, anchor, selection). */
    TextEditSnapshot getSnapshot() const;

    /** Whether interactive editing is enabled (caret, selection and mutation). */
    bool getEditable() const;

    /** Returns true when caret and anchor differ (i.e. a non-empty range is selected). */
    bool isSelectedRange() const;

    /**
     * Sets the maximum width for word-wrapping.
     *
     * @param maxWidth Maximum layout width in pixels, or std::nullopt to remove the limit.
     */
    void setMaxWidth(std::optional<float> const& maxWidth);

    /**
     * Stores a maximum layout height. Currently unused by layout — the
     * computed height is clamped only by setHeight().
     *
     * @param maxHeight Maximum layout height in pixels, or std::nullopt to remove the limit.
     */
    void setMaxHeight(std::optional<float> const& maxHeight);

    /**
     * Sets a fixed layout width, overriding the content width.
     *
     * @param width Fixed layout width in pixels, or std::nullopt to use the content width.
     */
    void setWidth(std::optional<float> const& width);

    /**
     * Sets a fixed layout height, overriding the content height.
     *
     * @param height Fixed layout height in pixels, or std::nullopt to use the content height.
     */
    void setHeight(std::optional<float> const& height);

    /**
     * Sets the top padding in pixels.
     *
     * @param paddingTop Top padding in pixels, or std::nullopt for no padding.
     */
    void setPaddingTop(std::optional<float> const& paddingTop);

    /**
     * Sets the left padding in pixels.
     *
     * @param paddingLeft Left padding in pixels, or std::nullopt for no padding.
     */
    void setPaddingLeft(std::optional<float> const& paddingLeft);

    /**
     * Sets the right padding in pixels.
     *
     * @param paddingRight Right padding in pixels, or std::nullopt for no padding.
     */
    void setPaddingRight(std::optional<float> const& paddingRight);

    /**
     * Sets the bottom padding in pixels.
     *
     * @param paddingBottom Bottom padding in pixels, or std::nullopt for no padding.
     */
    void setPaddingBottom(std::optional<float> const& paddingBottom);

    /**
     * Sets the horizontal text alignment.
     *
     * @param alignment The horizontal alignment to apply.
     */
    void setAlignment(TextAlignment const& alignment);

    /**
     * Sets the vertical justification of text within the layout height.
     *
     * @param justify The vertical justification to apply.
     */
    void setJustify(TextJustify const& justify);

    /**
     * Replaces the displayed string and marks the derived stages dirty to trigger re-shaping and re-layout.
     *
     * When the string actually changes and editing is enabled, the caret and
     * anchor are clamped to the new length (so a field that rewrites its
     * value while the user types keeps the caret in place, and a shorter
     * value leaves it at the end) and the undo history is cleared. Setting
     * the same string is a no-op, so a layout pass re-supplying the current
     * string leaves the caret where it is.
     *
     * @param string The new UTF-8 string to display.
     */
	void setString(std::string const& string);

    /**
     * Sets a display scale factor multiplied into all font sizes.
     *
     * @param scale Display scale factor, or std::nullopt for no scaling.
     */
    void setScale(std::optional<float> const& scale);

    /**
     * Enables or disables secure display.
     *
     * When enabled every glyph is built from a bullet (U+2022) instead of its
     * own codepoint, so the text renders fully masked while getString(),
     * getCodepoints() and editing all keep operating on the real string.
     *
     * @param secure Whether to mask the rendered text.
     */
    void setSecure(std::optional<bool> secure);

    /**
     * Sets the multi-line editing mode; unset behaves as true. When false,
     * strings passed to input() and paste() have their line breaks (\n,
     * \r\n, \r) replaced with single spaces, the layout never wraps (the
     * width limit is ignored, so the caller scrolls the overflow), and
     * moveUp()/moveDown() move to the line start/end instead of switching
     * lines. When true, typed and pasted \r\n and \r are normalised to \n.
     */
    void setMultiLine(std::optional<bool> multiLine);

    /**
     * Enables or disables interactive editing.
     *
     * Enabling allocates the edit state with the caret and anchor at 0;
     * disabling frees it. The codepoint buffer is maintained independently of
     * this flag. All editing methods are no-ops while editing is disabled.
     *
     * @param editable Whether the text can be edited.
     */
    void setEditable(bool editable);

    /**
     * Replaces all style ranges with a single style covering the entire string.
     *
     * Marks the index dirty, so the text is re-shaped and re-laid-out on next access.
     *
     * @param style The style applied to the entire string.
     */
    void setStyle(TextStyle const& style);

    /**
     * Appends a style range covering the entire string (no codepoint bounds).
     *
     * Later-appended styles take precedence over earlier ones for overlapping fields.
     *
     * @param style The style range to append, covering the whole string.
     */
    void addStyle(TextStyle const& style);

    /**
     * Appends a style range covering codepoints [startIndex, endIndex).
     *
     * Later-appended styles take precedence over earlier ones for overlapping fields.
     *
     * @param startIndex Codepoint index where the range starts (inclusive).
     * @param endIndex   Codepoint index where the range ends (exclusive).
     * @param style      The style applied to the range.
     */
    void addStyle(std::int64_t startIndex, std::int64_t endIndex, TextStyle const& style);

    /**
     * Replaces all style ranges with the given ones.
     *
     * Unlike clearStyle() + addStyle(), this only invalidates (and later
     * re-shapes and re-rasterizes) when the ranges actually differ, so it
     * is safe to call with identical styles on every layout pass.
     */
    void setStyles(std::vector<TextStyleRange> const&);

    /** Removes all style ranges, reverting to default appearance. */
    void clearStyle();

    /**
     * Inserts a UTF-8 string at the caret position, replacing the selection if any.
     *
     * @param string The UTF-8 string to insert at the caret.
     */
    void input(std::string const& string);

    /**
     * Moves the caret up one line; in single-line mode (see setMultiLine) moves to the start of the line instead.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveUp(bool selection = false);

    /**
     * Moves the caret one grapheme cluster to the left.
     *
     * When a selection exists and selection is false, collapses the caret to
     * the start of the selection instead of stepping.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveLeft(bool selection = false);

    /**
     * Moves the caret one grapheme cluster to the right.
     *
     * When a selection exists and selection is false, collapses the caret to
     * the end of the selection instead of stepping.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveRight(bool selection = false);

    /**
     * Moves the caret down one line; in single-line mode (see setMultiLine) moves to the end of the line instead.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveDown(bool selection = false);

    /**
     * Moves the caret one word to the left.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveWordLeft(bool selection = false);

    /**
     * Moves the caret one word to the right.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveWordRight(bool selection = false);

    /**
     * Moves the caret to the start of the current line.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveLineStart(bool selection = false);

    /**
     * Moves the caret to the end of the current visual line.
     *
     * At a soft wrap the caret stays on the current visual line (upstream
     * affinity) instead of jumping to the start of the following line.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveLineEnd(bool selection = false);

    /**
     * Moves the caret to the start of the document.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveDocumentStart(bool selection = false);

    /**
     * Moves the caret to the end of the document.
     *
     * @param selection Whether to extend the selection instead of moving it; default false.
     */
    void moveDocumentEnd(bool selection = false);

    /** Deletes the grapheme cluster after the caret, or the selection if one exists. */
    void deleteForward();

    /** Deletes the grapheme cluster before the caret, or the selection if one exists. */
    void deleteBackward();

    /** Deletes the word after the caret, or the selection if one exists. */
    void deleteWordForward();

    /** Deletes the word before the caret, or the selection if one exists. */
    void deleteWordBackward();

    /** Deletes from the caret to the start of its visual line, or the selection if one exists. */
    void deleteLineBackward();

    /** Deletes from the caret to the end of its visual line, or the selection if one exists. */
    void deleteLineForward();

    /**
     * Selects the word at the given codepoint index (the run of whitespace
     * when the index is on whitespace); the anchor is the run start and the
     * caret its end.
     *
     * @param index The codepoint index to select around.
     */
    void selectWordAt(std::int64_t index);

    /**
     * Selects the paragraph at the given codepoint index: the text between
     * the surrounding line breaks, excluding them.
     *
     * @param index The codepoint index to select around.
     */
    void selectParagraphAt(std::int64_t index);

    /**
     * Reverts the most recent edit group. Consecutive typing without moving
     * the caret forms one group, split where a whitespace character starts a
     * new word; consecutive deletes in one direction form one group too.
     */
    void undo();

    /** Re-applies the edit group most recently reverted by undo(). */
    void redo();

    /** Whether undo() has an edit group to revert. */
    bool canUndo() const;

    /** Whether redo() has an edit group to re-apply. */
    bool canRedo() const;

    /** Selects all codepoints. */
    void selectAll();

    /**
     * Positions the caret at the glyph nearest to the given layout-space point and begins a mouse drag.
     *
     * @param position   Layout-space point where the mouse button was pressed.
     * @param extend     When true, moves only the caret and keeps the anchor (shift+click selection); default false.
     * @param clickCount The consecutive click count: 2 selects the word under the point, 3 or more the paragraph; default 1.
     */
    void mouseDown(Vec2 const& position, bool extend = false, int clickCount = 1);

    /**
     * Extends the selection to the glyph nearest to the given point during a drag.
     *
     * @param position Layout-space point of the current mouse position.
     */
    void mouseMove(Vec2 const& position);

    /**
     * Ends the mouse drag.
     *
     * @param position Layout-space point where the mouse button was released.
     */
    void mouseUp(Vec2 const& position);

    /**
     * Inserts a UTF-8 string at the caret, replacing the selection if any (equivalent to input).
     *
     * Pasting an empty string is a no-op: the selection and caret are left untouched.
     *
     * @param string The UTF-8 string to insert at the caret.
     */
    void paste(std::string const& string);

    /**
     * Copies the selected text into the provided string, or clears it when nothing is selected.
     *
     * @param string Output string that receives the copied text.
     */
    void copy(std::string& string);

    /**
     * Cuts the selected text into the provided string, removing the selection from the text.
     *
     * @param string Output string that receives the cut text; cleared when nothing is selected.
     */
    void cut(std::string& string);

    /**
     * Restores a previously captured edit state atomically — text, styles,
     * caret, anchor, and selection — and re-syncs the string.
     *
     * @param snapshot The edit state to restore.
     */
    void restoreSnapshot(TextEditSnapshot const& snapshot);
private:
    struct _IndexEntry {
        FontGlyph const* glyphInfo;
        FontLine const* lineInfo;
        Font const* font;
        Vec4 color;
        Vec4 marker;
        float size;
        bool underline;
        bool strikeout;
    };

	struct _StyleRange {
		std::int64_t startIndex;
		std::int64_t endIndex;
		TextStyle style;
	};

    struct _EditState {
        std::int64_t caret;
        std::int64_t anchor;
        Vec4 caretRect;
        std::vector<Vec4> selectionRects;
        bool mouseIsDown;
        bool caretAffinity;
        std::optional<float> caretXGoal;
        std::vector<TextEditSnapshot> undoStack;
        std::vector<TextEditSnapshot> redoStack;
        int lastEditKind;
        std::int64_t lastEditCaret;
    };

    std::optional<bool> _secure;
    std::optional<bool> _multiline;
    std::optional<float> _scale;
    std::optional<float> _width;
    std::optional<float> _height;
    std::optional<float> _maxWidth;
    std::optional<float> _maxHeight;
    std::optional<float> _paddingTop;
    std::optional<float> _paddingLeft;
    std::optional<float> _paddingRight;
    std::optional<float> _paddingBottom;
    TextAlignment _alignment;
    TextJustify _justify;
    std::unique_ptr<_EditState> _editState;
    std::vector<std::uint32_t> _codepoints;
	mutable std::string _string;
	mutable std::vector<_StyleRange> _ranges;
    mutable std::vector<_IndexEntry> _index;
    mutable std::vector<TextLine> _lines;
    mutable std::vector<TextGlyph> _glyphs;
	mutable std::unique_ptr<Image> _image;
	mutable Vec2 _size;
    mutable bool _needsString;
    mutable bool _needsIndex;
    mutable bool _needsLayout;
    mutable bool _needsImage;
    mutable bool _needsEdit;

    std::int64_t _getMaxIndex() const;
    std::int64_t _getCaretRow() const;
    std::int64_t _getWordBoundaryLeft(std::int64_t) const;
    std::int64_t _getWordBoundaryRight(std::int64_t) const;
    std::int64_t _getRowAtPosition(Vec2 const&) const;
    std::int64_t _getIndexAtPosition(Vec2 const&) const;
    FontLine const& _getEmptyLine() const;

    void _updateString() const;
    void _updateIndex() const;
    void _updateLayout() const;
    void _updateImage() const;
    void _updateEdit() const;

    void _invalidateData();
    void _invalidateIndex();
    void _invalidateLayout();
    void _invalidateEdit();

    void _pushUndo(int, bool);
    void _endEdit();
    void _applySnapshot(TextEditSnapshot const&);
    void _setCaret(std::int64_t);
    void _setAnchor(std::int64_t);
    void _setCaretAffinity(bool);
    void _removeSelection();
    void _deleteSelection();
    void _insertCodepoints(std::int64_t, std::vector<std::uint32_t> const&);
    void _eraseCodepoints(std::int64_t, std::int64_t);
};

} /* namespace Rocket */
