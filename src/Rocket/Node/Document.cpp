/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */


#include <cassert>
#include <memory>
#include <cmath>
#include <algorithm>
#include <optional>
#include <yoga/Yoga.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/Paint/Text.hpp>
#include <Rocket/Window/Clipboard.hpp>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/Node.hpp>

namespace Rocket {

static constexpr auto _selectionColor = Vec4{ 0.4f, 0.6f, 1.0f, 0.4f };
static constexpr auto _caretBlinkPeriod = std::chrono::milliseconds(530);

static float _SnapToPixelGrid(float value, float scale) {
    return (std::roundf(value * scale) / scale);
}

static float _SnapBorderToPixelGrid(float value, float scale) {
    return (value > 0.0f) ? std::max((1.0f / scale), _SnapToPixelGrid(value, scale)) : 0.0f;
}

Document::~Document() {
    PROFILE

    if (_yogaConfig != nullptr) {
        ::YGConfigFree((::YGConfig*)_yogaConfig);
    }
}

Document::Document(Window& window)
    : Node()
    , _window(window)
    , _windowSub()
    , _painter()
    , _cursor(Cursor::Default)
    , _hoverNode(nullptr)
    , _activeNode(nullptr)
    , _focusedNode(nullptr)
    , _mousePosition({ 0.0f, 0.0f })
    , _mouseButton(Mouse::LeftButton)
    , _mouseIsDown(false)
    , _mouseIsDragging(false)
    , _yogaConfig(nullptr)
    , _scale(1.0f)
    , _isUpdating(false)
    , _needsUpdate(true)
    , _needsRender(true)
    , _caretVisible(true)
    , _caretBlinkStart(std::chrono::steady_clock::now())
    , _hoverPath()
    , _renderList()
{
    PROFILE

    _document = this;
    _scale = window.getScale();
    _yogaConfig = ::YGConfigNew();

    _windowSub.on(window.onEvent, [&](WindowEvent const& event) {
        auto const convertPoint = [&](Vec2 const& position) {
            return (position * (_window.getScale() / _scale));
        };

        if (auto e = event.as<MouseMoveWindowEvent>()) {
            _mouseMove(convertPoint(e->getPosition()), e->getModifiers());
        } else if (auto e = event.as<MouseDownWindowEvent>()) {
            _mouseDown(e->getMouse(), convertPoint(e->getPosition()), e->getModifiers(), e->getClickCount());
        } else if (auto e = event.as<MouseUpWindowEvent>()) {
            _mouseUp(e->getMouse(), convertPoint(e->getPosition()), e->getModifiers());
        } else if (auto e = event.as<MouseWheelWindowEvent>()) {
            _mouseWheel(convertPoint(e->getPosition()), e->getModifiers());
        } else if (auto e = event.as<KeyDownWindowEvent>()) {
            _keyDown(e->getKey(), e->getModifiers(), e->getInput());
        } else if (auto e = event.as<KeyUpWindowEvent>()) {
            _keyUp(e->getKey(), e->getModifiers());
        } else if (event.is<PaintWindowEvent>()) {
            update();
            render();
        } else if (
            event.is<ResizeWindowEvent>() ||
            event.is<DPIChangeWindowEvent>()
        ) {
            _needsUpdate = true;
        }
    });
}

Window& Document::getWindow() const {
    PROFILE

    return _window;
}

Vec2 Document::getSize() const {
    PROFILE

    return ((_window.getSize() * _window.getScale()) / _scale);
}

float Document::getScale() const {
    PROFILE

    return _scale;
}

void Document::setScale(float scale) {
    PROFILE

    if (_scale != scale) {
        _scale = scale;
        _needsUpdate = true;
    }
}

void Document::scrollNode(Node& node, Vec2 const& wheel) {
    PROFILE

    static thread_local auto _path = std::vector<Node*>();

    Node* xOverflowNode = nullptr;
    Node* yOverflowNode = nullptr;

    node.getPath(_path);

    for (auto pathNode : _path) {
        if (
            pathNode->_scrollOverflow.x > 0.0f &&
            pathNode->getOverflowX() == NodeOverflow::Scroll
        ) {
            xOverflowNode = pathNode;
        }

        if (
            pathNode->_scrollOverflow.y > 0.0f &&
            pathNode->getOverflowY() == NodeOverflow::Scroll
        ) {
            yOverflowNode = pathNode;
        }
    }

    auto const deltaX = (wheel.x * -1.0f);
    auto const deltaY = (wheel.y * -1.0f);

    if (xOverflowNode != nullptr) {
        xOverflowNode->_scrollPosition.x = _SnapToPixelGrid(std::clamp((xOverflowNode->_scrollPosition.x + deltaX), 0.0f, xOverflowNode->_scrollOverflow.x), _scale);
        _needsUpdate = true;
    }

    if (yOverflowNode != nullptr) {
        yOverflowNode->_scrollPosition.y = _SnapToPixelGrid(std::clamp((yOverflowNode->_scrollPosition.y + deltaY), 0.0f, yOverflowNode->_scrollOverflow.y), _scale);
        _needsUpdate = true;
    }
}

void Document::focusNode(Node* targetNode) {
    PROFILE

    Node* focusedNode = nullptr;
    Node* nextTarget = targetNode;
    while (nextTarget != nullptr) {
        if (
            _isNodeFocusable(*nextTarget) ||
            _isNodeEditable(*nextTarget)
        ) {
            focusedNode = nextTarget;
            break;
        }
        nextTarget = nextTarget->_parent;
    }

    if (_focusedNode != focusedNode) {
        if (_focusedNode != nullptr) {
            _focusedNode->_isFocused = false;
            if (_isNodeEditable(*_focusedNode) && _focusedNode->_firstChild->_textObject != nullptr) {
                _focusedNode->_firstChild->_textObject->setEditable(false);
                _focusedNode->_firstChild->_needsTextUpdate = true;
            }

            auto nextOldFocused = _focusedNode;
            while (nextOldFocused != nullptr) {
                nextOldFocused->_isFocusedWithin = false;
                nextOldFocused = nextOldFocused->_parent;
            }
        }

        if (focusedNode != nullptr) {
            focusedNode->_isFocused = true;
            if (_isNodeEditable(*focusedNode)) {
                if (focusedNode->_firstChild->_textObject == nullptr) {
                    focusedNode->_firstChild->_textObject = std::make_unique<Text>();
                }
                focusedNode->_firstChild->_textObject->setEditable(true);
                focusedNode->_firstChild->_needsTextUpdate = true;
            }

            auto nextNewFocused = focusedNode;
            while (nextNewFocused != nullptr) {
                nextNewFocused->_isFocusedWithin = true;
                nextNewFocused = nextNewFocused->_parent;
            }
        }

        _focusedNode = focusedNode;
        _needsUpdate = true;
        _restartCaretBlink();
    }
}

void Document::update() {
    PROFILE

    if (_isUpdating == true) {
        return;
    }

    _invalidateNode(*this);

    if (_needsUpdate == false) {
        return;
    }

    _isUpdating = true;
    _needsUpdate = false;
    _needsRender = true;
    _renderList.clear();

    _cascadeNode(*this);

    auto const size = getSize();

    ::YGConfigSetPointScaleFactor((::YGConfig*)_yogaConfig, _scale);
    ::YGNodeStyleSetWidth((::YGNode*)_layoutNode, size.width);
    ::YGNodeStyleSetHeight((::YGNode*)_layoutNode, size.height);
    ::YGNodeCalculateLayout((::YGNode*)_layoutNode, size.width, size.height, YGDirectionLTR);

    _updateNode(*this);

    _renderList[_computedZIndex].push_back(this);

    auto cursor = Cursor::Default;

    for (auto node = _hoverNode; node != nullptr; node = node->_parent) {
        if (node->_cursor.has_value()) {
            cursor = node->_cursor.value();
            break;
        }
    }

    if (_cursor != cursor) {
        _cursor = cursor;
        _window.setCursor(cursor);
    }

    _isUpdating = false;
}

void Document::render() {
    PROFILE

    if (
        (_focusedNode != nullptr) &&
        (_focusedNode->_firstChild != nullptr) &&
        (_focusedNode->_firstChild->_textObject != nullptr) &&
        _isNodeEditable(*_focusedNode)
    ) {
        auto const& text = *_focusedNode->_firstChild->_textObject;
        auto const phase = ((std::chrono::steady_clock::now() - _caretBlinkStart) / _caretBlinkPeriod);
        auto const visible = text.isSelectedRange() || ((phase % 2) == 0);

        if (_caretVisible != visible) {
            _caretVisible = visible;
            _needsRender = true;
        }
    }

    if (_needsRender == false) {
        return;
    }

    _needsRender = false;

    _painter.beginPaint(
        WindowPaintTarget{
            .window = _window,
            .clearColor = COLOR_TRANSPARENT
        }
    );

    for (auto& [ zIndex, nodes ] : _renderList) {
        for (auto& node : nodes) {
            _renderNode(*node, { 0.0f, 0.0f }, zIndex);
        }
    }

    _painter.endPaint();
}

std::optional<Document::_InputState> Document::_getInputState() {
    PROFILE

    if (
        _focusedNode != nullptr &&
        _isNodeEditable(*_focusedNode)
    ) {
        auto& textNode = *_focusedNode->_firstChild;

        /* the text child may have been swapped since focus: adopt it as the surface */
        if (textNode._textObject == nullptr) {
            textNode._textObject = std::make_unique<Text>();
            textNode._textObject->setString(textNode._content.value_or(""));
            textNode._needsTextUpdate = true;
            _needsUpdate = true;
        }

        if (textNode._textObject->getEditable() == false) {
            textNode._textObject->setEditable(true);
            textNode._needsTextUpdate = true;
            _needsUpdate = true;
        }

        return _InputState{ *_focusedNode, textNode, *textNode._textObject };
    } else {
        return std::nullopt;
    }
}

Vec2 Document::_getTextLocalPosition(_InputState const& inputState, Vec2 const& position) const {
    PROFILE

    return ((position - (inputState.textNode._computedBorderRectInDocument.origin + inputState.textNode._computedTextRect.origin)) * _scale)
        + Vec2{ inputState.textNode._textScrollX, 0.0f };
}

Node* Document::_findNodeAtPosition(Vec2 const& position) {
    PROFILE

    Node* found = nullptr;

    auto find = [&](Node* node, auto&& find) {
        if (node->_mouseEvents == false) {
            return;
        }

        if (node->_visible == false) {
            return;
        }

        if (node->_skip == true) {
            return;
        }

        auto const clipped = (
            (node->_clipped == true) &&
            (node->_parent != nullptr)
        );

        if (
            (clipped == false || position.inRect(node->_parent->_computedClipRectInDocument)) &&
            position.inRect(node->_computedBorderRectInDocument)
        ) {
            if (found != nullptr) {
                if (found->_computedZIndex <= node->_computedZIndex) {
                    found = node;
                }
            } else {
                found = node;
            }
        }

        for (auto child = node->_firstChild; child != nullptr; child = child->_nextSibling) {
            find(child, find);
        }
    };

    for (auto child = _firstChild; child != nullptr; child = child->_nextSibling) {
        find(child, find);
    }

    return found;
}

bool Document::_isNodeFocusable(Node const& node) const {
    PROFILE

    return (node._display.value_or(NodeDisplay::Box) == NodeDisplay::Box)
        && (node._tabIndex > 0);
}

bool Document::_isNodeEditable(Node const& node) const {
    PROFILE

    return (node._display.value_or(NodeDisplay::Box) == NodeDisplay::Box)
        && (node._contentEditable == true)
        && (node._firstChild != nullptr)
        && (node._firstChild->_display == NodeDisplay::Text);
}

void Document::_focusNext(bool reverse) {
    PROFILE

    static thread_local auto _candidates = std::vector<Node*>();

    _candidates.clear();

    auto const collect = [&](Node& node, auto&& collect) -> void {
        if (node._visible == false) {
            return;
        }
        if ((&node != this) && (_isNodeFocusable(node) || _isNodeEditable(node))) {
            _candidates.push_back(&node);
        }
        for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
            collect(*child, collect);
        }
    };

    collect(*this, collect);

    if (_candidates.empty()) {
        return;
    }

    auto const count = (std::int64_t)_candidates.size();
    auto current = (std::int64_t)-1;

    for (auto i = 0ll; i < count; i++) {
        if (_candidates[(std::size_t)i] == _focusedNode) {
            current = i;
            break;
        }
    }

    auto const next = reverse
        ? ((current <= 0) ? (count - 1) : (current - 1))
        : (((current < 0) || ((current + 1) >= count)) ? 0 : (current + 1));

    if (next == current) {
        return;
    }

    focusNode(_candidates[(std::size_t)next]);

    if (auto inputState = _getInputState()) {
        inputState->textObject.selectAll();
    }

    _needsUpdate = true;
}

void Document::_restartCaretBlink() {
    PROFILE

    _caretBlinkStart = std::chrono::steady_clock::now();
    _caretVisible = true;
}

void Document::_mouseWheel(Vec2 const& wheel, KeyModifiers const& modifiers) {
    PROFILE

    if (_hoverNode != nullptr) {
        scrollNode(*_hoverNode, wheel);
        _hoverNode->dispatchEvent(
            MouseWheelNodeEvent(*_hoverNode, wheel, modifiers)
        );
    }
}

void Document::_mouseMove(Vec2 const& position, KeyModifiers const& modifiers) {
    PROFILE

    static thread_local auto _newHoverPath = std::vector<Node*>();

    update();

    if (_mouseIsDown == true) {
        if (auto inputState = _getInputState()) {
            inputState->textObject.mouseMove(_getTextLocalPosition(*inputState, position));
            _restartCaretBlink();
            _needsRender = true;
        }

        if (_activeNode != nullptr) {
            auto const translate = (position - _mousePosition);

            if (_mouseIsDragging == false) {
                if (
                    std::fabsf(translate.x) > 2.0f ||
                    std::fabsf(translate.y) > 2.0f
                ) {
                    _mouseIsDragging = true;
                    _activeNode->dispatchEvent(
                        MouseBeginDragNodeEvent(*_activeNode, position, translate, modifiers)
                    );
                }
            }

            if (_mouseIsDragging == true) {
                _activeNode->dispatchEvent(
                    MouseDragNodeEvent(*_activeNode, position, translate, modifiers)
                );
            }
        }
    } else {
        auto const newHoverNode = _findNodeAtPosition(position);

        if (_hoverNode != newHoverNode) {
            _newHoverPath.clear();

            if (newHoverNode != nullptr) {
                newHoverNode->getPath(_newHoverPath);
            }

            for (auto node : _hoverPath) {
                if (std::ranges::contains(_newHoverPath, node) == false) {
                    node->_isHover = false;
                    node->dispatchEvent(
                        MouseExitNodeEvent(*node, position, modifiers)
                    );
                }
            }

            for (auto node : _newHoverPath) {
                if (std::ranges::contains(_hoverPath, node) == false) {
                    node->_isHover = true;
                    node->dispatchEvent(
                        MouseEnterNodeEvent(*node, position, modifiers)
                    );
                }
            }

            _hoverNode = newHoverNode;
            _hoverPath = _newHoverPath;
            _needsUpdate = true;
        }

        if (_hoverNode != nullptr) {
            _hoverNode->dispatchEvent(
                MouseMoveNodeEvent(*_hoverNode, position, modifiers)
            );
        }
    }
}

void Document::_mouseDown(Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers, int clickCount) {
    PROFILE

    update();

    if (_mouseIsDown == true) {
        return;
    }

    if (mouse == Mouse::LeftButton) {
        _mousePosition = position;
        _mouseButton = mouse;
        _mouseIsDown = true;
        _mouseIsDragging = false;

        _activateNode(_hoverNode);
        focusNode(_hoverNode);

        if (auto inputState = _getInputState()) {
            inputState->textObject.mouseDown(_getTextLocalPosition(*inputState, position), modifiers.shift, clickCount);
            _restartCaretBlink();
            _needsRender = true;
        }

        if (_activeNode != nullptr) {
            _activeNode->dispatchEvent(
                MouseDownNodeEvent(*_activeNode, mouse, position, modifiers, clickCount)
            );
        } else {
            dispatchEvent(
                MouseDownNodeEvent(*this, mouse, position, modifiers, clickCount)
            );
        }
    } else if (mouse == Mouse::RightButton) {
        _mousePosition = position;
        _mouseButton = mouse;
        _mouseIsDown = true;
        _mouseIsDragging = false;

        if (_hoverNode != nullptr) {
            _hoverNode->dispatchEvent(
                MouseDownNodeEvent(*_hoverNode, mouse, position, modifiers, clickCount)
            );
        } else {
            dispatchEvent(
                MouseDownNodeEvent(*this, mouse, position, modifiers, clickCount)
            );
        }
    }
}

void Document::_mouseUp(Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers) {
    PROFILE

    static thread_local auto _hoverPath  = std::vector<Node*>();
    static thread_local auto _activePath = std::vector<Node*>();

    if (_mouseIsDown == false) {
        return;
    }

    if (_mouseButton != mouse) {
        return;
    }

    if (mouse == Mouse::LeftButton) {
        if (auto inputState = _getInputState()) {
            inputState->textObject.mouseUp(_getTextLocalPosition(*inputState, position));
            _needsRender = true;
        }

        if (_activeNode != nullptr) {
            _activeNode->dispatchEvent(
                MouseUpNodeEvent(*_activeNode, mouse, position, modifiers)
            );

            if (_mouseIsDragging == true) {
                _activeNode->dispatchEvent(
                    MouseEndDragNodeEvent(*_activeNode, position, position - _mousePosition, modifiers)
                );

                _needsUpdate = true;
                _mouseIsDown = false;
                _mouseIsDragging = false;
                _activateNode(nullptr);
                _mouseMove(position, modifiers);
            } else {
                if (auto hoverNode = _findNodeAtPosition(position)) {
                    _activeNode->getPath(_activePath);
                    hoverNode->getPath(_hoverPath);

                    for (auto it = _hoverPath.rbegin(); it != _hoverPath.rend(); ++it) {
                        if (std::ranges::contains(_activePath, *it)) {
                            (*it)->dispatchEvent(
                                MouseClickNodeEvent(**it, position, modifiers)
                            );
                            break;
                        }
                    }
                }

                _needsUpdate = true;
                _mouseIsDown = false;
                _mouseIsDragging = false;
                _activateNode(nullptr);
            }
        } else {
            _mouseIsDown = false;
            _mouseIsDragging = false;
        }
    } else if (mouse == Mouse::RightButton) {
        if (_hoverNode != nullptr) {
            _hoverNode->dispatchEvent(
                MouseUpNodeEvent(*_hoverNode, mouse, position, modifiers)
            );
        }

        _mouseIsDown = false;
        _mouseIsDragging = false;
    } else {
        _mouseIsDown = false;
        _mouseIsDragging = false;
    }
}

void Document::_keyDown(Key const& key, KeyModifiers const& modifiers, std::string const& input) {
    PROFILE

    auto const keyNode = _getKeyNode();
    auto& target = (keyNode != nullptr) ? *keyNode : *this;
    auto const event = KeyDownNodeEvent(target, key, modifiers, input);

    target.dispatchEvent(event);

    if (event._defaultPrevented) {
        return;
    }

    auto consumed = false;

    if (keyNode != nullptr && _getInputState()) {
        consumed = _input(key, modifiers, input);
    }

    if (
        (consumed == false) &&
        (key == Key::Tab) &&
        (modifiers.meta == false) &&
        (modifiers.control == false) &&
        (modifiers.alt == false)
    ) {
        _focusNext(modifiers.shift);
    }
}

void Document::_keyUp(Key const& key, KeyModifiers const& modifiers) {
    PROFILE

    if (auto keyNode = _getKeyNode()) {
        keyNode->dispatchEvent(
            KeyUpNodeEvent(*keyNode, key, modifiers)
        );
    } else {
        dispatchEvent(
            KeyUpNodeEvent(*this, key, modifiers)
        );
    }
}

Node* Document::_getKeyNode() {
    PROFILE

    return ((_focusedNode != nullptr) && (_focusedNode->_keyEvents == true)) ? _focusedNode : nullptr;
}

bool Document::_input(Key const& key, KeyModifiers const& modifiers, std::string const& input) {
    PROFILE

    if (auto inputState = _getInputState()) {
        auto const singleLine = (inputState->boxNode._contentMultiLine == false);
        auto const controlOnly = modifiers.control && (modifiers.meta == false) && (modifiers.alt == false);
        auto const contentBefore = inputState->textObject.getString();

        inputState->textObject.setMultiLine(inputState->boxNode._contentMultiLine);

        if (controlOnly && (key == Key::KeyA)) {
            inputState->textObject.moveLineStart(modifiers.shift);
        } else if (controlOnly && (key == Key::KeyE)) {
            inputState->textObject.moveLineEnd(modifiers.shift);
        } else if (controlOnly && (key == Key::KeyF)) {
            inputState->textObject.moveRight(modifiers.shift);
        } else if (controlOnly && (key == Key::KeyB)) {
            inputState->textObject.moveLeft(modifiers.shift);
        } else if (controlOnly && (key == Key::KeyN)) {
            inputState->textObject.moveDown(modifiers.shift);
        } else if (controlOnly && (key == Key::KeyP)) {
            inputState->textObject.moveUp(modifiers.shift);
        } else if (controlOnly && (key == Key::KeyD)) {
            inputState->textObject.deleteForward();
        } else if (controlOnly && (key == Key::KeyH)) {
            inputState->textObject.deleteBackward();
        } else if (controlOnly && (key == Key::KeyK)) {
            inputState->textObject.deleteLineForward();
        } else if (key == Key::ArrowLeft) {
            if (modifiers.meta) {
                inputState->textObject.moveLineStart(modifiers.shift);
            } else if (modifiers.alt) {
                inputState->textObject.moveWordLeft(modifiers.shift);
            } else {
                inputState->textObject.moveLeft(modifiers.shift);
            }
        } else if (key == Key::ArrowRight) {
            if (modifiers.meta) {
                inputState->textObject.moveLineEnd(modifiers.shift);
            } else if (modifiers.alt) {
                inputState->textObject.moveWordRight(modifiers.shift);
            } else {
                inputState->textObject.moveRight(modifiers.shift);
            }
        } else if (key == Key::ArrowUp) {
            if (modifiers.meta) {
                inputState->textObject.moveDocumentStart(modifiers.shift);
            } else {
                inputState->textObject.moveUp(modifiers.shift);
            }
        } else if (key == Key::ArrowDown) {
            if (modifiers.meta) {
                inputState->textObject.moveDocumentEnd(modifiers.shift);
            } else {
                inputState->textObject.moveDown(modifiers.shift);
            }
        } else if (key == Key::Home) {
            inputState->textObject.moveLineStart(modifiers.shift);
        } else if (key == Key::End) {
            inputState->textObject.moveLineEnd(modifiers.shift);
        } else if (key == Key::Backspace) {
            if (modifiers.meta) {
                inputState->textObject.deleteLineBackward();
            } else if (modifiers.alt) {
                inputState->textObject.deleteWordBackward();
            } else {
                inputState->textObject.deleteBackward();
            }
        } else if (key == Key::Delete) {
            if (modifiers.meta) {
                inputState->textObject.deleteLineForward();
            } else if (modifiers.alt) {
                inputState->textObject.deleteWordForward();
            } else {
                inputState->textObject.deleteForward();
            }
        } else if ((key == Key::Enter) || (key == Key::NumpadEnter)) {
            if (singleLine) {
                return false; /* not inserted; the KeyDownNodeEvent already dispatched serves as the submit hook */
            }
            inputState->textObject.input("\n");
        } else if (key == Key::Tab) {
            if (singleLine) {
                return false; /* left for focus traversal */
            }
            inputState->textObject.input("    "); /* the Text pipeline has no tab-stop handling, so insert spaces */
        } else if ((key == Key::KeyZ) && modifiers.meta) {
            if (modifiers.shift) {
                inputState->textObject.redo();
            } else {
                inputState->textObject.undo();
            }
        } else if ((key == Key::KeyA) && modifiers.meta) {
            inputState->textObject.selectAll();
        } else if ((key == Key::KeyC) && modifiers.meta) {
            if ((inputState->boxNode._contentSecure || inputState->textNode._contentSecure) == false) {
                auto string = std::string();
                inputState->textObject.copy(string);
                if (string.empty() == false) {
                    SetClipboardString(string);
                }
            }
        } else if ((key == Key::KeyX) && modifiers.meta) {
            if ((inputState->boxNode._contentSecure || inputState->textNode._contentSecure) == false) {
                auto string = std::string();
                inputState->textObject.cut(string);
                if (string.empty() == false) {
                    SetClipboardString(string);
                }
            }
        } else if ((key == Key::KeyV) && modifiers.meta) {
            inputState->textObject.paste(GetClipboardString());
        } else if (
            (input.empty() == false) &&
            (modifiers.meta == false) &&
            (modifiers.control == false)
        ) {
            inputState->textObject.input(input);
        } else {
            return false;
        }

        _needsUpdate = true;
        _restartCaretBlink();

        auto const& contentAfter = inputState->textObject.getString();
        if (contentAfter != contentBefore) {
            inputState->textNode.setContent(contentAfter);
            inputState->boxNode.dispatchEvent(
                InputNodeEvent(*_focusedNode, contentAfter)
            );
        }

        return true;
    }

    return false;
}

void Document::_invalidateNode(Node& node) {
    PROFILE

    static auto const _invalidateTextObject = [](Node& node, auto&& _invalidateTextObject) -> void {
        if (node._textNode != nullptr) {
            ::YGNodeMarkDirty((::YGNode*)node._textNode);
        } else if (node._parent != nullptr) {
            _invalidateTextObject(*node._parent, _invalidateTextObject);
        }
    };

    if (node._needsLayoutUpdate == true) {
        node._needsLayoutUpdate = false;
        _needsUpdate = true;
    }

    if (node._needsTextUpdate == true) {
        node._needsTextUpdate  = false;
        _invalidateTextObject(node, _invalidateTextObject);
        _needsUpdate = true;

        for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
            child->_needsTextUpdate = true;
        }
    }

    for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
        _invalidateNode(*child);
    }
}

void Document::_cascadeNode(Node& node) {
    PROFILE

    if (node._parent != nullptr) {
        node._computedFontFamily  = node._fontFamily.value_or(node._parent->_computedFontFamily);
        node._computedFontWeight  = node._fontWeight.value_or(node._parent->_computedFontWeight);
        node._computedFontStyle   = node._fontStyle.value_or(node._parent->_computedFontStyle);
        node._computedFontSize    = node._fontSize.value_or(node._parent->_computedFontSize);
        node._computedTextColor   = node._textColor.value_or(node._parent->_computedTextColor);
        node._computedMarkerColor = node._textMarker.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
        node._computedLineHeight  = node._lineHeight.value_or(node._parent->_computedLineHeight);
    } else {
        node._computedFontFamily  = node._fontFamily.value_or(TEXT_DEFAULT_FONT_FAMILY);
        node._computedFontWeight  = node._fontWeight.value_or(TEXT_DEFAULT_FONT_WEIGHT);
        node._computedFontStyle   = node._fontStyle.value_or(TEXT_DEFAULT_FONT_STYLE);
        node._computedFontSize    = node._fontSize.value_or(TEXT_DEFAULT_FONT_SIZE);
        node._computedTextColor   = node._textColor.value_or(TEXT_DEFAULT_COLOR);
        node._computedMarkerColor = node._textMarker.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
        node._computedLineHeight  = node._lineHeight.value_or(TEXT_DEFAULT_LINE_HEIGHT);
    }

    for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
        _cascadeNode(*child);
    }
}

void Document::_updateNode(Node& node) {
    PROFILE

    auto const margin = Vec4{
        ::YGNodeLayoutGetMargin((::YGNode*)node._layoutNode, ::YGEdgeLeft),
        ::YGNodeLayoutGetMargin((::YGNode*)node._layoutNode, ::YGEdgeTop),
        ::YGNodeLayoutGetMargin((::YGNode*)node._layoutNode, ::YGEdgeRight),
        ::YGNodeLayoutGetMargin((::YGNode*)node._layoutNode, ::YGEdgeBottom)
    };

    auto const padding = Vec4{
        ::YGNodeLayoutGetPadding((::YGNode*)node._layoutNode, ::YGEdgeLeft),
        ::YGNodeLayoutGetPadding((::YGNode*)node._layoutNode, ::YGEdgeTop),
        ::YGNodeLayoutGetPadding((::YGNode*)node._layoutNode, ::YGEdgeRight),
        ::YGNodeLayoutGetPadding((::YGNode*)node._layoutNode, ::YGEdgeBottom)
    };

    node._computedBorderEdge = {
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeLeft), _scale),
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeTop), _scale),
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeRight), _scale),
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeBottom), _scale)
    };

    node._computedBorderRect = {
        ::YGNodeLayoutGetLeft((::YGNode*)node._layoutNode),
        ::YGNodeLayoutGetTop((::YGNode*)node._layoutNode),
        ::YGNodeLayoutGetWidth((::YGNode*)node._layoutNode),
        ::YGNodeLayoutGetHeight((::YGNode*)node._layoutNode)
    };

    if (std::isfinite(node._computedBorderRect.x) == false)      node._computedBorderRect.x = 0.0f;
    if (std::isfinite(node._computedBorderRect.y) == false)      node._computedBorderRect.y = 0.0f;
    if (std::isfinite(node._computedBorderRect.width) == false)  node._computedBorderRect.width = 0.0f;
    if (std::isfinite(node._computedBorderRect.height) == false) node._computedBorderRect.height = 0.0f;

    if (node._offset) {
        node._computedBorderRect.origin.x += _SnapToPixelGrid(node._offset->x, _scale);
        node._computedBorderRect.origin.y += _SnapToPixelGrid(node._offset->y, _scale);
    }

    if (node._transform) {
        node._computedBorderRect.origin.x += _SnapToPixelGrid(node._transform->translateX.match(
            [](PixelValue const& pixel) { return pixel.value; },
            [&](PercentValue const& percent) { return ((percent.value / 100.0f) * node._computedBorderRect.width); }
        ), _scale);
        node._computedBorderRect.origin.y += _SnapToPixelGrid(node._transform->translateY.match(
            [](PixelValue const& pixel) { return pixel.value; },
            [&](PercentValue const& percent) { return ((percent.value / 100.0f) * node._computedBorderRect.height); }
        ), _scale);
    }

    node._computedMarginRect = {
        (node._computedBorderRect.x - margin.left),
        (node._computedBorderRect.y - margin.top),
        (node._computedBorderRect.width + margin.left + margin.right),
        (node._computedBorderRect.height + margin.top + margin.bottom)
    };

    if ((node._display == NodeDisplay::Text) && (node._textNode != nullptr)) {
        node._computedTextRect = {
            ::YGNodeLayoutGetLeft((::YGNode*)node._textNode),
            ::YGNodeLayoutGetTop((::YGNode*)node._textNode),
            ::YGNodeLayoutGetWidth((::YGNode*)node._textNode),
            ::YGNodeLayoutGetHeight((::YGNode*)node._textNode)
        };
    }

    auto const contentSize = getSize();

    node._computedClipRectInDocument = Vec4{ 0.0f, 0.0f, contentSize.width, contentSize.height };
    node._computedBorderRectInDocument = node._computedBorderRect;
    node._computedMarginRectInDocument = node._computedMarginRect;
    node._computedZIndex = node._zIndex.value_or(0);

    if (node._parent != nullptr) {
        node._computedBorderRectInDocument.origin += node._parent->_computedBorderRectInDocument.origin;
        node._computedMarginRectInDocument.origin += node._parent->_computedBorderRectInDocument.origin;

        if (node._position != NodePosition::Fixed) {
            node._computedBorderRectInDocument.origin -= node._parent->_scrollPosition;
            node._computedMarginRectInDocument.origin -= node._parent->_scrollPosition;
        }

        if (node._clipped == true) {
            node._computedClipRectInDocument = node._parent->_computedClipRectInDocument;
        }

        if (node._computedZIndex < node._parent->_computedZIndex) {
            node._computedZIndex = node._parent->_computedZIndex;
        }

        if (node._computedZIndex > node._parent->_computedZIndex) {
            _renderList[node._computedZIndex].push_back(&node);
        }
    }

    auto const& borderEdge = node._computedBorderEdge;
    auto const innerBorderRect = Vec4{
        (node._computedBorderRectInDocument.x + borderEdge.left),
        (node._computedBorderRectInDocument.y + borderEdge.top),
        std::max(0.0f, (node._computedBorderRectInDocument.width - borderEdge.left - borderEdge.right)),
        std::max(0.0f, (node._computedBorderRectInDocument.height - borderEdge.top - borderEdge.bottom))
    };

    if (
        (node._overflowX == NodeOverflow::Hidden) ||
        (node._overflowX == NodeOverflow::Scroll)
    ) {
        auto newClipRect = node._computedClipRectInDocument.getIntersection(innerBorderRect);
        node._computedClipRectInDocument.x = newClipRect.x;
        node._computedClipRectInDocument.width = newClipRect.width;
    }

    if (
        (node._overflowY == NodeOverflow::Hidden) ||
        (node._overflowY == NodeOverflow::Scroll)
    ) {
        auto newClipRect = node._computedClipRectInDocument.getIntersection(innerBorderRect);
        node._computedClipRectInDocument.y = newClipRect.y;
        node._computedClipRectInDocument.height = newClipRect.height;
    }

    auto scrollMaxX = 0.0f;
    auto scrollMaxY = 0.0f;
    auto contentMinX = 0.0f;
    auto contentMinY = 0.0f;
    auto contentMaxX = node._computedBorderRect.width;
    auto contentMaxY = node._computedBorderRect.height;

    for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
        _updateNode(*child);

        if (child->_position == NodePosition::Fixed) {
            continue; // fixed children don't contribute to the content box or scrollable area
        }

        auto childLeft   = child->_computedMarginRect.x;
        auto childTop    = child->_computedMarginRect.y;
        auto childRight  = child->_computedMarginRect.getMaxX();
        auto childBottom = child->_computedMarginRect.getMaxY();

        if (
            (child->_overflowX != NodeOverflow::Hidden) &&
            (child->_overflowX != NodeOverflow::Scroll)
        ) {
            childLeft  = std::min(childLeft,  (child->_computedBorderRect.x + child->_computedContentRect.x));
            childRight = std::max(childRight, (child->_computedBorderRect.x + child->_computedContentRect.getMaxX()));
        }
        if (
            (child->_overflowY != NodeOverflow::Hidden) &&
            (child->_overflowY != NodeOverflow::Scroll)
        ) {
            childTop    = std::min(childTop,    (child->_computedBorderRect.y + child->_computedContentRect.y));
            childBottom = std::max(childBottom, (child->_computedBorderRect.y + child->_computedContentRect.getMaxY()));
        }

        contentMinX = std::min(contentMinX, childLeft);
        contentMinY = std::min(contentMinY, childTop);
        contentMaxX = std::max(contentMaxX, childRight);
        contentMaxY = std::max(contentMaxY, childBottom);
        scrollMaxX = std::max(scrollMaxX, childRight);
        scrollMaxY = std::max(scrollMaxY, childBottom);
    }

    node._computedContentRect = {
        contentMinX,
        contentMinY,
        (contentMaxX - contentMinX),
        (contentMaxY - contentMinY)
    };

    node._scrollOverflow = {
        _SnapToPixelGrid(std::max(0.0f, (scrollMaxX - (node._computedBorderRect.width  - node._computedBorderEdge.right  - padding.right))), _scale),
        _SnapToPixelGrid(std::max(0.0f, (scrollMaxY - (node._computedBorderRect.height - node._computedBorderEdge.bottom - padding.bottom))), _scale)
    };

    node._scrollPosition = {
        std::clamp(node._scrollPosition.x, 0.0f, node._scrollOverflow.x),
        std::clamp(node._scrollPosition.y, 0.0f, node._scrollOverflow.y)
    };
}

void Document::_renderNode(Node& node, Vec2 const& offset, int zIndex) {
    PROFILE

    if (node._visible == false) {
        return;
    }

    if (node._computedZIndex != zIndex) {
        return;
    }

    auto nodeOffset = offset;
    auto nodeClipRect = (node._computedClipRectInDocument * _scale);
    auto nodeBorderRect = (node._computedBorderRectInDocument * _scale);
    auto scissorRect = std::optional<Vec4>{};
    auto layerOffset = Vec2{};
    auto layerRect = Vec4{};
    auto layerNeeded = (
        node._opacity.value_or(1.0f) < 1.0f ||
        node._shadow.has_value()
    );

    nodeClipRect.origin += nodeOffset;
    nodeBorderRect.origin += nodeOffset;

    if (
        (node._clipped == true) &&
        (node._parent != nullptr)
    ) {
        auto parentClipRect = (node._parent->_computedClipRectInDocument * _scale);
        parentClipRect.origin += nodeOffset;
        scissorRect = parentClipRect;
    }

    /* The node's own box is clipped by its ancestors only; its own overflow
       clip applies to its children. Inside a layer the ancestor clip is
       applied when the layer is composited. */
    auto boxScissorRect = scissorRect;

    if (layerNeeded) {
        auto nodeContentRect = (node._computedContentRect * _scale);
        auto nodeContentOffset = Vec2{
            std::min(0.0f, nodeContentRect.x),
            std::min(0.0f, nodeContentRect.y)
        };

        layerOffset = (nodeBorderRect.origin + nodeContentOffset);
        layerRect = {
            (nodeBorderRect.x + nodeContentOffset.width),
            (nodeBorderRect.y + nodeContentOffset.height),
            std::max(nodeBorderRect.width, nodeContentRect.width),
            std::max(nodeBorderRect.height, nodeContentRect.height)
        };

        if (
            (layerRect.width < 1.0f) ||
            (layerRect.height < 1.0f)
        ) {
            return;
        }

        if (
            (node._layerImage == nullptr) ||
            (node._layerImage->getSize() != layerRect.size)
        ) {
            node._layerImage = std::make_unique<Image>(layerRect.size);
        }

        nodeBorderRect.origin -= layerOffset;
        nodeClipRect.origin -= layerOffset;
        nodeOffset -= layerOffset;
        boxScissorRect = std::nullopt;

        _painter.beginPaint(
            ImagePaintTarget{
                .image = *node._layerImage,
                .clearColor = COLOR_TRANSPARENT
            }
        );
    }

    auto const scaledRadius = [this](std::optional<float> const& radius) -> std::optional<float> {
        return radius.has_value() ? std::optional<float>{ (*radius * _scale) } : std::nullopt;
    };

    if (node._background) {
        auto const backgroundBrush = *node._background;
        auto const backgroundShape = QuadShape{
            .rect = nodeBorderRect,
            .borderRadius = scaledRadius(node._borderRadius),
            .borderTopLeftRadius = scaledRadius(node._borderTopLeftRadius),
            .borderTopRightRadius = scaledRadius(node._borderTopRightRadius),
            .borderBottomLeftRadius = scaledRadius(node._borderBottomLeftRadius),
            .borderBottomRightRadius = scaledRadius(node._borderBottomRightRadius)
        };
        _painter.paint(backgroundShape, backgroundBrush, { .scissor = boxScissorRect });
    }

    if (node._border) {
        auto const borderBrush = *node._border;
        auto const borderShape = QuadOutlineShape{
            .rect = nodeBorderRect,
            .borderRadius = scaledRadius(node._borderRadius),
            .borderTopLeftRadius = scaledRadius(node._borderTopLeftRadius),
            .borderTopRightRadius = scaledRadius(node._borderTopRightRadius),
            .borderBottomLeftRadius = scaledRadius(node._borderBottomLeftRadius),
            .borderBottomRightRadius = scaledRadius(node._borderBottomRightRadius),
            .leftBorder = (node._computedBorderEdge.left * _scale),
            .topBorder = (node._computedBorderEdge.top * _scale),
            .rightBorder = (node._computedBorderEdge.right * _scale),
            .bottomBorder = (node._computedBorderEdge.bottom * _scale)
        };
        _painter.paint(borderShape, borderBrush, { .scissor = boxScissorRect });
    }

    auto const text = node._textObject.get();

    if (
        (node._display == NodeDisplay::Text) &&
        (text != nullptr)
    ) {
        auto const editable = text->getEditable();
        auto const singleLine = (text->getMultiLine() == false) && (node._parent != nullptr);

        text->setMaxWidth(std::nullopt);
        text->setMaxHeight(std::nullopt);
        text->setWidth(singleLine ? std::nullopt : std::optional<float>(std::floorf(node._computedTextRect.width * _scale)));
        text->setHeight(std::floorf(node._computedTextRect.height * _scale));

        auto const textOrigin = (nodeBorderRect.origin + (node._computedTextRect.origin * _scale));
        auto textClipRect = nodeClipRect;
        auto textRect = Vec4{
            Vec2{ std::roundf(textOrigin.x), std::roundf(textOrigin.y) },
            text->getSize()
        };

        if (singleLine) {
            /* A single-line surface never wraps and is clipped to the slot
               the box gives its text (the box's border rect minus its border
               and, mirrored, the text's left inset). While it is being
               edited it scrolls horizontally so the caret stays inside that
               slot; unfocused, it shows its start. */
            auto const inset = (node._computedBorderRectInDocument.x - node._parent->_computedBorderRectInDocument.x - node._parent->_computedBorderEdge.left);
            auto const visibleMaxX = ((node._parent->_computedBorderRectInDocument.getMaxX() - node._parent->_computedBorderEdge.right - inset) * _scale) + nodeOffset.x;
            auto const visibleWidth = std::max(1.0f, (visibleMaxX - nodeBorderRect.x));
            auto& scrollX = node._textScrollX;

            if (editable) {
                auto const& caretRect = text->getCaretRect();

                if ((caretRect.getMaxX() - scrollX) > visibleWidth) {
                    scrollX = (caretRect.getMaxX() - visibleWidth);
                }
                if ((caretRect.x - scrollX) < 0.0f) {
                    scrollX = caretRect.x;
                }

                scrollX = std::ceilf(std::clamp(scrollX, 0.0f, std::max(0.0f, (std::max(textRect.width, caretRect.getMaxX()) - visibleWidth))));
            } else {
                scrollX = 0.0f;
            }

            textRect.x -= scrollX;

            auto const clipMinX = std::max(nodeClipRect.x, nodeBorderRect.x);
            auto const clipMaxX = std::min(nodeClipRect.getMaxX(), (nodeBorderRect.x + visibleWidth));
            textClipRect = { clipMinX, nodeClipRect.y, std::max(0.0f, (clipMaxX - clipMinX)), nodeClipRect.height };
        } else {
            node._textScrollX = 0.0f;
        }

        if (editable) {
            for (auto const& rect : text->getSelectionRects()) {
                auto const selectionShape = QuadShape{ Vec4{ (textRect.origin + rect.origin), rect.size } };
                auto const selectionBrush = ColorBrush{ .color = _selectionColor };
                _painter.paint(selectionShape, selectionBrush, { .scissor = textClipRect });
            }
        }

        auto const textShape = QuadShape{ textRect };
        auto const textBrush = ImageBrush{
            .image = text->getImage(),
            .positionX = ImagePosition::Start,
            .positionY = ImagePosition::Start,
            .filterMag = ImageFilter::Linear,
            .filterMin = ImageFilter::Linear
        };
        _painter.paint(textShape, textBrush, { .scissor = textClipRect });

        if (
            editable &&
            _caretVisible &&
            (text->isSelectedRange() == false)
        ) {
            auto const& caretRect = text->getCaretRect();
            auto const caretShape = QuadShape{ Vec4{ (textRect.origin + caretRect.origin), caretRect.size } };
            auto const caretBrush = ColorBrush{ .color = node._computedTextColor };
            _painter.paint(caretShape, caretBrush, { .scissor = textClipRect });
        }
    }

    for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
        _renderNode(*child, nodeOffset, zIndex);
    }

    if (node._foreground) {
        auto const foregroundBrush = *node._foreground;
        auto const foregroundShape = QuadShape{
            .rect = nodeBorderRect,
            .borderRadius = scaledRadius(node._borderRadius),
            .borderTopLeftRadius = scaledRadius(node._borderTopLeftRadius),
            .borderTopRightRadius = scaledRadius(node._borderTopRightRadius),
            .borderBottomLeftRadius = scaledRadius(node._borderBottomLeftRadius),
            .borderBottomRightRadius = scaledRadius(node._borderBottomRightRadius)
        };
        _painter.paint(foregroundShape, foregroundBrush, { .scissor = boxScissorRect });
    }

    if (layerNeeded) {
        _painter.endPaint();

        auto const shape = QuadShape{ layerRect };
        auto const brush = ImageBrush{
            .image = &*node._layerImage,
            .positionX = ImagePosition::Start,
            .positionY = ImagePosition::Start,
            .filterMag = ImageFilter::Nearest,
            .filterMin = ImageFilter::Nearest
        };

        if (node._shadow.has_value()) {
            auto const& shadow = *node._shadow;
            auto const shadowColor = shadow.color.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 1.0f });

            if (shadowColor.alpha > 0.0f) {
                auto const radius  = (shadow.blur.value_or(0.0f) * _scale);
                auto const spread  = (shadow.spread.value_or(Vec2{}) * _scale);
                auto const offset = (shadow.offset.value_or(Vec2{}) * _scale);
                auto const padding = Vec2{
                    std::ceil(radius + std::max(spread.x, 0.0f)),
                    std::ceil(radius + std::max(spread.y, 0.0f))
                };
                auto const shadowSize = Vec2{
                    (layerRect.width  + (padding.x * 2.0f)),
                    (layerRect.height + (padding.y * 2.0f))
                };
                auto const shadowShape = QuadShape{
                    Vec4{ padding, layerRect.size }
                };

                auto rebake = false;
                auto shadowKey = shadow;
                shadowKey.offset = std::nullopt;

                auto const clipKey = nodeClipRect.getIntersection(
                    Vec4{ 0.0f, 0.0f, layerRect.width, layerRect.height }
                );

                auto const radiusKey = Vec4{
                    node._borderTopLeftRadius.value_or(node._borderRadius.value_or(0.0f)),
                    node._borderTopRightRadius.value_or(node._borderRadius.value_or(0.0f)),
                    node._borderBottomRightRadius.value_or(node._borderRadius.value_or(0.0f)),
                    node._borderBottomLeftRadius.value_or(node._borderRadius.value_or(0.0f))
                };

                if (
                    (node._shadowImage == nullptr) ||
                    (node._shadowImage->getSize() != shadowSize)
                ) {
                    node._shadowImage = std::make_unique<Image>(shadowSize);
                    rebake = true;
                }

                if (
                    rebake ||
                    (node._shadowImageShadow != shadowKey) ||
                    (node._shadowImageRadius != radiusKey) ||
                    (node._shadowImageScale != _scale) ||
                    (node._shadowImageClipRect != clipKey)
                ) {
                    node._shadowImageShadow = shadowKey;
                    node._shadowImageRadius = radiusKey;
                    node._shadowImageScale = _scale;
                    node._shadowImageClipRect = clipKey;

                    _painter.beginPaint(
                        ImagePaintTarget{
                            .image = *node._shadowImage,
                            .clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f }
                        }
                    );
                    _painter.paint(shadowShape, brush, {
                        .filter = Filter{
                            ShadowFilter{
                                .radius = radius,
                                .color = shadowColor,
                                .spread = (shadow.spread.has_value() ? std::optional<Vec2>{ spread } : std::optional<Vec2>{})
                            }
                        }
                    });
                    _painter.endPaint();
                }

                auto const shadowImageShape = QuadShape{ Vec4{ (layerRect.origin - padding + offset), shadowSize } };
                auto const shadowImageBrush = ImageBrush{
                    .image = &*node._shadowImage,
                    .positionX = ImagePosition::Start,
                    .positionY = ImagePosition::Start,
                    .filterMag = ImageFilter::Nearest,
                    .filterMin = ImageFilter::Nearest
                };
                _painter.paint(shadowImageShape, shadowImageBrush, {
                    .scissor = scissorRect,
                    .opacity = node._opacity
                });
            }
        }

        _painter.paint(shape, brush, {
            .scissor = scissorRect,
            .opacity = node._opacity
        });
    }
}

void Document::_activateNode(Node* targetNode) {
    PROFILE

    Node* nextOldActive = _activeNode;
    while (nextOldActive != nullptr) {
        nextOldActive->_isActive = false;
        nextOldActive = nextOldActive->_parent;
    }

    Node* nextNewActive = targetNode;
    while (nextNewActive != nullptr) {
        nextNewActive->_isActive = true;
        nextNewActive = nextNewActive->_parent;
    }

    _activeNode = targetNode;
    _needsUpdate = true;
}

} /* namespace Rocket */
