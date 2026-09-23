/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Paint/Text.hpp>
#include <Rocket/Window/Clipboard.hpp>
#include <gtest/gtest.h>

using namespace Rocket;

/* End-to-end editing tests: input is synthesized as WindowEvents through the
   public window.onEvent source, exactly as the platform layer publishes it,
   and the outcome is observed through the node content, InputNodeEvents and
   the paint calls the document issues. The expectations are those of macOS
   text fields and browser <input> / <textarea> elements; a test that pins a
   known divergence is DISABLED_ (run with --gtest_also_run_disabled_tests)
   and its comment says what the reference does. */

/* -------------------------------------------------------------------------
 * Link-seam Painter: this TU defines Painter's public symbols so the real
 * Painter.cpp.o is never pulled from libRocket.a (see Document_Test.cpp).
 * Paint calls are recorded when a record vector is bound, else dropped.
 * ------------------------------------------------------------------------- */

struct _PaintRecord {
    std::optional<Vec4> rect;
    std::optional<Vec4> color;
    std::optional<Vec4> scissor;
    bool image;
};

static std::vector<_PaintRecord>* _record = nullptr;

Painter::Painter() : _impl(nullptr) {}
Painter::~Painter() {}
void Painter::beginPaint(PaintTarget const&) {}
void Painter::endPaint() {}

void Painter::paint(Shape const& shape, Brush const& brush, PaintOptions const& options) {
    if (_record != nullptr) {
        auto record = _PaintRecord{};
        if (auto quad = shape.as<QuadShape>()) record.rect = quad->rect;
        if (auto color = brush.as<ColorBrush>()) record.color = color->color;
        record.scissor = options.scissor;
        record.image = (brush.as<ImageBrush>() != nullptr);
        _record->push_back(record);
    }
}

/* ------------------------------- harness --------------------------------- */

static KeyModifiers _Mods(bool shift = false, bool meta = false, bool alt = false, bool control = false) {
    return KeyModifiers{ .control = control, .shift = shift, .meta = meta, .alt = alt };
}

/* An editable box with one text child on a hidden 640x480 window. */
struct _Editor {
    Window window;
    Document document;
    Node box;
    Node text;
    std::vector<std::string> inputs;
    std::vector<std::string> sequence;
    Sub<NodeEvent const&> sub;
    std::vector<_PaintRecord> paints;

    _Editor(std::string const& content = "", bool multiLine = false, Vec2 const& size = { 200.0f, 30.0f })
        : window(), document(window) {
        window.setSize({ 640.0f, 480.0f });

        box.setWidth(size.x);
        box.setHeight(size.y);
        box.setContentEditable(true);
        box.setContentMultiLine(multiLine);
        document.appendChild(box);

        text.setDisplay(NodeDisplay::Text);
        text.setContent(content);
        box.appendChild(text);

        sub.on(box.onEvent, [this](NodeEvent const& event) {
            if (auto input = event.as<InputNodeEvent>()) {
                inputs.push_back(input->getContent());
                sequence.push_back("input");
            } else if (event.is<KeyDownNodeEvent>()) {
                sequence.push_back("keydown");
            }
        });

        document.update();
    }

    void focus() {
        document.focusNode(&box);
        document.update();
    }

    std::string content() const {
        return text.getContent().value_or("");
    }

    void key(Rocket::Key key, KeyModifiers const& modifiers = {}, std::string const& input = "") {
        window.onEvent.publish(KeyDownWindowEvent(window, key, modifiers, input));
        window.onEvent.publish(KeyUpWindowEvent(window, key, modifiers));
        document.update();
    }

    /** Types ASCII text one key per character, as the platform does. */
    void type(std::string const& string) {
        for (auto ch : string) {
            key(Rocket::Key::Unknown, {}, std::string(1, ch));
        }
    }

    /** Types one multi-byte character as a single keypress. */
    void typeOne(std::string const& character) {
        key(Rocket::Key::Unknown, {}, character);
    }

    void move(Vec2 const& position, KeyModifiers const& modifiers = {}) {
        window.onEvent.publish(MouseMoveWindowEvent(window, position, modifiers));
        document.update();
    }

    void down(Vec2 const& position, KeyModifiers const& modifiers = {}, Mouse mouse = Mouse::LeftButton) {
        window.onEvent.publish(MouseMoveWindowEvent(window, position, modifiers));
        window.onEvent.publish(MouseDownWindowEvent(window, mouse, position, modifiers));
        document.update();
    }

    void up(Vec2 const& position, KeyModifiers const& modifiers = {}, Mouse mouse = Mouse::LeftButton) {
        window.onEvent.publish(MouseUpWindowEvent(window, mouse, position, modifiers));
        document.update();
    }

    void click(Vec2 const& position, KeyModifiers const& modifiers = {}, Mouse mouse = Mouse::LeftButton) {
        down(position, modifiers, mouse);
        up(position, modifiers, mouse);
    }

    void drag(Vec2 const& from, Vec2 const& to, KeyModifiers const& modifiers = {}) {
        down(from, modifiers);
        move(to, modifiers);
        up(to, modifiers);
    }

    /** Converts a node-space point to the window-space point that maps onto it. */
    Vec2 toWindow(Vec2 const& point) const {
        return point * (document.getScale() / window.getScale());
    }

    /**
     * A window-space point on the given text node's glyph `index` (left
     * quarter of its advance, so the caret lands at `index`), or just past
     * the last glyph for `index` == length. Computed with a probe Text laid
     * out like the node's own text.
     */
    Vec2 glyphPoint(std::int64_t index, Node const& textNode) const {
        auto const scale = document.getScale();
        auto const& rect = textNode.getComputedBorderRect();

        auto probe = Text();
        probe.setScale(scale);
        probe.setString(textNode.getContent().value_or(""));
        probe.setWidth(std::floorf(rect.width * scale));

        auto const& glyphs = probe.getGlyphs();
        auto local = Vec2{};
        if (glyphs.empty()) {
            local = { 1.0f, probe.getSize().height / 2.0f };
        } else if (index < (std::int64_t)glyphs.size()) {
            auto const& glyph = glyphs[(std::size_t)index];
            local = { glyph.rect.x + (glyph.rect.width * 0.25f), glyph.rect.y + (glyph.rect.height / 2.0f) };
        } else {
            auto const& glyph = glyphs.back();
            local = { glyph.rect.getMaxX() + 1.0f, glyph.rect.y + (glyph.rect.height / 2.0f) };
        }

        return toWindow(rect.origin + (local / scale));
    }

    Vec2 glyphPoint(std::int64_t index) const {
        return glyphPoint(index, text);
    }

    /** Paints the document, recording every paint call. */
    void render() {
        paints.clear();
        _record = &paints;
        document.update();
        document.render();
        _record = nullptr;
    }

    /** The last image paint: the text image (glyph rasterisation paints
        precede it). */
    _PaintRecord const* textPaint() const {
        auto const it = std::find_if(paints.rbegin(), paints.rend(), [](auto const& p) { return p.image; });
        return (it == paints.rend()) ? nullptr : &*it;
    }

    /** Solid quads painted after the text image: the caret. */
    std::vector<Vec4> caretRects() const {
        auto rects = std::vector<Vec4>();
        auto afterImage = false;
        for (auto const& paint : paints) {
            if (paint.image) afterImage = true;
            else if (afterImage && paint.rect.has_value() && paint.color.has_value()) rects.push_back(*paint.rect);
        }
        return rects;
    }

    /** Solid quads painted before the text image: the selection. */
    std::vector<Vec4> selectionRects() const {
        auto rects = std::vector<Vec4>();
        for (auto const& paint : paints) {
            if (paint.image) break;
            if (paint.rect.has_value() && paint.color.has_value()) rects.push_back(*paint.rect);
        }
        return rects;
    }
};

/* Runs body with the given clipboard text, restoring the user's clipboard after. */
static void _WithClipboard(std::string const& string, std::function<void()> const& body) {
    auto const original = GetClipboardString();
    SetClipboardString(string);
    body();
    SetClipboardString(original);
}

/* ============================ key bindings ================================ */

/* Cmd+Left / Cmd+Right go to the line edges. */
TEST(Editing, CmdArrowsMoveToLineEdges) {
    auto e = _Editor("one two three");
    e.focus();
    e.key(Rocket::Key::ArrowRight);
    e.key(Rocket::Key::ArrowRight);
    e.key(Rocket::Key::ArrowRight, _Mods(false, true));
    e.type("X");
    EXPECT_EQ(e.content(), "one two threeX");
    e.key(Rocket::Key::ArrowLeft, _Mods(false, true));
    e.type("Y");
    EXPECT_EQ(e.content(), "Yone two threeX");
}

/* Option+Left / Option+Right move by words. */
TEST(Editing, OptionArrowsMoveByWords) {
    auto e = _Editor("one two three");
    e.focus();
    e.key(Rocket::Key::ArrowRight, _Mods(false, false, true));
    e.type("X");
    EXPECT_EQ(e.content(), "oneX two three");
    e.key(Rocket::Key::ArrowRight, _Mods(false, false, true));
    e.type("X");
    EXPECT_EQ(e.content(), "oneX twoX three");
    e.key(Rocket::Key::ArrowLeft, _Mods(false, false, true));
    e.type("Y");
    EXPECT_EQ(e.content(), "oneX YtwoX three");
}

/* Home / End go to the line edges. */
TEST(Editing, HomeEndMoveToLineEdges) {
    auto e = _Editor("abc");
    e.focus();
    e.key(Rocket::Key::End);
    e.type("X");
    EXPECT_EQ(e.content(), "abcX");
    e.key(Rocket::Key::Home);
    e.type("Y");
    EXPECT_EQ(e.content(), "YabcX");
}

/* Cmd+Up / Cmd+Down go to the document edges in multi-line content. */
TEST(Editing, CmdVerticalArrowsMoveToDocumentEdges) {
    auto e = _Editor("ab\ncd", true, { 200.0f, 80.0f });
    e.focus();
    e.key(Rocket::Key::ArrowDown, _Mods(false, true));
    e.type("X");
    EXPECT_EQ(e.content(), "ab\ncdX");
    e.key(Rocket::Key::ArrowUp, _Mods(false, true));
    e.type("Y");
    EXPECT_EQ(e.content(), "Yab\ncdX");
}

/* Shift with any movement extends the selection, and typing replaces it. */
TEST(Editing, ShiftMovesSelectAndTypingReplaces) {
    auto e = _Editor("one two three");
    e.focus();
    e.key(Rocket::Key::End);
    e.key(Rocket::Key::ArrowLeft, _Mods(true, false, true));
    e.type("3");
    EXPECT_EQ(e.content(), "one two 3");
    e.key(Rocket::Key::ArrowLeft, _Mods(true, true));
    e.type("z");
    EXPECT_EQ(e.content(), "z");
    e.key(Rocket::Key::ArrowLeft, _Mods(true));
    e.key(Rocket::Key::Backspace);
    EXPECT_EQ(e.content(), "");
}

/* Backspace and Delete with a selection remove exactly the selection. */
TEST(Editing, BackspaceAndDeleteRemoveSelection) {
    auto e = _Editor("abcd");
    e.focus();
    e.key(Rocket::Key::ArrowRight, _Mods(true));
    e.key(Rocket::Key::ArrowRight, _Mods(true));
    e.key(Rocket::Key::Backspace);
    EXPECT_EQ(e.content(), "cd");
    e.key(Rocket::Key::End);
    e.key(Rocket::Key::ArrowLeft, _Mods(true));
    e.key(Rocket::Key::Delete);
    EXPECT_EQ(e.content(), "c");
}

/* Option+Backspace deletes the word before the caret, Option+Delete the word after. */
TEST(Editing, OptionBackspaceAndDeleteRemoveWords) {
    auto e = _Editor("one two three");
    e.focus();
    e.key(Rocket::Key::End);
    e.key(Rocket::Key::Backspace, _Mods(false, false, true));
    EXPECT_EQ(e.content(), "one two ");
    e.key(Rocket::Key::Home);
    e.key(Rocket::Key::Delete, _Mods(false, false, true));
    EXPECT_EQ(e.content(), " two ");
}

/* Cmd+A selects everything; typing then replaces the whole content. */
TEST(Editing, CmdASelectsAllAndTypingReplaces) {
    auto e = _Editor("hello");
    e.focus();
    e.key(Rocket::Key::KeyA, _Mods(false, true), "a");
    e.type("x");
    EXPECT_EQ(e.content(), "x");
}

/* Keys that are not editing commands change nothing and fire no input. */
TEST(Editing, NonEditingKeysAreNoOps) {
    auto e = _Editor("hello");
    e.focus();
    e.key(Rocket::Key::Escape);
    e.key(Rocket::Key::PageUp);
    e.key(Rocket::Key::PageDown);
    e.key(Rocket::Key::F1);
    e.key(Rocket::Key::Shift);
    e.key(Rocket::Key::Meta);
    e.key(Rocket::Key::Capslock);
    e.key(Rocket::Key::ArrowLeft);
    e.key(Rocket::Key::ArrowRight);
    e.key(Rocket::Key::KeyS, _Mods(false, true), "s");   /* Cmd+S */
    e.key(Rocket::Key::KeyB, _Mods(false, true), "b");   /* Cmd+B */
    e.key(Rocket::Key::KeyG, _Mods(false, false, false, true), "g"); /* Ctrl+G */
    e.key(Rocket::Key::Tab);
    e.key(Rocket::Key::Enter);
    EXPECT_EQ(e.content(), "hello");
    EXPECT_TRUE(e.inputs.empty());
}

/* Option+letter yields a composed character on macOS keyboards (Option+A
   is "å"); it is inserted like any other text. */
TEST(Editing, OptionCharacterIsInserted) {
    auto e = _Editor("");
    e.focus();
    e.key(Rocket::Key::KeyA, _Mods(false, false, true), "\xC3\xA5");
    EXPECT_EQ(e.content(), "\xC3\xA5");
}

/* Multi-codepoint characters are inserted whole and deleted whole. */
TEST(Editing, UnicodeTypingAndDeletion) {
    auto const family = std::string("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7");
    auto e = _Editor("");
    e.focus();
    e.typeOne("\xC3\xA9");
    e.typeOne(family);
    e.typeOne("\xE6\xB1\x89");
    EXPECT_EQ(e.content(), "\xC3\xA9" + family + "\xE6\xB1\x89");
    e.key(Rocket::Key::Backspace);
    e.key(Rocket::Key::Backspace);
    EXPECT_EQ(e.content(), "\xC3\xA9");
    e.key(Rocket::Key::Backspace);
    EXPECT_EQ(e.content(), "");
}

/* ============================ input events =============================== */

/* InputNodeEvent carries the full content and fires once per change, never
   for pure caret movement or for edits that change nothing. */
TEST(Editing, InputEventFiresOncePerChangeWithFullContent) {
    auto e = _Editor("");
    e.focus();
    e.key(Rocket::Key::Backspace);
    e.key(Rocket::Key::Delete);
    EXPECT_TRUE(e.inputs.empty());

    e.type("ab");
    EXPECT_EQ(e.inputs, (std::vector<std::string>{ "a", "ab" }));

    e.key(Rocket::Key::ArrowLeft);
    e.key(Rocket::Key::End);
    e.key(Rocket::Key::KeyA, _Mods(false, true), "a");
    e.key(Rocket::Key::ArrowRight);
    e.key(Rocket::Key::Delete);
    EXPECT_EQ(e.inputs.size(), 2u);

    e.key(Rocket::Key::Backspace);
    EXPECT_EQ(e.inputs, (std::vector<std::string>{ "a", "ab", "a" }));
}

/* The KeyDownNodeEvent is delivered before the InputNodeEvent it causes. */
TEST(Editing, KeyDownPrecedesInputEvent) {
    auto e = _Editor("");
    e.focus();
    e.type("a");
    EXPECT_EQ(e.sequence, (std::vector<std::string>{ "keydown", "input" }));
}

/* InputNodeEvent bubbles from the editable box to its ancestors. */
TEST(Editing, InputEventBubblesToAncestors) {
    auto e = _Editor("");
    auto seen = std::vector<std::string>();
    auto sub = Sub<NodeEvent const&>();
    sub.on(e.document.onEvent, [&](NodeEvent const& event) {
        if (auto input = event.as<InputNodeEvent>()) seen.push_back(input->getContent());
    });
    e.focus();
    e.type("hi");
    EXPECT_EQ(seen, (std::vector<std::string>{ "h", "hi" }));
}

/* ============================ clipboard ================================== */

/* Cut, copy and paste round-trip Unicode through the system clipboard and
   report each content change. */
TEST(Editing, CutCopyPasteRoundTripUnicode) {
    _WithClipboard("sentinel", []{
        auto const content = std::string("a\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9" "b");
        auto e = _Editor(content);
        e.focus();
        e.key(Rocket::Key::KeyA, _Mods(false, true), "a");
        e.key(Rocket::Key::KeyC, _Mods(false, true), "c");
        EXPECT_EQ(GetClipboardString(), content);
        EXPECT_EQ(e.content(), content);
        EXPECT_TRUE(e.inputs.empty());

        e.key(Rocket::Key::KeyX, _Mods(false, true), "x");
        EXPECT_EQ(e.content(), "");
        EXPECT_EQ(e.inputs, (std::vector<std::string>{ "" }));

        e.key(Rocket::Key::KeyV, _Mods(false, true), "v");
        e.key(Rocket::Key::KeyV, _Mods(false, true), "v");
        EXPECT_EQ(e.content(), content + content);
        EXPECT_EQ(e.inputs.size(), 3u);
    });
}

/* Copy with nothing selected leaves the clipboard alone; paste of an empty
   clipboard changes nothing and fires no input. */
TEST(Editing, EmptyCopyAndPasteAreNoOps) {
    _WithClipboard("keep", []{
        auto e = _Editor("hello");
        e.focus();
        e.key(Rocket::Key::KeyC, _Mods(false, true), "c");
        EXPECT_EQ(GetClipboardString(), "keep");
        e.key(Rocket::Key::KeyX, _Mods(false, true), "x");
        EXPECT_EQ(GetClipboardString(), "keep");
        EXPECT_EQ(e.content(), "hello");

        SetClipboardString("");
        e.key(Rocket::Key::KeyV, _Mods(false, true), "v");
        EXPECT_EQ(e.content(), "hello");
        EXPECT_TRUE(e.inputs.empty());
    });
}

/* Paste replaces the selection. */
TEST(Editing, PasteReplacesSelection) {
    _WithClipboard("NEW", []{
        auto e = _Editor("one two three");
        e.focus();
        e.key(Rocket::Key::ArrowRight, _Mods(true, false, true));
        e.key(Rocket::Key::KeyV, _Mods(false, true), "v");
        EXPECT_EQ(e.content(), "NEW two three");
    });
}

/* A secure (password) field never exports its content: Cmd+C and Cmd+X
   leave the clipboard and the content alone, while paste still works. */
TEST(Editing, SecureFieldBlocksCopyAndCutAllowsPaste) {
    _WithClipboard("sentinel", []{
        auto e = _Editor("hunter2");
        e.box.setContentSecure(true);
        e.focus();
        e.key(Rocket::Key::KeyA, _Mods(false, true), "a");
        e.key(Rocket::Key::KeyC, _Mods(false, true), "c");
        EXPECT_EQ(GetClipboardString(), "sentinel");
        e.key(Rocket::Key::KeyX, _Mods(false, true), "x");
        EXPECT_EQ(GetClipboardString(), "sentinel");
        EXPECT_EQ(e.content(), "hunter2");
        e.key(Rocket::Key::KeyV, _Mods(false, true), "v");
        EXPECT_EQ(e.content(), "sentinel");
    });
}

/* The secure flag is honoured on either node: set on the box it masks the
   rendered text (bullets measure wider than a run of "i"), and set on the
   text node it blocks copy just like on the box. */
TEST(Editing, SecureFlagOnEitherNode) {
    {
        auto e = _Editor("iiii");
        auto const plainWidth = e.text.getComputedBorderRect().width;
        e.box.setContentSecure(true);
        e.document.update();
        EXPECT_GT(e.text.getComputedBorderRect().width, plainWidth);
    }

    _WithClipboard("sentinel", []{
        auto e = _Editor("hunter2");
        e.text.setContentSecure(true);
        e.focus();
        e.key(Rocket::Key::KeyA, _Mods(false, true), "a");
        e.key(Rocket::Key::KeyC, _Mods(false, true), "c");
        EXPECT_EQ(GetClipboardString(), "sentinel");
    });
}

/* ============================ mouse ====================================== */

/* A click places the caret at the glyph under the pointer. */
TEST(Editing, ClickPlacesCaretAtGlyph) {
    auto e = _Editor("hello");
    e.click(e.glyphPoint(2));
    ASSERT_TRUE(e.box.isFocused());
    e.type("X");
    EXPECT_EQ(e.content(), "heXllo");
}

/* A click in the empty area right of the text puts the caret at the end. */
TEST(Editing, ClickRightOfTextPlacesCaretAtEnd) {
    auto e = _Editor("hello");
    auto const& rect = e.box.getComputedBorderRect();
    e.click(e.toWindow({ rect.getMaxX() - 2.0f, rect.y + (rect.height / 2.0f) }));
    e.type("X");
    EXPECT_EQ(e.content(), "helloX");
}

/* A click in the box's padding left of the text puts the caret at the start. */
TEST(Editing, ClickInLeftPaddingPlacesCaretAtStart) {
    auto e = _Editor("hello");
    e.box.setPaddingLeft(30.0f);
    e.document.update();
    auto const& rect = e.box.getComputedBorderRect();
    e.click(e.toWindow({ rect.x + 3.0f, rect.y + (rect.height / 2.0f) }));
    e.type("X");
    EXPECT_EQ(e.content(), "Xhello");
}

/* Shift+click extends the selection from the caret; typing replaces it. */
TEST(Editing, ShiftClickExtendsSelection) {
    auto e = _Editor("hello");
    e.click(e.glyphPoint(1));
    e.click(e.glyphPoint(4), _Mods(true));
    e.type("X");
    EXPECT_EQ(e.content(), "hXo");
}

/* A drag selects the range it sweeps; typing replaces it. */
TEST(Editing, DragSelectsAndTypingReplaces) {
    auto e = _Editor("hello");
    e.drag(e.glyphPoint(1), e.glyphPoint(4));
    e.type("X");
    EXPECT_EQ(e.content(), "hXo");
}

/* Dragging out of the box keeps selecting up to the end of the text. */
TEST(Editing, DragOutsideBoxSelectsToEnd) {
    auto e = _Editor("hello");
    e.drag(e.glyphPoint(1), { 630.0f, 470.0f });
    e.type("X");
    EXPECT_EQ(e.content(), "hX");
}

/* A right click leaves the caret and selection where they are. */
TEST(Editing, RightClickDoesNotMoveCaret) {
    auto e = _Editor("hello");
    e.click(e.glyphPoint(2));
    e.click(e.glyphPoint(4), {}, Mouse::RightButton);
    e.type("X");
    EXPECT_EQ(e.content(), "heXllo");
}

/* Clicking from one editable into another moves focus and typing with it. */
TEST(Editing, ClickSwitchesBetweenEditables) {
    auto e = _Editor("first");
    auto other = Node();
    other.setWidth(200.0f);
    other.setHeight(30.0f);
    other.setContentEditable(true);
    e.document.appendChild(other);
    auto otherText = Node();
    otherText.setDisplay(NodeDisplay::Text);
    otherText.setContent("second");
    other.appendChild(otherText);
    e.document.update();

    e.click(e.glyphPoint(5));
    e.type("A");
    e.click(e.glyphPoint(6, otherText));
    ASSERT_TRUE(other.isFocused());
    ASSERT_FALSE(e.box.isFocused());
    e.type("B");
    EXPECT_EQ(e.content(), "firstA");
    EXPECT_EQ(otherText.getContent().value_or(""), "secondB");
}

/* With a document scale that differs from the window's, clicks still map
   onto the right glyph. */
TEST(Editing, ClickMapsUnderDocumentScale) {
    auto e = _Editor("hello");
    e.document.setScale(e.window.getScale() * 1.5f);
    e.document.update();
    e.click(e.glyphPoint(3));
    e.type("X");
    EXPECT_EQ(e.content(), "helXlo");
}

/* An editable inside a scrolled container maps clicks through the scroll offset. */
TEST(Editing, ClickMapsInsideScrolledContainer) {
    auto window = Window();
    window.setSize({ 640.0f, 480.0f });
    auto document = Document(window);

    auto scroller = Node();
    scroller.setWidth(300.0f);
    scroller.setHeight(60.0f);
    scroller.setDirection(NodeDirection::Vertical);
    scroller.setOverflowY(NodeOverflow::Scroll);
    document.appendChild(scroller);

    auto spacer = Node();
    spacer.setHeight(100.0f);
    scroller.appendChild(spacer);

    auto box = Node();
    box.setWidth(200.0f);
    box.setHeight(30.0f);
    box.setContentEditable(true);
    scroller.appendChild(box);

    auto text = Node();
    text.setDisplay(NodeDisplay::Text);
    text.setContent("hello");
    box.appendChild(text);

    document.update();
    document.scrollNode(box, { 0.0f, -100.0f });
    document.update();
    ASSERT_LT(box.getComputedBorderRect().y, 60.0f);

    auto probe = Text();
    probe.setScale(document.getScale());
    probe.setString("hello");
    auto const& glyph = probe.getGlyphs()[2];
    auto const& rect = text.getComputedBorderRect();
    auto const point = Vec2{
        rect.x + ((glyph.rect.x + (glyph.rect.width * 0.25f)) / document.getScale()),
        rect.y + (rect.height / 2.0f)
    } * (document.getScale() / window.getScale());

    window.onEvent.publish(MouseMoveWindowEvent(window, point, {}));
    window.onEvent.publish(MouseDownWindowEvent(window, Mouse::LeftButton, point, {}));
    window.onEvent.publish(MouseUpWindowEvent(window, Mouse::LeftButton, point, {}));
    ASSERT_TRUE(box.isFocused());
    window.onEvent.publish(KeyDownWindowEvent(window, Rocket::Key::Unknown, {}, "X"));
    EXPECT_EQ(text.getContent().value_or(""), "heXllo");
}

/* ============================ painting =================================== */

/* Focusing paints a caret; typing moves it right; selecting all paints the
   selection and no caret; blurring paints neither. */
TEST(Editing, CaretAndSelectionPaintLifecycle) {
    auto e = _Editor("");
    e.render();
    EXPECT_TRUE(e.caretRects().empty());

    e.focus();
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    auto const caretAtStart = e.caretRects()[0];
    EXPECT_GT(caretAtStart.height, 0.0f);

    e.type("abc");
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    EXPECT_GT(e.caretRects()[0].x, caretAtStart.x);
    EXPECT_TRUE(e.selectionRects().empty());

    e.key(Rocket::Key::KeyA, _Mods(false, true), "a");
    e.render();
    EXPECT_TRUE(e.caretRects().empty());
    EXPECT_EQ(e.selectionRects().size(), 1u);

    e.document.focusNode(nullptr);
    e.render();
    EXPECT_TRUE(e.caretRects().empty());
    EXPECT_TRUE(e.selectionRects().empty());

    e.type("z");
    EXPECT_EQ(e.content(), "abc");
}

/* The caret is painted inside the box it belongs to. */
TEST(Editing, CaretPaintsInsideBox) {
    auto e = _Editor("hello");
    e.focus();
    e.key(Rocket::Key::End);
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    auto const caret = e.caretRects()[0];
    auto const box = e.box.getComputedBorderRect() * e.document.getScale();
    EXPECT_GE(caret.x, box.x);
    EXPECT_LE(caret.getMaxX(), box.getMaxX());
    EXPECT_GE(caret.y, box.y);
    EXPECT_LE(caret.getMaxY(), box.getMaxY());
}

/* ============================ lifecycle ================================== */

/* Removing the focused editable from the tree while keys keep arriving must
   not crash or edit anything. */
TEST(Editing, RemovingFocusedEditableThenKeysIsSafe) {
    auto e = _Editor("hello");
    e.focus();
    e.document.removeChild(e.box);
    e.type("X");
    e.key(Rocket::Key::KeyA, _Mods(false, true), "a");
    e.key(Rocket::Key::Backspace);
    EXPECT_EQ(e.content(), "hello");
    EXPECT_TRUE(e.inputs.empty());
    EXPECT_FALSE(e.box.isFocused());
}

/* Replacing the text child of a focused editable (as a reconciler does when
   a key changes) keeps the box editable once layout has run: typing edits
   the new child. */
TEST(Editing, ReplacingTextChildWhileFocusedKeepsEditingAfterUpdate) {
    auto e = _Editor("old");
    e.focus();

    auto replacement = Node();
    replacement.setDisplay(NodeDisplay::Text);
    replacement.setContent("new");
    e.box.removeChild(e.text);
    e.box.appendChild(replacement);
    e.document.update();

    e.type("Y");
    EXPECT_TRUE(e.box.isFocused());
    EXPECT_NE(replacement.getContent().value_or("").find('Y'), std::string::npos);
}

/* A key arriving between the replacement of the text child and the next
   layout edits the new child instead of crashing. */
TEST(Editing, ReplacingTextChildThenKeyBeforeUpdateEditsNewChild) {
    auto e = _Editor("old");
    e.focus();

    auto replacement = Node();
    replacement.setDisplay(NodeDisplay::Text);
    replacement.setContent("new");
    e.box.removeChild(e.text);
    e.box.appendChild(replacement);

    e.window.onEvent.publish(KeyDownWindowEvent(e.window, Rocket::Key::Unknown, {}, "X"));
    e.document.update();
    EXPECT_TRUE(e.box.isFocused());
    EXPECT_EQ(replacement.getContent().value_or(""), "Xnew");
}

/* Turning editing off on a focused box stops keys from editing; turning it
   back on resumes. */
TEST(Editing, TogglingEditableWhileFocusedStopsAndResumesEditing) {
    auto e = _Editor("hello");
    e.focus();
    e.box.setContentEditable(false);
    e.document.update();
    e.type("X");
    EXPECT_EQ(e.content(), "hello");
    EXPECT_TRUE(e.inputs.empty());

    e.box.setContentEditable(true);
    e.document.update();
    e.type("X");
    EXPECT_NE(e.content().find('X'), std::string::npos);
}

/* Programmatic content changes while focused are picked up by editing. */
TEST(Editing, ProgrammaticContentChangeIsEditable) {
    auto e = _Editor("hello");
    e.focus();
    e.text.setContent("world");
    e.document.update();
    e.key(Rocket::Key::End);
    e.type("X");
    EXPECT_EQ(e.content(), "worldX");
}

/* Re-focusing an editable programmatically after blur makes it editable again. */
TEST(Editing, RefocusAfterBlurEditsAgain) {
    auto e = _Editor("hello");
    e.focus();
    e.document.focusNode(nullptr);
    e.document.update();
    e.focus();
    e.key(Rocket::Key::End);
    e.type("X");
    EXPECT_EQ(e.content(), "helloX");
}

/* Multi-line: Enter inserts a line break, ArrowDown moves to the next line
   and Tab indents. */
TEST(Editing, MultiLineEnterArrowDownAndTab) {
    auto e = _Editor("ab\ncd", true, { 200.0f, 80.0f });
    e.focus();
    e.key(Rocket::Key::ArrowDown);
    e.type("X");
    EXPECT_EQ(e.content(), "ab\nXcd");
    e.key(Rocket::Key::End);
    e.key(Rocket::Key::Enter);
    e.key(Rocket::Key::Tab);
    e.type("Y");
    EXPECT_EQ(e.content(), "ab\nXcd\n    Y");
}

/* ============================ known gaps ================================= */

/* Cmd+Backspace deletes from the caret to the start of the line, Cmd+Delete
   to its end. */
TEST(Editing, CmdBackspaceAndCmdDeleteDeleteToLineEdges) {
    auto e = _Editor("one two");
    e.focus();
    e.key(Rocket::Key::End);
    e.key(Rocket::Key::Backspace, _Mods(false, true));
    EXPECT_EQ(e.content(), "");
    e.type("one two");
    e.key(Rocket::Key::ArrowLeft, _Mods(false, false, true));
    e.key(Rocket::Key::Delete, _Mods(false, true));
    EXPECT_EQ(e.content(), "one ");
}

/* Every Cocoa text view (and Chrome on macOS) honours the Emacs bindings
   Ctrl+A / Ctrl+E (line start / end), Ctrl+F / Ctrl+B (char), Ctrl+D
   (delete forward), Ctrl+H (delete backward), Ctrl+K (kill to line end). */
TEST(Editing, ControlEmacsBindings) {
    auto e = _Editor("abc");
    e.focus();
    e.key(Rocket::Key::KeyE, _Mods(false, false, false, true));
    e.type("X");
    EXPECT_EQ(e.content(), "abcX");
    e.key(Rocket::Key::KeyA, _Mods(false, false, false, true));
    e.type("Y");
    EXPECT_EQ(e.content(), "YabcX");
    e.key(Rocket::Key::KeyF, _Mods(false, false, false, true));
    e.key(Rocket::Key::KeyD, _Mods(false, false, false, true));
    EXPECT_EQ(e.content(), "YacX");
    e.key(Rocket::Key::KeyB, _Mods(false, false, false, true));
    e.key(Rocket::Key::KeyF, _Mods(false, false, false, true));
    e.key(Rocket::Key::KeyH, _Mods(false, false, false, true));
    EXPECT_EQ(e.content(), "YcX");
    e.key(Rocket::Key::KeyK, _Mods(false, false, false, true));
    EXPECT_EQ(e.content(), "Y");
}

/* Cmd+Z undoes the last edit group and Cmd+Shift+Z redoes it, each
   reporting the content change. */
TEST(Editing, CmdZUndoesAndCmdShiftZRedoes) {
    auto e = _Editor("");
    e.focus();
    e.type("abc def");
    e.key(Rocket::Key::KeyZ, _Mods(false, true), "z");
    EXPECT_EQ(e.content(), "abc");
    e.key(Rocket::Key::KeyZ, _Mods(false, true), "z");
    EXPECT_EQ(e.content(), "");
    e.key(Rocket::Key::KeyZ, _Mods(true, true), "z");
    EXPECT_EQ(e.content(), "abc");
    e.key(Rocket::Key::KeyZ, _Mods(true, true), "z");
    EXPECT_EQ(e.content(), "abc def");
    EXPECT_EQ(e.inputs.back(), "abc def");
    EXPECT_EQ(e.inputs.size(), 11u);
}

/* Double-click selects the word under the pointer, triple-click the line
   (Chrome: "one two three" double-click -> [0,3), triple-click -> [0,13)).
   The click count travels on MouseDownWindowEvent and MouseDownNodeEvent. */
TEST(Editing, DoubleClickSelectsWordTripleClickSelectsLine) {
    auto e = _Editor("one two");
    auto counts = std::vector<int>();
    auto sub = Sub<NodeEvent const&>();
    sub.on(e.box.onEvent, [&](NodeEvent const& event) {
        if (auto down = event.as<MouseDownNodeEvent>()) counts.push_back(down->getClickCount());
    });
    e.window.onEvent.publish(MouseMoveWindowEvent(e.window, e.glyphPoint(1), {}));
    e.window.onEvent.publish(MouseDownWindowEvent(e.window, Mouse::LeftButton, e.glyphPoint(1), {}, 1));
    e.window.onEvent.publish(MouseUpWindowEvent(e.window, Mouse::LeftButton, e.glyphPoint(1), {}));
    e.window.onEvent.publish(MouseDownWindowEvent(e.window, Mouse::LeftButton, e.glyphPoint(1), {}, 2));
    e.window.onEvent.publish(MouseUpWindowEvent(e.window, Mouse::LeftButton, e.glyphPoint(1), {}));
    e.type("X");
    EXPECT_EQ(e.content(), "X two");
    EXPECT_EQ(counts, (std::vector<int>{ 1, 2 }));

    auto f = _Editor("one two");
    f.window.onEvent.publish(MouseMoveWindowEvent(f.window, f.glyphPoint(5), {}));
    f.window.onEvent.publish(MouseDownWindowEvent(f.window, Mouse::LeftButton, f.glyphPoint(5), {}, 3));
    f.window.onEvent.publish(MouseUpWindowEvent(f.window, Mouse::LeftButton, f.glyphPoint(5), {}));
    f.type("X");
    EXPECT_EQ(f.content(), "X");
}

/* A KeyDownNodeEvent listener that calls preventDefault() keeps the key from
   editing (how a max-length or numeric-only field is built) and from moving
   focus on Tab. */
TEST(Editing, PreventDefaultOnKeyDownBlocksEditAndTab) {
    auto e = _Editor("");
    auto sub = Sub<NodeEvent const&>();
    sub.on(e.box.onEvent, [](NodeEvent const& event) {
        if (auto keyDown = event.as<KeyDownNodeEvent>()) {
            if ((keyDown->getInput() == "b") || (keyDown->getKey() == Rocket::Key::Tab)) event.preventDefault();
        }
    });
    e.focus();
    e.type("abc");
    EXPECT_EQ(e.content(), "ac");
    e.key(Rocket::Key::Tab);
    EXPECT_TRUE(e.box.isFocused());
}

/* Tab moves focus through focusable and editable nodes in tree order,
   Shift+Tab backwards, wrapping at both ends; an editable reached by Tab
   has its content selected so typing replaces it. */
TEST(Editing, TabTraversalCyclesFocusablesAndSelectsAll) {
    auto e = _Editor("first");
    auto button = Node();
    button.setWidth(40.0f);
    button.setHeight(20.0f);
    button.setTabIndex(1);
    e.document.appendChild(button);
    auto other = Node();
    other.setWidth(200.0f);
    other.setHeight(30.0f);
    other.setContentEditable(true);
    e.document.appendChild(other);
    auto otherText = Node();
    otherText.setDisplay(NodeDisplay::Text);
    otherText.setContent("second");
    other.appendChild(otherText);
    e.document.update();

    e.key(Rocket::Key::Tab);
    ASSERT_TRUE(e.box.isFocused());
    e.key(Rocket::Key::Tab);
    ASSERT_TRUE(button.isFocused());
    e.key(Rocket::Key::Tab);
    ASSERT_TRUE(other.isFocused());
    e.type("X");
    EXPECT_EQ(otherText.getContent().value_or(""), "X");
    e.key(Rocket::Key::Tab);
    ASSERT_TRUE(e.box.isFocused());
    e.key(Rocket::Key::Tab, _Mods(true));
    ASSERT_TRUE(other.isFocused());
    e.key(Rocket::Key::Tab, _Mods(true));
    ASSERT_TRUE(button.isFocused());
    e.key(Rocket::Key::Tab, _Mods(true));
    ASSERT_TRUE(e.box.isFocused());
    EXPECT_EQ(e.content(), "first");
}

/* The caret blinks: it is painted right after activity, hidden a little
   over half a second later, and painted again as soon as a key arrives. */
TEST(Editing, CaretBlinksWhileIdle) {
    auto e = _Editor("abc");
    e.focus();
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    e.render();
    EXPECT_TRUE(e.caretRects().empty());
    e.key(Rocket::Key::End);
    e.render();
    EXPECT_EQ(e.caretRects().size(), 1u);
}

/* A single-line field never wraps: content wider than the field stays on
   one line. */
TEST(Editing, LongSingleLineValueDoesNotWrap) {
    auto e = _Editor("short", false, { 100.0f, 30.0f });
    auto const oneLineHeight = e.text.getComputedBorderRect().height;
    e.focus();
    e.key(Rocket::Key::End);
    e.type(" and now a value that is much wider than the field");
    e.document.update();
    EXPECT_EQ(e.text.getComputedBorderRect().height, oneLineHeight);
}

/* While typing past the right edge of a single-line field the caret stays
   inside the field: the text scrolls horizontally, mouse hits map through
   the scroll, Home scrolls back to the start, and blurring resets the
   scroll. */
TEST(Editing, CaretStaysVisibleWhenSingleLineOverflows) {
    auto e = _Editor("", false, { 100.0f, 30.0f });
    e.focus();
    e.type("a value that is much wider than the field it lives in");
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    auto const caret = e.caretRects()[0];
    auto const box = e.box.getComputedBorderRect() * e.document.getScale();
    EXPECT_GE(caret.x, box.x);
    EXPECT_LE(caret.getMaxX(), box.getMaxX());
    EXPECT_GE(caret.y, box.y);
    EXPECT_LE(caret.getMaxY(), box.getMaxY());

    /* a click at the field's left edge lands on the scrolled-in text, not at index 0 */
    auto const rect = e.box.getComputedBorderRect();
    e.click(e.toWindow({ rect.x + 2.0f, rect.y + (rect.height / 2.0f) }));
    e.type("X");
    EXPECT_NE(e.content().find('X'), std::string::npos);
    EXPECT_NE(e.content()[0], 'X');

    e.key(Rocket::Key::Home);
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    EXPECT_GE(e.caretRects()[0].x, box.x);
    EXPECT_LE(e.caretRects()[0].getMaxX(), box.getMaxX());
    EXPECT_LE(e.caretRects()[0].x, box.x + (4.0f * e.document.getScale()));
}

/* A single-line field clips its text to the box whether or not it is
   focused: a long value never spills past the frame, and blurring shows the
   start of the value again. */
TEST(Editing, LongSingleLineValueIsClippedWhenUnfocused) {
    auto e = _Editor("a value that is much wider than the field it lives in", false, { 100.0f, 30.0f });
    e.render();
    auto const box = e.box.getComputedBorderRect() * e.document.getScale();
    ASSERT_TRUE(e.textPaint() != nullptr && e.textPaint()->scissor.has_value());
    EXPECT_GE(e.textPaint()->scissor->x, box.x);
    EXPECT_LE(e.textPaint()->scissor->getMaxX(), box.getMaxX());
    EXPECT_GT(e.text.getComputedBorderRect().width, e.box.getComputedBorderRect().width);

    e.focus();
    e.key(Rocket::Key::End);
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    EXPECT_LE(e.caretRects()[0].getMaxX(), box.getMaxX());

    e.document.focusNode(nullptr);
    e.render();
    EXPECT_TRUE(e.caretRects().empty());
    ASSERT_TRUE(e.textPaint() != nullptr && e.textPaint()->scissor.has_value() && e.textPaint()->rect.has_value());
    EXPECT_LE(e.textPaint()->scissor->getMaxX(), box.getMaxX());
    EXPECT_GE(e.textPaint()->rect->x, box.x);
}

/* A controlled field that rejects an edit (max length, validation) writes
   the previous value back; the caret must stay at the end so the user can
   keep typing there. */
TEST(Editing, ControlledRejectionKeepsCaretAtEnd) {
    auto e = _Editor("abc");
    e.focus();
    e.key(Rocket::Key::End);
    e.type("d");
    ASSERT_EQ(e.content(), "abcd");
    e.text.setContent("abc"); /* the owner rejects the fourth character */
    e.document.update();
    e.type("e");
    EXPECT_EQ(e.content(), "abce");
}

/* Disabling editing on a focused field also hides its caret. */
TEST(Editing, DisablingEditableWhileFocusedHidesCaret) {
    auto e = _Editor("hello");
    e.focus();
    e.render();
    ASSERT_EQ(e.caretRects().size(), 1u);
    e.box.setContentEditable(false);
    e.render();
    EXPECT_TRUE(e.caretRects().empty());
}
