#include <cmath>
#include <cassert>
#include <cfloat>
#include <limits>
#include <algorithm>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/String.hpp>
#include <Rocket/Paint/Text.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/Paint/Painter.hpp>

namespace Rocket {

static auto constexpr _TEXT_CARET_WIDTH = 2.0f;
static auto constexpr _TEXT_SECURE_BULLET = 0x2022u;
static auto constexpr _TEXT_UNDO_LIMIT = 200uz;

enum _EditKind {
    _EDIT_NONE,
    _EDIT_INSERT,
    _EDIT_DELETE_BACKWARD,
    _EDIT_DELETE_FORWARD
};

static Painter& _GetPainter() {
    PROFILE

    static auto _painter = Painter();

    return _painter;
}

bool TextStyle::operator==(TextStyle const& options) const {
    return (fontFamily == options.fontFamily)
        && (fontWeight == options.fontWeight)
        && (fontStyle == options.fontStyle)
        && (fontSize == options.fontSize)
        && (color == options.color)
        && (marker == options.marker)
        && (underline == options.underline)
        && (strikeout == options.strikeout);
}

bool TextStyle::operator!=(TextStyle const& options) const {
    return !operator==(options);
}

Text::~Text() {
    PROFILE
}

Text::Text()
    : _secure(std::nullopt)
    , _multiline(std::nullopt)
    , _scale(std::nullopt)
    , _width(std::nullopt)
    , _height(std::nullopt)
    , _maxWidth(std::nullopt)
    , _maxHeight(std::nullopt)
    , _paddingTop(std::nullopt)
    , _paddingLeft(std::nullopt)
    , _paddingRight(std::nullopt)
    , _paddingBottom(std::nullopt)
    , _alignment(TextAlignment::Start)
    , _justify(TextJustify::Start)
    , _editState(nullptr)
    , _codepoints()
    , _string()
    , _ranges()
    , _index()
    , _lines()
    , _glyphs()
    , _image(nullptr)
    , _size({ 0.0f, 0.0f })
    , _needsString(true)
    , _needsIndex(true)
    , _needsLayout(true)
    , _needsImage(true)
    , _needsEdit(true)
{
    PROFILE
}

Vec2 const& Text::getSize() const {
    PROFILE

    _updateIndex();
    _updateLayout();

    return _size;
}

Image const* Text::getImage() const {
    PROFILE

    _updateIndex();
    _updateLayout();
    _updateImage();

    return _image.get();
}

std::vector<TextLine> const& Text::getLines() const {
    PROFILE

    _updateIndex();
    _updateLayout();

    return _lines;
}

std::vector<TextGlyph> const& Text::getGlyphs() const {
    PROFILE

    _updateIndex();
    _updateLayout();

    return _glyphs;
}

std::optional<float> const& Text::getMaxWidth() const {
    PROFILE

    return _maxWidth;
}

std::optional<float> const& Text::getMaxHeight() const {
    PROFILE

    return _maxHeight;
}

std::optional<float> const& Text::getWidth() const {
    PROFILE

    return _width;
}

std::optional<float> const& Text::getHeight() const {
    PROFILE

    return _height;
}

std::optional<float> const& Text::getPaddingTop() const {
    PROFILE

    return _paddingTop;
}

std::optional<float> const& Text::getPaddingLeft() const {
    PROFILE

    return _paddingLeft;
}

std::optional<float> const& Text::getPaddingRight() const {
    PROFILE

    return _paddingRight;
}

std::optional<float> const& Text::getPaddingBottom() const {
    PROFILE

    return _paddingBottom;
}

TextAlignment const& Text::getAlignment() const {
    PROFILE

    return _alignment;
}

TextJustify const& Text::getJustify() const {
    PROFILE

    return _justify;
}

std::string const& Text::getString() const {
    PROFILE

    _updateString();

    return _string;
}

std::optional<float> const& Text::getScale() const {
    PROFILE

    return _scale;
}

std::optional<bool> Text::getSecure() const {
    PROFILE

    return _secure;
}

std::optional<bool> Text::getMultiLine() const {
    PROFILE

    return _multiline;
}

std::vector<std::uint32_t> const& Text::getCodepoints() const {
    PROFILE

    return _codepoints;
}

std::optional<TextGlyph> Text::getGlyphAtPosition(Vec2 const& position) const {
    PROFILE

    _updateIndex();
    _updateLayout();

    auto lineDistance = FLT_MAX;
    auto lineFound = (TextLine const*)nullptr;

    for (auto& line : _lines) {
        auto const fullRect = Vec4{ 0.0f, line.rect.y, _size.width, line.rect.height };

        if (position.inRect(fullRect)) {
            lineFound = &line;
            break;
        } else {
            auto const edgePoint = fullRect.getEdgePoint(position);
            auto const distance = edgePoint.getDistance(position);

            if (distance < lineDistance) {
                lineDistance = distance;
                lineFound = &line;
            }
        }
    }

    if (lineFound) {
        auto glyphDistance = FLT_MAX;
        auto glyphFound = (TextGlyph const*)nullptr;

        for (auto i = lineFound->startIndex; i < lineFound->endIndex; i++) {
            auto& glyph = _glyphs[i];

            if (position.inRect(glyph.rect)) {
                return glyph;
            } else {
                auto const edgePoint = glyph.rect.getEdgePoint(position);
                auto const distance = edgePoint.getDistance(position);

                if (distance < glyphDistance) {
                    glyphDistance = distance;
                    glyphFound = &glyph;
                }
            }
        }

        if (glyphFound) {
            return *glyphFound;
        }
    }

    return std::nullopt;
}

std::vector<Vec4> const& Text::getSelectionRects() const {
    PROFILE

    static auto const empty = std::vector<Vec4>();

    if (_editState == nullptr) {
        return empty;
    }

    _updateEdit();

    return _editState->selectionRects;
}

Vec4 const& Text::getCaretRect() const {
    PROFILE

    static auto const empty = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };

    if (_editState == nullptr) {
        return empty;
    }

    _updateEdit();

    return _editState->caretRect;
}

std::int64_t Text::getCaretPosition() const {
    PROFILE

    if (_editState == nullptr) {
        return 0;
    }

    return _editState->caret;
}

std::int64_t Text::getAnchorPosition() const {
    PROFILE

    if (_editState == nullptr) {
        return 0;
    }

    return _editState->anchor;
}

TextEditSnapshot Text::getSnapshot() const {
    PROFILE

    if (_editState == nullptr) {
        return TextEditSnapshot{};
    }

    auto styles = std::vector<TextStyleRange>();

    styles.reserve(_ranges.size());
    for (auto const& range : _ranges) {
        styles.push_back({
            .startIndex = range.startIndex,
            .endIndex = range.endIndex,
            .style = range.style
        });
    }

    return TextEditSnapshot{
        .codepoints = _codepoints,
        .styles = std::move(styles),
        .caret = _editState->caret,
        .anchor = _editState->anchor,
        .caretXGoal = _editState->caretXGoal,
        .caretAffinity = _editState->caretAffinity
    };
}

bool Text::getEditable() const {
    PROFILE

    return (_editState != nullptr);
}

bool Text::isSelectedRange() const {
    PROFILE

    if (_editState == nullptr) {
        return false;
    }

    return (_editState->caret != _editState->anchor);
}

void Text::setMaxWidth(std::optional<float> const& maxWidth) {
    PROFILE

    if (_maxWidth != maxWidth) {
        _maxWidth = maxWidth;
        _invalidateLayout();
    }
}

void Text::setMaxHeight(std::optional<float> const& maxHeight) {
    PROFILE

    if (_maxHeight != maxHeight) {
        _maxHeight = maxHeight;
        _invalidateLayout();
    }
}

void Text::setWidth(std::optional<float> const& width) {
    PROFILE

    if (_width != width) {
        _width = width;
        _invalidateLayout();
    }
}

void Text::setHeight(std::optional<float> const& height) {
    PROFILE

    if (_height != height) {
        _height = height;
        _invalidateLayout();
    }
}

void Text::setPaddingTop(std::optional<float> const& paddingTop) {
    PROFILE

    if (_paddingTop != paddingTop) {
        _paddingTop = paddingTop;
        _invalidateLayout();
    }
}

void Text::setPaddingLeft(std::optional<float> const& paddingLeft) {
    PROFILE

    if (_paddingLeft != paddingLeft) {
        _paddingLeft = paddingLeft;
        _invalidateLayout();
    }
}

void Text::setPaddingRight(std::optional<float> const& paddingRight) {
    PROFILE

    if (_paddingRight != paddingRight) {
        _paddingRight = paddingRight;
        _invalidateLayout();
    }
}

void Text::setPaddingBottom(std::optional<float> const& paddingBottom) {
    PROFILE

    if (_paddingBottom != paddingBottom) {
        _paddingBottom = paddingBottom;
        _invalidateLayout();
    }
}

void Text::setAlignment(TextAlignment const& alignment) {
    PROFILE

    if (_alignment != alignment) {
        _alignment = alignment;
        _invalidateLayout();
    }
}

void Text::setJustify(TextJustify const& justify) {
    PROFILE

    if (_justify != justify) {
        _justify = justify;
        _invalidateLayout();
    }
}

void Text::setString(std::string const& string) {
    PROFILE

    _updateString();

    if (_string != string) {
        _string = string;

        StringToCodepoints(_string, _codepoints);

        _invalidateData();
        _needsString = false; /* _string itself is the fresh source, no need to re-encode it */

        if (_editState != nullptr) {
            _editState->caretXGoal = std::nullopt;
            _editState->undoStack.clear();
            _editState->redoStack.clear();
            _editState->lastEditKind = _EDIT_NONE;

            _setCaretAffinity(false);
            _setCaret(_editState->caret);
            _setAnchor(_editState->anchor);
        }
    }
}

void Text::setScale(std::optional<float> const& scale) {
    PROFILE

    if (_scale != scale) {
        _scale = scale;
        _invalidateIndex();
    }
}

void Text::setMultiLine(std::optional<bool> multiLine) {
    PROFILE

    if (_multiline != multiLine) {
        _multiline = multiLine;
        _invalidateLayout();
    }
}

void Text::setSecure(std::optional<bool> secure) {
    PROFILE

    if (_secure != secure) {
        _secure = secure;
        _invalidateIndex();
    }
}

void Text::setEditable(bool editable) {
    PROFILE

    if (editable == (_editState != nullptr)) {
        return;
    }

    if (editable == true) {
        _editState = std::make_unique<_EditState>();
        _editState->caret = 0;
        _editState->anchor = 0;
        _editState->caretRect = { 0.0f, 0.0f, 0.0f, 0.0f };
        _editState->mouseIsDown = false;
        _editState->caretAffinity = false;
        _editState->caretXGoal = std::nullopt;
        _editState->lastEditKind = _EDIT_NONE;
        _editState->lastEditCaret = 0;
    } else {
        _editState.reset();
    }

    _invalidateLayout();
}

void Text::setStyle(TextStyle const& style) {
    PROFILE

    _ranges = {{
        .startIndex = 0,
        .endIndex = std::numeric_limits<std::int64_t>::max(),
        .style = style
    }};

    _invalidateIndex();
}

void Text::addStyle(TextStyle const& style) {
    PROFILE

    _ranges.push_back({
        .startIndex = 0,
        .endIndex = std::numeric_limits<std::int64_t>::max(),
        .style = style
    });

    _invalidateIndex();
}

void Text::addStyle(std::int64_t startIndex, std::int64_t endIndex, TextStyle const& style) {
    PROFILE

    _ranges.push_back({
        .startIndex = startIndex,
        .endIndex = endIndex,
        .style = style
    });

    _invalidateIndex();
}

void Text::setStyles(std::vector<TextStyleRange> const& styles) {
    PROFILE

    auto const equal = (
        (_ranges.size() == styles.size()) &&
        std::equal(
            _ranges.begin(),
            _ranges.end(),
            styles.begin(),
            [](auto const& range, auto const& style) {
                return (range.startIndex == style.startIndex)
                    && (range.endIndex == style.endIndex)
                    && (range.style == style.style);
            }
        )
    );

    if (equal == false) {
        _ranges.clear();

        for (auto const& style : styles) {
            _ranges.push_back({
                .startIndex = style.startIndex,
                .endIndex = style.endIndex,
                .style = style.style
            });
        }

        _invalidateIndex();
    }
}

void Text::clearStyle() {
    PROFILE

    _ranges.clear();

    _invalidateIndex();
}

void Text::input(std::string const& string) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    static thread_local auto _stringCodepoints = std::vector<std::uint32_t>();

    StringToCodepoints(string, _stringCodepoints);

    if (_multiline.value_or(true) == false) {
        auto writeIndex = 0llu;
        for (auto readIndex = 0llu; readIndex < _stringCodepoints.size(); readIndex += 1) {
            auto const codepoint = _stringCodepoints[readIndex];
            if (codepoint == '\r') {
                if (((readIndex + 1) < _stringCodepoints.size()) && (_stringCodepoints[readIndex + 1] == '\n')) {
                    readIndex += 1;
                }
                _stringCodepoints[writeIndex++] = ' ';
            } else if (codepoint == '\n') {
                _stringCodepoints[writeIndex++] = ' ';
            } else {
                _stringCodepoints[writeIndex++] = codepoint;
            }
        }
        _stringCodepoints.resize(writeIndex);
    } else {
        auto writeIndex = 0llu;
        for (auto readIndex = 0llu; readIndex < _stringCodepoints.size(); readIndex += 1) {
            auto const codepoint = _stringCodepoints[readIndex];
            if (codepoint == '\r') {
                if (((readIndex + 1) < _stringCodepoints.size()) && (_stringCodepoints[readIndex + 1] == '\n')) {
                    readIndex += 1;
                }
                _stringCodepoints[writeIndex++] = '\n';
            } else {
                _stringCodepoints[writeIndex++] = codepoint;
            }
        }
        _stringCodepoints.resize(writeIndex);
    }

    if (_stringCodepoints.empty()) {
        return;
    }

    auto const startsWord = (_stringCodepoints.size() == 1) && (CodepointIsWhitespace(_stringCodepoints.front()) == false);

    _pushUndo(_EDIT_INSERT, startsWord);

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (isSelectedRange()) {
        _removeSelection();
    }

    _insertCodepoints(_editState->caret, _stringCodepoints);

    _setCaret(_editState->caret + _stringCodepoints.size());
    _setAnchor(_editState->caret);
    _invalidateData();
    _endEdit();
}

void Text::moveUp(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;

    if (_multiline.value_or(true) == false) {
        moveLineStart(selection);
        return;
    }

    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();

    if (glyphs.empty()) {
        return;
    }

    _updateEdit();

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::min(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
        _setCaretAffinity(false);
    }

    auto const x = _editState->caretXGoal.value_or(_editState->caretRect.x);
    auto const row = _getCaretRow();
    auto index = (std::int64_t)0;
    auto affinity = false;

    if (row > 0) {
        auto const& line = lines[row - 1];
        auto const point = Vec2{ x, line.rect.y + line.rect.height / 2.0f };

        index = _getIndexAtPosition(point);

        auto const lineHasGlyphs = (line.startIndex < line.endIndex);
        auto const atLineEnd = ((index > 0) && (index == line.endIndex));
        auto const beforeTextEnd = (index < (std::int64_t)glyphs.size());
        auto const afterBreakline = ((index > 0) && (glyphs[index - 1].codepoint == U'\n'));

        if (atLineEnd && lineHasGlyphs && afterBreakline) {
            index = line.endIndex - 1;
        } else if (atLineEnd && beforeTextEnd && (afterBreakline == false)) {
            affinity = true;
        }
    }

    if (selection) {
        _setCaret(index);
    } else {
        _setCaret(index);
        _setAnchor(_editState->caret);
    }

    _setCaretAffinity(affinity);
    _editState->caretXGoal = x;
}

void Text::moveLeft(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;
    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::min(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
        return;
    }

    if (_editState->caret <= 0) {
        return;
    }

    if (selection) {
        _setCaret(GraphemePrev(_codepoints, _editState->caret));
    } else {
        _setCaret(GraphemePrev(_codepoints, _editState->caret));
        _setAnchor(_editState->caret);
    }
}

void Text::moveRight(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;
    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::max(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
        return;
    }

    if (_editState->caret >= (std::int64_t)_codepoints.size()) {
        return;
    }

    if (selection) {
        _setCaret(GraphemeNext(_codepoints, _editState->caret));
    } else {
        _setCaret(GraphemeNext(_codepoints, _editState->caret));
        _setAnchor(_editState->caret);
    }
}

void Text::moveDown(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;

    if (_multiline.value_or(true) == false) {
        moveLineEnd(selection);
        return;
    }

    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();

    if (glyphs.empty()) {
        return;
    }

    _updateEdit();

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::max(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
        _setCaretAffinity(false);
    }

    auto const x = _editState->caretXGoal.value_or(_editState->caretRect.x);
    auto const row = _getCaretRow();
    auto index = _getMaxIndex();
    auto affinity = false;

    if ((row + 1) < (std::int64_t)lines.size()) {
        auto const& line = lines[row + 1];
        auto const point = Vec2{ x, line.rect.y + line.rect.height / 2.0f };

        index = _getIndexAtPosition(point);

        auto const lineHasGlyphs = (line.startIndex < line.endIndex);
        auto const atLineEnd = ((index > 0) && (index == line.endIndex));
        auto const beforeTextEnd = (index < (std::int64_t)glyphs.size());
        auto const afterBreakline = ((index > 0) && (glyphs[index - 1].codepoint == U'\n'));

        if (atLineEnd && lineHasGlyphs && afterBreakline) {
            index = line.endIndex - 1;
        } else if (atLineEnd && beforeTextEnd && (afterBreakline == false)) {
            affinity = true;
        }
    }

    if (selection) {
        _setCaret(index);
    } else {
        _setCaret(index);
        _setAnchor(_editState->caret);
    }

    _setCaretAffinity(affinity);
    _editState->caretXGoal = x;
}

void Text::moveWordLeft(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;
    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::min(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
    }

    if (_editState->caret <= 0) {
        return;
    }

    auto const index = _getWordBoundaryLeft(_editState->caret);

    if (selection) {
        _setCaret(index);
    } else {
        _setCaret(index);
        _setAnchor(_editState->caret);
    }
}

void Text::moveWordRight(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;
    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::max(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
    }

    if (_editState->caret >= (std::int64_t)_codepoints.size()) {
        return;
    }

    auto const index = _getWordBoundaryRight(_editState->caret);

    if (selection) {
        _setCaret(index);
    } else {
        _setCaret(index);
        _setAnchor(_editState->caret);
    }
}

void Text::moveLineStart(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;

    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();

    _editState->caretXGoal = std::nullopt;

    if (glyphs.empty()) {
        return;
    }

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::min(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
        _setCaretAffinity(false);
    }

    auto const row = _getCaretRow();

    _setCaretAffinity(false);

    if (selection) {
        _setCaret(lines[row].startIndex);
    } else {
        _setCaret(lines[row].startIndex);
        _setAnchor(_editState->caret);
    }
}

void Text::moveLineEnd(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;

    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();

    _editState->caretXGoal = std::nullopt;

    if (glyphs.empty()) {
        return;
    }

    if ((selection == false) && isSelectedRange()) {
        auto const collapseIndex = std::max(_editState->caret, _editState->anchor);

        _setCaret(collapseIndex);
        _setAnchor(collapseIndex);
        _setCaretAffinity(false);
    }

    auto const row = _getCaretRow();
    auto const& line = lines[row];
    auto index = line.endIndex;
    auto affinity = false;

    auto const lineHasGlyphs = (line.startIndex < index);
    auto const beforeTextEnd = (index < (std::int64_t)glyphs.size());
    auto const afterBreakline = ((index > 0) && (glyphs[index - 1].codepoint == U'\n'));

    if (lineHasGlyphs && afterBreakline) {
        index -= 1;
    } else if (beforeTextEnd) {
        affinity = true;
    }

    if (selection) {
        _setCaret(index);
    } else {
        _setCaret(index);
        _setAnchor(_editState->caret);
    }

    _setCaretAffinity(affinity);
}

void Text::moveDocumentStart(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;
    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (selection) {
        _setCaret(0);
    } else {
        _setCaret(0);
        _setAnchor(_editState->caret);
    }
}

void Text::moveDocumentEnd(bool selection) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;
    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (selection) {
        _setCaret(_getMaxIndex());
    } else {
        _setCaret(_getMaxIndex());
        _setAnchor(_editState->caret);
    }
}

void Text::deleteForward() {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (isSelectedRange()) {
        _pushUndo(_EDIT_DELETE_FORWARD, false);
        _removeSelection();
        _invalidateData();
    } else {
        if (_editState->caret >= _getMaxIndex()) {
            return;
        }

        _pushUndo(_EDIT_DELETE_FORWARD, true);
        _eraseCodepoints(_editState->caret, GraphemeNext(_codepoints, _editState->caret));
        _invalidateData();
    }
    _endEdit();
}

void Text::deleteBackward() {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (isSelectedRange()) {
        _pushUndo(_EDIT_DELETE_BACKWARD, false);
        _removeSelection();
        _invalidateData();
    } else {
        if (_editState->caret <= 0) {
            return;
        }

        _pushUndo(_EDIT_DELETE_BACKWARD, true);

        auto const index = GraphemePrev(_codepoints, _editState->caret);

        _eraseCodepoints(index, _editState->caret);

        _setCaret(index);
        _setAnchor(index);
        _invalidateData();
    }

    _endEdit();
}

void Text::deleteWordForward() {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (isSelectedRange()) {
        _pushUndo(_EDIT_DELETE_FORWARD, false);
        _removeSelection();
        _invalidateData();
    } else {
        if (_editState->caret >= _getMaxIndex()) {
            return;
        }

        _pushUndo(_EDIT_NONE, false);

        auto const index = _getWordBoundaryRight(_editState->caret);

        _eraseCodepoints(_editState->caret, index);
        _invalidateData();
    }

    _endEdit();
}

void Text::deleteWordBackward() {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (isSelectedRange()) {
        _pushUndo(_EDIT_DELETE_BACKWARD, false);
        _removeSelection();
        _invalidateData();
    } else {
        if (_editState->caret <= 0) {
            return;
        }

        _pushUndo(_EDIT_NONE, false);

        auto const index = _getWordBoundaryLeft(_editState->caret);

        _eraseCodepoints(index, _editState->caret);

        _setCaret(index);
        _setAnchor(index);
        _invalidateData();
    }

    _endEdit();
}

void Text::deleteLineBackward() {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (isSelectedRange()) {
        _pushUndo(_EDIT_DELETE_BACKWARD, false);
        _removeSelection();
        _invalidateData();
    } else {
        auto const& lines = getLines();

        if (_editState->caret <= 0 || lines.empty()) {
            return;
        }

        auto const index = lines[_getCaretRow()].startIndex;

        if (index >= _editState->caret) {
            return;
        }

        _pushUndo(_EDIT_NONE, false);
        _eraseCodepoints(index, _editState->caret);
        _setCaret(index);
        _setAnchor(index);
        _invalidateData();
    }

    _endEdit();
}

void Text::deleteLineForward() {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    if (isSelectedRange()) {
        _pushUndo(_EDIT_DELETE_FORWARD, false);
        _removeSelection();
        _invalidateData();
    } else {
        auto const& glyphs = getGlyphs();
        auto const& lines = getLines();

        if (_editState->caret >= _getMaxIndex() || lines.empty()) {
            return;
        }

        auto const& line = lines[_getCaretRow()];
        auto index = line.endIndex;

        if ((index > line.startIndex) && (glyphs[index - 1].codepoint == U'\n')) {
            index -= 1;
        }

        if (index <= _editState->caret) {
            index = std::min(_getMaxIndex(), _editState->caret + 1); /* at a line end: the break itself goes */
        }

        _pushUndo(_EDIT_NONE, false);
        _eraseCodepoints(_editState->caret, index);
        _invalidateData();
    }

    _endEdit();
}

void Text::selectWordAt(std::int64_t index) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    auto const count = _getMaxIndex();

    if (count == 0) {
        return;
    }

    index = std::clamp(index, (std::int64_t)0, count - 1);

    auto start = index;
    auto end = index + 1;

    if (CodepointIsWhitespace(_codepoints[index]) && (_secure.value_or(false) == false)) {
        while ((start > 0) && CodepointIsWhitespace(_codepoints[start - 1])) start -= 1;
        while ((end < count) && CodepointIsWhitespace(_codepoints[end])) end += 1;
    } else {
        start = _getWordBoundaryLeft(index + 1);
        end = _getWordBoundaryRight(index);
    }

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);
    _setAnchor(start);
    _setCaret(end);
    _editState->lastEditKind = _EDIT_NONE;
}

void Text::selectParagraphAt(std::int64_t index) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    auto const count = _getMaxIndex();

    index = std::clamp(index, (std::int64_t)0, count);

    auto start = index;
    auto end = index;

    while ((start > 0) && (_codepoints[start - 1] != '\n')) start -= 1;
    while ((end < count) && (_codepoints[end] != '\n')) end += 1;

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);
    _setAnchor(start);
    _setCaret(end);
    _editState->lastEditKind = _EDIT_NONE;
}

void Text::undo() {
    PROFILE

    if ((_editState == nullptr) || _editState->undoStack.empty()) {
        return;
    }

    _editState->redoStack.push_back(getSnapshot());

    auto const snapshot = _editState->undoStack.back();
    _editState->undoStack.pop_back();
    _applySnapshot(snapshot);
    _editState->lastEditKind = _EDIT_NONE;
}

void Text::redo() {
    PROFILE

    if ((_editState == nullptr) || _editState->redoStack.empty()) {
        return;
    }

    _editState->undoStack.push_back(getSnapshot());

    auto const snapshot = _editState->redoStack.back();
    _editState->redoStack.pop_back();
    _applySnapshot(snapshot);
    _editState->lastEditKind = _EDIT_NONE;
}

bool Text::canUndo() const {
    PROFILE

    return (_editState != nullptr) && (_editState->undoStack.empty() == false);
}

bool Text::canRedo() const {
    PROFILE

    return (_editState != nullptr) && (_editState->redoStack.empty() == false);
}

void Text::selectAll() {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE;
    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    _setAnchor(0);
    _setCaret(_getMaxIndex());
}

void Text::mouseDown(Vec2 const& position, bool extend, int clickCount) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    if (_editState->mouseIsDown) {
        mouseUp(position);
    }

    _editState->caretXGoal = std::nullopt;
    _editState->lastEditKind = _EDIT_NONE;

    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();
    auto const index = _getIndexAtPosition(position);

    if ((extend == false) && (clickCount >= 2)) {
        if (clickCount == 2) {
            selectWordAt((index < _getMaxIndex()) ? index : (index - 1));
        } else {
            selectParagraphAt(index);
        }
        _editState->mouseIsDown = true;
        return;
    }

    auto const row = _getRowAtPosition(position);
    auto affinity = false;

    if (row >= 0) {
        auto const& line = lines[row];
        auto const lineHasGlyphs = (line.startIndex < line.endIndex);
        auto const atLineEnd = (index == line.endIndex);
        auto const beforeTextEnd = (index < (std::int64_t)glyphs.size());
        auto const afterBreakline = ((index > 0) && (glyphs[index - 1].codepoint == U'\n'));

        affinity = (
            lineHasGlyphs &&
            atLineEnd &&
            beforeTextEnd &&
            (afterBreakline == false)
        );
    }

    if (extend) {
        _setCaret(index);
    } else {
        _setCaret(index);
        _setAnchor(index);
    }

    _setCaretAffinity(affinity);
    _editState->mouseIsDown = true;
}

void Text::mouseMove(Vec2 const& position) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    if (_editState->mouseIsDown) {
        _editState->caretXGoal = std::nullopt;

        auto const& glyphs = getGlyphs();
        auto const& lines = getLines();
        auto const index = _getIndexAtPosition(position);
        auto const row = _getRowAtPosition(position);
        auto affinity = false;

        if (row >= 0) {
            auto const& line = lines[row];
            auto const lineHasGlyphs = (line.startIndex < line.endIndex);
            auto const atLineEnd = (index == line.endIndex);
            auto const beforeTextEnd = (index < (std::int64_t)glyphs.size());
            auto const afterBreakline = ((index > 0) && (glyphs[index - 1].codepoint == U'\n'));

            affinity = (
                lineHasGlyphs &&
                atLineEnd &&
                beforeTextEnd &&
                (afterBreakline == false)
            );
        }

        _setCaret(index);
        _setCaretAffinity(affinity);
    }
}

void Text::mouseUp(Vec2 const& position) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _editState->mouseIsDown = false;
}

void Text::paste(std::string const& string) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    if (string.empty()) {
        return;
    }

    _editState->lastEditKind = _EDIT_NONE; /* a paste is its own undo group */
    input(string);
    _editState->lastEditKind = _EDIT_NONE;
}

void Text::copy(std::string& string) {
    PROFILE

    if (_editState == nullptr) {
        string.clear();

        return;
    }

    if (isSelectedRange()) {
        auto const fromIndex = std::min(_editState->caret, _editState->anchor);
        auto const toIndex = std::max(_editState->caret, _editState->anchor);

        auto const selectionCodepoints = std::vector<std::uint32_t>(
            _codepoints.begin() + fromIndex,
            _codepoints.begin() + toIndex
        );

        StringFromCodepoints(selectionCodepoints, string);
    } else {
        string.clear();
    }
}

void Text::cut(std::string& string) {
    PROFILE

    copy(string);

    if (isSelectedRange()) {
        _pushUndo(_EDIT_NONE, false);
        _deleteSelection();
        _endEdit();
    }
}

void Text::restoreSnapshot(TextEditSnapshot const& snapshot) {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    _applySnapshot(snapshot);
}

std::int64_t Text::_getMaxIndex() const {
    PROFILE

    return (std::int64_t)_codepoints.size();
}

std::int64_t Text::_getCaretRow() const {
    PROFILE

    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();

    if (glyphs.empty()) {
        return 0;
    }

    auto const caret = _editState->caret;
    auto const beforeTextEnd = (caret < (std::int64_t)glyphs.size());
    auto const hasPrevGlyph = ((caret > 0) && beforeTextEnd);
    auto const afterBreakline = (hasPrevGlyph && (glyphs[caret - 1].codepoint == U'\n'));
    auto const atLineEnd = (hasPrevGlyph && (caret == lines[glyphs[caret - 1].rowIndex].endIndex));
    auto row = beforeTextEnd ? glyphs[caret].rowIndex : lines.back().rowIndex;

    if (_editState->caretAffinity && atLineEnd && (afterBreakline == false)) {
        row = glyphs[caret - 1].rowIndex;
    }

    return row;
}

std::int64_t Text::_getWordBoundaryLeft(std::int64_t index) const {
    PROFILE

    if (_secure.value_or(false)) {
        return 0; /* masked text is one run: word moves reveal nothing */
    }

    while ((index > 0) && CodepointIsWhitespace(_codepoints[index - 1])) {
        index -= 1;
    }

    if ((index > 0) && CodepointIsWordChar(_codepoints[index - 1])) {
        while ((index > 0) && CodepointIsWordChar(_codepoints[index - 1])) {
            index -= 1;
        }
    } else {
        while (
            (index > 0) &&
            (CodepointIsWhitespace(_codepoints[index - 1]) == false) &&
            (CodepointIsWordChar(_codepoints[index - 1]) == false)
        ) {
            index -= 1;
        }
    }

    return index;
}

std::int64_t Text::_getWordBoundaryRight(std::int64_t index) const {
    PROFILE

    auto const count = (std::int64_t)_codepoints.size();

    if (_secure.value_or(false)) {
        return count;
    }

    while ((index < count) && CodepointIsWhitespace(_codepoints[index])) {
        index += 1;
    }

    if ((index < count) && CodepointIsWordChar(_codepoints[index])) {
        while ((index < count) && CodepointIsWordChar(_codepoints[index])) {
            index += 1;
        }
    } else {
        while (
            (index < count) &&
            (CodepointIsWhitespace(_codepoints[index]) == false) &&
            (CodepointIsWordChar(_codepoints[index]) == false)
        ) {
            index += 1;
        }
    }

    return index;
}

std::int64_t Text::_getRowAtPosition(Vec2 const& position) const {
    PROFILE

    auto const& lines = getLines();
    auto const& size = getSize();

    auto rowDistance = FLT_MAX;
    auto rowFound = (std::int64_t)-1;

    for (auto& line : lines) {
        auto const fullRect = Vec4{ 0.0f, line.rect.y, size.width, line.rect.height };

        if (position.inRect(fullRect)) {
            return line.rowIndex;
        } else {
            auto const edgePoint = fullRect.getEdgePoint(position);
            auto const distance = edgePoint.getDistance(position);

            if (distance < rowDistance) {
                rowDistance = distance;
                rowFound = line.rowIndex;
            }
        }
    }

    return rowFound;
}

std::int64_t Text::_getIndexAtPosition(Vec2 const& position) const {
    PROFILE

    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();

    auto const row = _getRowAtPosition(position);

    if (row < 0) {
        return 0;
    }

    auto& line = lines[row];

    if (line.startIndex == line.endIndex) {
        return line.startIndex;
    }

    auto glyphDistance = FLT_MAX;
    auto glyphIndex = line.startIndex;

    for (auto i = line.startIndex; i < line.endIndex; i++) {
        auto& glyph = glyphs[i];

        if ((position.x >= glyph.rect.x) && (position.x < glyph.rect.getMaxX())) {
            glyphIndex = i;
            break;
        } else {
            auto const distance = std::min(
                std::abs(position.x - glyph.rect.x),
                std::abs(position.x - glyph.rect.getMaxX())
            );

            if (distance < glyphDistance) {
                glyphDistance = distance;
                glyphIndex = i;
            }
        }
    }

    auto& glyph = glyphs[glyphIndex];

    if (glyph.codepoint == U'\n') {
        return glyph.index;
    }

    if (position.x < (glyph.rect.x + glyph.rect.width / 2.0f)) {
        return glyph.index;
    }

    return glyph.index + 1;
}

FontLine const& Text::_getEmptyLine() const {
    PROFILE

    auto fontFamily = std::optional<std::string>();
    auto fontWeight = std::optional<FontWeight>();
    auto fontStyle = std::optional<FontStyle>();
    auto fontSize = std::ceilf(TEXT_DEFAULT_FONT_SIZE * _scale.value_or(1.0f));

    for (auto const& range : _ranges) {
        fontFamily = range.style.fontFamily.has_value() ? range.style.fontFamily : fontFamily;
        fontWeight = range.style.fontWeight.has_value() ? range.style.fontWeight : fontWeight;
        fontStyle = range.style.fontStyle.has_value() ? range.style.fontStyle : fontStyle;
        fontSize = std::ceilf(range.style.fontSize.has_value() ? (range.style.fontSize.value() * _scale.value_or(1.0f)) : fontSize);
    }

    auto font = Font::Find(
        fontFamily.value_or(TEXT_DEFAULT_FONT_FAMILY),
        fontWeight.value_or(TEXT_DEFAULT_FONT_WEIGHT),
        fontStyle.value_or(TEXT_DEFAULT_FONT_STYLE)
    );

    if (font == nullptr) {
        font = &Font::GetDefault(
            fontWeight.value_or(TEXT_DEFAULT_FONT_WEIGHT),
            fontStyle.value_or(TEXT_DEFAULT_FONT_STYLE)
        );
    }

    return font->getLine(fontSize);
}

void Text::_updateString() const {
    PROFILE

    if (_needsString == false) {
        return;
    }

    _needsString = false;

    StringFromCodepoints(_codepoints, _string);
}

void Text::_updateIndex() const {
    PROFILE

    static thread_local auto _boundaries = std::vector<std::int64_t>();

    if (_needsIndex == false) {
        return;
    }

    _needsIndex = false;

    auto const secure = _secure.value_or(false);
    auto const codepointCount = (std::int64_t)_codepoints.size();
    auto const rangesCount = (std::int64_t)_ranges.size();

    _boundaries.clear();
    _boundaries.reserve(rangesCount * 2);
    _boundaries.push_back(0);

    for (auto const& range : _ranges) {
        if (
            (range.startIndex > 0) &&
            (range.startIndex < codepointCount)
        ) {
            _boundaries.push_back(range.startIndex);
        }
        if (
            (range.endIndex > 0) &&
            (range.endIndex < codepointCount)
        ) {
            _boundaries.push_back(range.endIndex);
        }
    }

    _boundaries.push_back(codepointCount);

    std::sort(_boundaries.begin(), _boundaries.end());

    _boundaries.erase(std::unique(_boundaries.begin(), _boundaries.end()), _boundaries.end());

    _index.clear();
    _index.reserve(codepointCount);

    for (auto segment = 0ull; (segment + 1) < _boundaries.size(); segment++) {
        auto const segmentStart = _boundaries[segment];
        auto const segmentEnd = _boundaries[segment + 1];

        auto fontFamily = std::optional<std::string>();
        auto fontWeight = std::optional<FontWeight>();
        auto fontStyle = std::optional<FontStyle>();
        auto fontSize = std::ceilf(TEXT_DEFAULT_FONT_SIZE * _scale.value_or(1.0f));
        auto color = TEXT_DEFAULT_COLOR;
        auto marker = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
        auto underline = false;
        auto strikeout = false;

        for (auto const& range : _ranges) {
            if (
                (segmentStart >= range.startIndex) &&
                (segmentStart < range.endIndex)
            ) {
                fontFamily = range.style.fontFamily.has_value() ? range.style.fontFamily : fontFamily;
                fontWeight = range.style.fontWeight.has_value() ? range.style.fontWeight : fontWeight;
                fontStyle = range.style.fontStyle.has_value() ? range.style.fontStyle : fontStyle;
                fontSize = std::ceilf(range.style.fontSize.has_value() ? (range.style.fontSize.value() * _scale.value_or(1.0f)) : fontSize);
                color = range.style.color.value_or(color);
                marker = range.style.marker.value_or(marker);
                underline = range.style.underline.value_or(underline);
                strikeout = range.style.strikeout.value_or(strikeout);
            }
        }

        auto font = Font::Find(
            fontFamily.value_or(TEXT_DEFAULT_FONT_FAMILY),
            fontWeight.value_or(TEXT_DEFAULT_FONT_WEIGHT),
            fontStyle.value_or(TEXT_DEFAULT_FONT_STYLE)
        );

        if (font == nullptr) {
            font = &Font::GetDefault(
                fontWeight.value_or(TEXT_DEFAULT_FONT_WEIGHT),
                fontStyle.value_or(TEXT_DEFAULT_FONT_STYLE)
            );
        }

        auto const& lineInfo = font->getLine(fontSize);

        for (auto index = segmentStart; index < segmentEnd; index++) {
            auto const codepoint = (secure ? _TEXT_SECURE_BULLET : _codepoints[index]);
            auto const& glyphInfo = font->getGlyph(codepoint, fontSize);

            _index.push_back({
                .glyphInfo = &glyphInfo,
                .lineInfo = &lineInfo,
                .font = font,
                .color = color,
                .marker = marker,
                .size = fontSize,
                .underline = underline,
                .strikeout = strikeout
            });
        }
    }
}

void Text::_updateLayout() const {
    PROFILE

    if (_needsLayout == false) {
        return;
    }

    _needsLayout = false;

    auto const paddingTop = _paddingTop.value_or(0.0f);
    auto const paddingLeft = _paddingLeft.value_or(0.0f);
    auto const paddingRight = _paddingRight.value_or(0.0f);
    auto const paddingBottom = _paddingBottom.value_or(0.0f);
    auto const limit = (_multiline.value_or(true) == false)
        ? FLT_MAX /* single-line editing never wraps; the caller scrolls the overflow */
        : std::max(0.0f, (_width.value_or(_maxWidth.value_or(FLT_MAX)) - paddingLeft - paddingRight));

    auto const secure = _secure.value_or(false);
    auto const renderedCodepoint = [&](std::int64_t index) {
        return (secure ? _TEXT_SECURE_BULLET : _codepoints[index]);
    };

    auto prevIsWhitespace = false;
    auto prevIsBreakline = false;
    auto wordStartIndex = 0ll;
    auto lineStartIndex = 0ll;
    auto rowIndex = 0ll;

    _glyphs.clear();
    _lines.clear();
    _glyphs.reserve(_index.size());

    for (auto index = 0ll; index < _index.size(); index++) {
        auto const& indexEntry = _index[index];
        auto const codepoint = renderedCodepoint(index);

        if (prevIsWhitespace && (indexEntry.glyphInfo->whitespace == false)) {
            wordStartIndex = index;
        }

        if (prevIsBreakline) {
            wordStartIndex = index;
        }

        if (index == 0) {
            _lines.push_back({
                .rect = {},
                .rowIndex = 0,
                .startIndex = 0,
                .endIndex = 0
            });

            _glyphs.push_back({
                .rect = { 0.0f, 0.0f, indexEntry.glyphInfo->advance, 0.0f },
                .bitmapRect = { {}, indexEntry.glyphInfo->imageSlice.size },
                .index = index,
                .rowIndex = 0,
                .colIndex = 0,
                .codepoint = codepoint,
                .font = indexEntry.font,
                .color = indexEntry.color,
                .marker = indexEntry.marker,
                .size = indexEntry.size,
                .underline = indexEntry.underline,
                .strikeout = indexEntry.strikeout,
                .bearing = indexEntry.glyphInfo->bearing,
                .ascent = indexEntry.lineInfo->ascent,
                .descent = indexEntry.lineInfo->descent,
                .leading = indexEntry.lineInfo->leading,
                .whitespace = indexEntry.glyphInfo->whitespace,
                .image = indexEntry.glyphInfo->image,
                .imageSlice = indexEntry.glyphInfo->imageSlice
            });
        } else {
            auto const maxX = (_glyphs.back().rect.getMaxX() + indexEntry.glyphInfo->advance);
            auto const overflow = (maxX > limit);
            auto wrap = ((overflow && (wordStartIndex != lineStartIndex)) || prevIsBreakline);
            auto const emergencyWrap = (overflow && (wrap == false) && (indexEntry.glyphInfo->whitespace == false) && (_glyphs.back().rect.getMaxX() > 0.0f));
            auto xPos = _glyphs.back().rect.getMaxX();

            if (emergencyWrap) {
                wordStartIndex = index;
                wrap = true;
            }

            if (wrap) {
                rowIndex += 1;

                _lines.back().endIndex = wordStartIndex;
                _lines.push_back({
                    .rect = {},
                    .rowIndex = rowIndex,
                    .startIndex = wordStartIndex,
                    .endIndex = 0
                });

                if (wordStartIndex < index) {
                    auto const offset = _glyphs[wordStartIndex].rect.x;
                    for (auto i = wordStartIndex; i < _glyphs.size(); i++) {
                        _glyphs[i].rect.x -= offset;
                        _glyphs[i].rowIndex = rowIndex;
                    }
                    xPos = _glyphs.back().rect.getMaxX();
                } else {
                    xPos = 0.0f;
                }

                lineStartIndex = wordStartIndex;
            }

            if (wrap == false) {
                auto const& prevEntry = _index[index - 1];
                if (
                    (prevEntry.font == indexEntry.font) &&
                    (prevEntry.size == indexEntry.size)
                ) {
                    xPos += indexEntry.font->getKerning(renderedCodepoint(index - 1), codepoint, indexEntry.size);
                }
            }

            _glyphs.push_back({
                .rect = {
                    xPos,
                    0.0f,
                    indexEntry.glyphInfo->advance,
                    0.0f
                },
                .bitmapRect = { {}, indexEntry.glyphInfo->imageSlice.size },
                .index = index,
                .rowIndex = rowIndex,
                .colIndex = 0,
                .codepoint = codepoint,
                .font = indexEntry.font,
                .color = indexEntry.color,
                .marker = indexEntry.marker,
                .size = indexEntry.size,
                .underline = indexEntry.underline,
                .strikeout = indexEntry.strikeout,
                .bearing = indexEntry.glyphInfo->bearing,
                .ascent = indexEntry.lineInfo->ascent,
                .descent = indexEntry.lineInfo->descent,
                .leading = indexEntry.lineInfo->leading,
                .whitespace = indexEntry.glyphInfo->whitespace,
                .image = indexEntry.glyphInfo->image,
                .imageSlice = indexEntry.glyphInfo->imageSlice
            });
        }

        prevIsWhitespace = indexEntry.glyphInfo->whitespace;
        prevIsBreakline = (codepoint == U'\n');
    }

    if ((_lines.size() > 0) && (_glyphs.size() > 0)) {
        _lines.back().endIndex = (std::int64_t)_glyphs.size();
    }

    if ((_index.empty() == false) && (renderedCodepoint((std::int64_t)_index.size() - 1) == U'\n')) {
        _lines.push_back({
            .rect = {},
            .rowIndex = (_lines.back().rowIndex + 1),
            .startIndex = (std::int64_t)_glyphs.size(),
            .endIndex = (std::int64_t)_glyphs.size()
        });
    }

    auto maxTextWidth = 0.0f;
    auto maxTextHeight = 0.0f;

    for (auto i = 0ll; i < _lines.size(); i++) {
        auto& line = _lines[i];
        assert(line.rowIndex == i);

        auto maxWidth = 0.0f;
        auto maxAscent = 0.0f;
        auto maxDescent = 0.0f;
        auto maxLeading = 0.0f;

        for (auto i = line.startIndex; i < line.endIndex; i++) {
            auto& glyph = _glyphs[i];
            assert(glyph.rowIndex == line.rowIndex);

            maxWidth = std::max(maxWidth, glyph.rect.getMaxX());
            maxAscent = std::max(maxAscent, glyph.ascent);
            maxDescent = std::max(maxDescent, glyph.descent);
            maxLeading = std::max(maxLeading, glyph.leading);
        }

        if ((line.startIndex == line.endIndex) && (i > 0)) {
            auto const& prevLine = _lines[i - 1];
            maxAscent = prevLine.ascent;
            maxDescent = prevLine.descent;
            maxLeading = prevLine.leading;
        }

        line.ascent = maxAscent;
        line.descent = maxDescent;
        line.leading = maxLeading;

        line.rect.x = 0;
        line.rect.width = maxWidth;
        line.rect.height = (maxAscent + maxDescent);
        if (i > 0) {
            auto const& prevLine = _lines[i - 1];
            line.rect.y = (prevLine.rect.getMaxY() + prevLine.leading);
        }

        for (auto i = line.startIndex; i < line.endIndex; i++) {
            auto& glyph = _glyphs[i];
            assert(glyph.rowIndex == line.rowIndex);

            glyph.rect.y = line.rect.y;
            glyph.rect.height = line.rect.height;
            glyph.bitmapRect.x = (glyph.rect.x + glyph.bearing.x);
            glyph.bitmapRect.y = (glyph.rect.y + maxAscent - glyph.bearing.y);
        }

        maxTextWidth = std::max(maxTextWidth, maxWidth);
        maxTextHeight = std::max(maxTextHeight, line.rect.getMaxY());
    }

    if (_glyphs.empty()) {
        if (_editState != nullptr) {
            auto const& lineInfo = _getEmptyLine();
            maxTextHeight = std::max(maxTextHeight, (lineInfo.ascent + lineInfo.descent));
        }
    }

    auto const finalWidth = std::ceilf(_width.value_or(maxTextWidth + paddingLeft + paddingRight));
    auto const finalHeight = std::ceilf(_height.value_or(maxTextHeight + paddingTop + paddingBottom));
    auto const availWidth = (finalWidth - paddingLeft - paddingRight);
    auto const availHeight = (finalHeight - paddingTop - paddingBottom);

    for (auto i = 0ll; i < _lines.size(); i++) {
        auto& line = _lines[i];
        auto const remainder = std::floorf(availWidth - line.rect.width);
        auto const offset = paddingLeft + (
            _alignment == TextAlignment::Center ? std::max(0.0f, std::floorf(remainder / 2.0f)) :
            _alignment == TextAlignment::End ? std::max(0.0f, remainder) : 0.0f
        );

        if (offset != 0.0f) {
            line.rect.x += offset;
            for (auto i = line.startIndex; i < line.endIndex; i++) {
                auto& glyph = _glyphs[i];
                glyph.rect.x += offset;
                glyph.bitmapRect.x += offset;
            }
        }
    }

    {
        auto const remainder = std::floorf(availHeight - maxTextHeight);
        auto const offset = paddingTop + (
            _justify == TextJustify::Center ? std::max(0.0f, std::floorf(remainder / 2.0f)) :
            _justify == TextJustify::End ? std::max(0.0f, remainder) : 0.0f
        );

        if (offset != 0.0f) {
            for (auto& line : _lines) line.rect.y += offset;
            for (auto& glyph : _glyphs) {
                glyph.rect.y += offset;
                glyph.bitmapRect.y += offset;
            }
        }
    }

    _size = { finalWidth, finalHeight };
}

void Text::_updateImage() const {
    PROFILE

    if (_needsImage == false) {
        return;
    }

    _needsImage = false;

    auto const size = Vec2{ std::max(1.0f, _size.width), std::max(1.0f, _size.height) };

    if ((_image == nullptr) || (_image->getSize() != size)) {
        _image = std::make_unique<Image>(size);
    }

    auto& painter = _GetPainter();

    painter.beginPaint(
        ImagePaintTarget{ .image = *_image, .clearColor = COLOR_TRANSPARENT }
    );

    for (auto const& glyph : _glyphs) {
        if (glyph.marker.alpha > 0.0f) {
            painter.paint(QuadShape{ glyph.rect }, ColorBrush{ glyph.marker });
        }

        if (
            (glyph.whitespace == false) &&
            (glyph.color.alpha > 0)
        ) {
            painter.paint(QuadShape{ glyph.bitmapRect }, ImageBrush{
                .image = glyph.image,
                .slice = glyph.imageSlice,
                .color = glyph.color
            });
        }
    }

    painter.endPaint();
}

void Text::_updateEdit() const {
    PROFILE

    if (_editState == nullptr) {
        return;
    }

    if (_needsEdit == false) {
        return;
    }

    _needsEdit = false;

    auto const scale = getScale().value_or(1.0f);
    auto const caretWidth = (_TEXT_CARET_WIDTH * scale);
    auto const& glyphs = getGlyphs();
    auto const& lines = getLines();

    if (glyphs.empty()) {
        auto const& lineInfo = _getEmptyLine();

        _editState->caretRect = {};
        _editState->caretRect.x = _paddingLeft.value_or(0.0f);
        _editState->caretRect.y = _paddingTop.value_or(0.0f);
        _editState->caretRect.width = caretWidth;
        _editState->caretRect.height = (lineInfo.ascent + lineInfo.descent);
    } else {
        auto const caret = _editState->caret;
        auto const beforeTextEnd = (caret < (std::int64_t)glyphs.size());
        auto const hasPrevGlyph = ((caret > 0) && beforeTextEnd);
        auto const afterBreakline = (hasPrevGlyph && (glyphs[caret - 1].codepoint == U'\n'));
        auto const atLineEnd = (hasPrevGlyph && (caret == lines[glyphs[caret - 1].rowIndex].endIndex));
        auto const affinity = (
            _editState->caretAffinity &&
            atLineEnd &&
            (afterBreakline == false)
        );

        if (affinity) {
            _editState->caretRect = glyphs[caret - 1].rect;
            _editState->caretRect.x = _editState->caretRect.getMaxX();
            _editState->caretRect.width = caretWidth;
        } else if (beforeTextEnd) {
            _editState->caretRect = glyphs[caret].rect;
            _editState->caretRect.width = caretWidth;
        } else if (glyphs.back().codepoint == U'\n') {
            auto const& line = lines.back();

            _editState->caretRect = {};
            _editState->caretRect.origin = line.rect.origin;
            _editState->caretRect.width = caretWidth;
            _editState->caretRect.height = line.rect.height;
        } else {
            _editState->caretRect = glyphs.back().rect;
            _editState->caretRect.x = _editState->caretRect.getMaxX();
            _editState->caretRect.width = caretWidth;
        }
    }

    if (
        isSelectedRange() &&
        _getMaxIndex() > 0
    ) {
        _editState->selectionRects = {};

        if (lines.empty() == false) {
            auto lastGlyph = false;
            auto const fromIndex = std::clamp(std::min(_editState->caret, _editState->anchor), (std::int64_t)0, _getMaxIndex());
            auto toIndex = std::clamp(std::max(_editState->caret, _editState->anchor), (std::int64_t)0, _getMaxIndex());

            if (toIndex == _getMaxIndex()) {
                toIndex -= 1;
                lastGlyph = true;
            }

            auto const fromLineIndex = glyphs[fromIndex].rowIndex;
            auto const toLineIndex = glyphs[toIndex].rowIndex;

            for (auto i = fromLineIndex; i <= toLineIndex; i++) {
                if (
                    i == fromLineIndex &&
                    i == toLineIndex
                ) {
                    auto rect = Vec4{};
                    rect.origin = glyphs[fromIndex].rect.origin;
                    rect.size = {
                        glyphs[toIndex].rect.origin.x - rect.origin.x,
                        glyphs[toIndex].rect.height
                    };

                    if (lastGlyph) {
                        rect.size.width += glyphs[toIndex].rect.width;
                    }

                    _editState->selectionRects.push_back(rect);
                } else if (i == fromLineIndex) {
                    auto rect = Vec4{};
                    rect.origin = glyphs[fromIndex].rect.origin;
                    rect.size = {
                        lines[i].rect.getMaxX() - rect.origin.x,
                        lines[i].rect.height
                    };

                    _editState->selectionRects.push_back(rect);
                } else if (i == toLineIndex) {
                    auto rect = Vec4{};
                    rect.origin = lines[i].rect.origin;
                    rect.size = {
                        glyphs[toIndex].rect.origin.x - rect.origin.x,
                        glyphs[toIndex].rect.height
                    };

                    if (lastGlyph) {
                        rect.size.width += glyphs[toIndex].rect.width;
                    }

                    _editState->selectionRects.push_back(rect);
                } else {
                    _editState->selectionRects.push_back(lines[i].rect);
                }
            }

            if ((_editState->selectionRects.size() > 1) && (_editState->selectionRects.back().width == 0.0f)) {
                _editState->selectionRects.pop_back();
            }
        }
    } else {
        _editState->selectionRects = {};
    }
}

void Text::_invalidateData() {
    PROFILE

    _needsString = true;
    _invalidateIndex();
}

void Text::_invalidateIndex() {
    PROFILE

    _needsIndex = true;
    _invalidateLayout();
}

void Text::_invalidateLayout() {
    PROFILE

    _needsLayout = true;
    _needsImage = true;
    _needsEdit = true;
}

void Text::_invalidateEdit() {
    PROFILE

    _needsEdit = true;
}

void Text::_pushUndo(int kind, bool coalesce) {
    PROFILE

    auto& edit = *_editState;
    auto const continues = (
        coalesce &&
        (kind != _EDIT_NONE) &&
        (kind == edit.lastEditKind) &&
        (edit.caret == edit.lastEditCaret) &&
        (isSelectedRange() == false)
    );

    if (continues == false) {
        edit.undoStack.push_back(getSnapshot());
        if (edit.undoStack.size() > _TEXT_UNDO_LIMIT) {
            edit.undoStack.erase(edit.undoStack.begin());
        }
    }

    edit.redoStack.clear();
    edit.lastEditKind = kind;
}

void Text::_endEdit() {
    PROFILE

    _editState->lastEditCaret = _editState->caret;
}

void Text::_applySnapshot(TextEditSnapshot const& snapshot) {
    PROFILE

    _codepoints = snapshot.codepoints;
    _editState->caret = snapshot.caret;
    _editState->anchor = snapshot.anchor;
    _editState->caretXGoal = snapshot.caretXGoal;
    _setCaretAffinity(snapshot.caretAffinity);
    _invalidateData();
    _invalidateEdit();

    setStyles(snapshot.styles); /* change-gated, so identical styles don't invalidate */
}

void Text::_setCaret(std::int64_t caret) {
    PROFILE

    auto const value = std::clamp(caret, 0ll, _getMaxIndex());

    if (_editState->caret != value) {
        _editState->caret = value;
        _invalidateEdit();
    }
}

void Text::_setAnchor(std::int64_t anchor) {
    PROFILE

    auto const value = std::clamp(anchor, 0ll, _getMaxIndex());

    if (_editState->anchor != value) {
        _editState->anchor = value;
        _invalidateEdit();
    }
}

void Text::_setCaretAffinity(bool affinity) {
    PROFILE

    if (_editState->caretAffinity != affinity) {
        _editState->caretAffinity = affinity;
        _invalidateEdit();
    }
}

void Text::_removeSelection() {
    PROFILE

    auto const fromIndex = std::min(_editState->caret, _editState->anchor);
    auto const toIndex = std::max(_editState->caret, _editState->anchor);

    _eraseCodepoints(fromIndex, toIndex);

    _setCaret(fromIndex);
    _setAnchor(fromIndex);
}

void Text::_deleteSelection() {
    PROFILE

    _editState->caretXGoal = std::nullopt;
    _setCaretAffinity(false);

    _removeSelection();

    _invalidateData();
}

void Text::_insertCodepoints(std::int64_t index, std::vector<std::uint32_t> const& codepoints) {
    PROFILE

    _codepoints.insert(
        _codepoints.begin() + index,
        codepoints.begin(),
        codepoints.end()
    );

    auto const count = (std::int64_t)codepoints.size();
    auto const maxIndex = std::numeric_limits<std::int64_t>::max();
    auto changed = false;

    if (count == 0) {
        return;
    }

    for (auto& range : _ranges) {
        auto const empty = (range.startIndex >= range.endIndex);
        auto const shiftStart = (range.startIndex > index)
            || ((range.startIndex == index) && (index > 0));
        auto const shiftEnd = empty
            ? shiftStart
            : (range.endIndex >= index);

        if (shiftStart && (range.startIndex != maxIndex)) {
            range.startIndex += count;
            changed = true;
        }

        if (shiftEnd && (range.endIndex != maxIndex)) {
            range.endIndex += count;
            changed = true;
        }
    }

    if (changed) {
        _invalidateIndex();
    }
}

void Text::_eraseCodepoints(std::int64_t fromIndex, std::int64_t toIndex) {
    PROFILE

    _codepoints.erase(
        _codepoints.begin() + fromIndex,
        _codepoints.begin() + toIndex
    );

    auto const count = (toIndex - fromIndex);
    auto const maxIndex = std::numeric_limits<std::int64_t>::max();
    auto changed = false;

    if (count <= 0) {
        return;
    }

    auto const remap = [&](std::int64_t index) {
        if (index == maxIndex) {
            return index;
        }
        if (index >= toIndex) {
            return (index - count);
        }
        if (index > fromIndex) {
            return fromIndex;
        }
        return index;
    };

    for (auto& range : _ranges) {
        auto const startIndex = remap(range.startIndex);
        auto const endIndex = remap(range.endIndex);

        if ((startIndex != range.startIndex) || (endIndex != range.endIndex)) {
            range.startIndex = startIndex;
            range.endIndex = endIndex;
            changed = true;
        }
    }

    auto const removed = std::erase_if(_ranges, [](auto const& range) {
        return (range.startIndex >= range.endIndex);
    });

    if (changed || (removed > 0)) {
        _invalidateIndex();
    }
}

} /* namespace Rocket */
