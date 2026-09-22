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
        auto const& pathLayout = pathNode->_layoutState;

        if (
            pathLayout.scrollOverflow.x > 0.0f &&
            pathNode->getOverflowX() == NodeOverflow::Scroll
        ) {
            xOverflowNode = pathNode;
        }

        if (
            pathLayout.scrollOverflow.y > 0.0f &&
            pathNode->getOverflowY() == NodeOverflow::Scroll
        ) {
            yOverflowNode = pathNode;
        }
    }

    auto const deltaX = (wheel.x * -1.0f);
    auto const deltaY = (wheel.y * -1.0f);

    if (xOverflowNode != nullptr) {
        xOverflowNode->_layoutState.scrollPosition.x = _SnapToPixelGrid(std::clamp((xOverflowNode->_layoutState.scrollPosition.x + deltaX), 0.0f, xOverflowNode->_layoutState.scrollOverflow.x), _scale);
        _needsUpdate = true;
    }

    if (yOverflowNode != nullptr) {
        yOverflowNode->_layoutState.scrollPosition.y = _SnapToPixelGrid(std::clamp((yOverflowNode->_layoutState.scrollPosition.y + deltaY), 0.0f, yOverflowNode->_layoutState.scrollOverflow.y), _scale);
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
            _focusedNode->_inputState.isFocused = false;
            if (_isNodeEditable(*_focusedNode) && _focusedNode->_firstChild->_textState.text != nullptr) {
                _focusedNode->_firstChild->_textState.text->setEditable(false);
                _focusedNode->_firstChild->_textState.invalidate = true;
            }

            auto nextOldFocused = _focusedNode;
            while (nextOldFocused != nullptr) {
                nextOldFocused->_inputState.isFocusedWithin = false;
                nextOldFocused = nextOldFocused->_parent;
            }
        }

        if (focusedNode != nullptr) {
            focusedNode->_inputState.isFocused = true;
            if (_isNodeEditable(*focusedNode)) {
                if (focusedNode->_firstChild->_textState.text == nullptr) {
                    focusedNode->_firstChild->_textState.text = std::make_unique<Text>();
                }
                focusedNode->_firstChild->_textState.text->setEditable(true);
                focusedNode->_firstChild->_textState.invalidate = true;
            }

            auto nextNewFocused = focusedNode;
            while (nextNewFocused != nullptr) {
                nextNewFocused->_inputState.isFocusedWithin = true;
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

    _renderList[_layoutState.computedZIndex].push_back(this);

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
        (_focusedNode->_firstChild->_textState.text != nullptr) &&
        _isNodeEditable(*_focusedNode)
    ) {
        auto const& text = *_focusedNode->_firstChild->_textState.text;
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
        if (textNode._textState.text == nullptr) {
            textNode._textState.text = std::make_unique<Text>();
            textNode._textState.text->setString(textNode._content.value_or(""));
            textNode._textState.invalidate = true;
            _needsUpdate = true;
        }

        if (textNode._textState.text->getEditable() == false) {
            textNode._textState.text->setEditable(true);
            textNode._textState.invalidate = true;
            _needsUpdate = true;
        }

        return _InputState{ *_focusedNode, textNode, *textNode._textState.text };
    } else {
        return std::nullopt;
    }
}

Vec2 Document::_getTextLocalPosition(_InputState const& inputState, Vec2 const& position) const {
    PROFILE

    auto const& textLayout = inputState.textNode._layoutState;

    return ((position - (textLayout.computedBorderRect.origin + textLayout.textRect.origin)) * _scale)
        + Vec2{ inputState.textNode._textState.scrollX, 0.0f };
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

        auto const& nodeLayout = node->_layoutState;

        auto const clipped = (
            (node->_clipped == true) &&
            (node->_parent != nullptr)
        );

        if (
            (clipped == false || position.inRect(node->_parent->_layoutState.computedClipRect)) &&
            position.inRect(nodeLayout.computedBorderRect)
        ) {
            if (found != nullptr) {
                if (found->_layoutState.computedZIndex <= nodeLayout.computedZIndex) {
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
                    node->_inputState.isHover = false;
                    node->dispatchEvent(
                        MouseExitNodeEvent(*node, position, modifiers)
                    );
                }
            }

            for (auto node : _newHoverPath) {
                if (std::ranges::contains(_hoverPath, node) == false) {
                    node->_inputState.isHover = true;
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

    auto& target = (_focusedNode != nullptr) ? *_focusedNode : *this;
    auto const event = KeyDownNodeEvent(target, key, modifiers, input);

    target.dispatchEvent(event);

    if (event._defaultPrevented) {
        return;
    }

    auto consumed = false;

    if (_focusedNode != nullptr && _getInputState()) {
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

    if (_focusedNode != nullptr) {
        _focusedNode->dispatchEvent(
            KeyUpNodeEvent(*_focusedNode, key, modifiers)
        );
    } else {
        dispatchEvent(
            KeyUpNodeEvent(*this, key, modifiers)
        );
    }
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
            if (inputState->boxNode._contentSecure == false) {
                auto string = std::string();
                inputState->textObject.copy(string);
                if (string.empty() == false) {
                    SetClipboardString(string);
                }
            }
        } else if ((key == Key::KeyX) && modifiers.meta) {
            if (inputState->boxNode._contentSecure == false) {
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

    if (node._layoutState.invalidate == true) {
        node._layoutState.invalidate = false;
        _needsUpdate = true;
    }

    if (node._textState.invalidate == true) {
        node._textState.invalidate = false;
        _invalidateTextObject(node, _invalidateTextObject);
        _needsUpdate = true;

        for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
            child->_textState.invalidate = true;
        }
    }

    for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
        _invalidateNode(*child);
    }
}

void Document::_cascadeNode(Node& node) {
    PROFILE

    if (node._parent != nullptr) {
        node._textState.fontFamily = node._fontFamily.value_or(node._parent->_textState.fontFamily);
        node._textState.fontWeight = node._fontWeight.value_or(node._parent->_textState.fontWeight);
        node._textState.fontStyle  = node._fontStyle.value_or(node._parent->_textState.fontStyle);
        node._textState.fontSize   = node._fontSize.value_or(node._parent->_textState.fontSize);
        node._textState.textColor  = node._textColor.value_or(node._parent->_textState.textColor);
        node._textState.marker     = node._textMarker;
        node._textState.lineHeight = node._lineHeight.value_or(node._parent->_textState.lineHeight);
    } else {
        node._textState.fontFamily = node._fontFamily.value_or(TEXT_DEFAULT_FONT_FAMILY);
        node._textState.fontWeight = node._fontWeight.value_or(TEXT_DEFAULT_FONT_WEIGHT);
        node._textState.fontStyle  = node._fontStyle.value_or(TEXT_DEFAULT_FONT_STYLE);
        node._textState.fontSize   = node._fontSize.value_or(TEXT_DEFAULT_FONT_SIZE);
        node._textState.textColor  = node._textColor.value_or(TEXT_DEFAULT_COLOR);
        node._textState.marker     = node._textMarker;
        node._textState.lineHeight = node._lineHeight.value_or(TEXT_DEFAULT_LINE_HEIGHT);
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

    node._layoutState.borderEdge = {
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeLeft), _scale),
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeTop), _scale),
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeRight), _scale),
        _SnapBorderToPixelGrid(::YGNodeLayoutGetBorder((::YGNode*)node._layoutNode, ::YGEdgeBottom), _scale)
    };

    node._layoutState.borderRect = {
        ::YGNodeLayoutGetLeft((::YGNode*)node._layoutNode),
        ::YGNodeLayoutGetTop((::YGNode*)node._layoutNode),
        ::YGNodeLayoutGetWidth((::YGNode*)node._layoutNode),
        ::YGNodeLayoutGetHeight((::YGNode*)node._layoutNode)
    };

    if (std::isfinite(node._layoutState.borderRect.x) == false)      node._layoutState.borderRect.x = 0.0f;
    if (std::isfinite(node._layoutState.borderRect.y) == false)      node._layoutState.borderRect.y = 0.0f;
    if (std::isfinite(node._layoutState.borderRect.width) == false)  node._layoutState.borderRect.width = 0.0f;
    if (std::isfinite(node._layoutState.borderRect.height) == false) node._layoutState.borderRect.height = 0.0f;

    if (node._offset) {
        node._layoutState.borderRect.origin.x += _SnapToPixelGrid(node._offset->x, _scale);
        node._layoutState.borderRect.origin.y += _SnapToPixelGrid(node._offset->y, _scale);
    }

    if (node._transform) {
        node._layoutState.borderRect.origin.x += _SnapToPixelGrid(node._transform->translateX.match(
            [](PixelValue const& pixel) { return pixel.value; },
            [&](PercentValue const& percent) { return ((percent.value / 100.0f) * node._layoutState.borderRect.width); }
        ), _scale);
        node._layoutState.borderRect.origin.y += _SnapToPixelGrid(node._transform->translateY.match(
            [](PixelValue const& pixel) { return pixel.value; },
            [&](PercentValue const& percent) { return ((percent.value / 100.0f) * node._layoutState.borderRect.height); }
        ), _scale);
    }

    node._layoutState.marginRect = {
        (node._layoutState.borderRect.x - margin.left),
        (node._layoutState.borderRect.y - margin.top),
        (node._layoutState.borderRect.width + margin.left + margin.right),
        (node._layoutState.borderRect.height + margin.top + margin.bottom)
    };

    if ((node._display == NodeDisplay::Text) && (node._textNode != nullptr)) {
        node._layoutState.textRect = {
            ::YGNodeLayoutGetLeft((::YGNode*)node._textNode),
            ::YGNodeLayoutGetTop((::YGNode*)node._textNode),
            ::YGNodeLayoutGetWidth((::YGNode*)node._textNode),
            ::YGNodeLayoutGetHeight((::YGNode*)node._textNode)
        };
    }

    auto const contentSize = getSize();

    node._layoutState.computedClipRect = Vec4{ 0.0f, 0.0f, contentSize.width, contentSize.height };
    node._layoutState.computedBorderRect = node._layoutState.borderRect;
    node._layoutState.computedMarginRect = node._layoutState.marginRect;
    node._layoutState.computedZIndex = node._zIndex.value_or(0);

    if (node._parent != nullptr) {
        auto const& parentLayout = node._parent->_layoutState;

        node._layoutState.computedBorderRect.origin += parentLayout.computedBorderRect.origin;
        node._layoutState.computedMarginRect.origin += parentLayout.computedBorderRect.origin;

        if (node._position != NodePosition::Fixed) {
            node._layoutState.computedBorderRect.origin -= parentLayout.scrollPosition;
            node._layoutState.computedMarginRect.origin -= parentLayout.scrollPosition;
        }

        if (node._clipped == true) {
            node._layoutState.computedClipRect = parentLayout.computedClipRect;
        }

        if (node._layoutState.computedZIndex < parentLayout.computedZIndex) {
            node._layoutState.computedZIndex = parentLayout.computedZIndex;
        }

        if (node._layoutState.computedZIndex > parentLayout.computedZIndex) {
            _renderList[node._layoutState.computedZIndex].push_back(&node);
        }
    }

    auto const& borderEdge = node._layoutState.borderEdge;
    auto const innerBorderRect = Vec4{
        (node._layoutState.computedBorderRect.x + borderEdge.left),
        (node._layoutState.computedBorderRect.y + borderEdge.top),
        std::max(0.0f, (node._layoutState.computedBorderRect.width - borderEdge.left - borderEdge.right)),
        std::max(0.0f, (node._layoutState.computedBorderRect.height - borderEdge.top - borderEdge.bottom))
    };

    if (
        (node._overflowX == NodeOverflow::Hidden) ||
        (node._overflowX == NodeOverflow::Scroll)
    ) {
        auto newClipRect = node._layoutState.computedClipRect.getIntersection(innerBorderRect);
        node._layoutState.computedClipRect.x = newClipRect.x;
        node._layoutState.computedClipRect.width = newClipRect.width;
    }

    if (
        (node._overflowY == NodeOverflow::Hidden) ||
        (node._overflowY == NodeOverflow::Scroll)
    ) {
        auto newClipRect = node._layoutState.computedClipRect.getIntersection(innerBorderRect);
        node._layoutState.computedClipRect.y = newClipRect.y;
        node._layoutState.computedClipRect.height = newClipRect.height;
    }

    auto scrollMaxX = 0.0f;
    auto scrollMaxY = 0.0f;
    auto contentMinX = 0.0f;
    auto contentMinY = 0.0f;
    auto contentMaxX = node._layoutState.borderRect.width;
    auto contentMaxY = node._layoutState.borderRect.height;

    for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
        _updateNode(*child);

        if (child->_position == NodePosition::Fixed) {
            continue; // fixed children don't contribute to the content box or scrollable area
        }

        auto const& childLayout = child->_layoutState;

        auto childLeft   = childLayout.marginRect.x;
        auto childTop    = childLayout.marginRect.y;
        auto childRight  = childLayout.marginRect.getMaxX();
        auto childBottom = childLayout.marginRect.getMaxY();

        if (
            (child->_overflowX != NodeOverflow::Hidden) &&
            (child->_overflowX != NodeOverflow::Scroll)
        ) {
            childLeft  = std::min(childLeft,  (childLayout.borderRect.x + childLayout.contentRect.x));
            childRight = std::max(childRight, (childLayout.borderRect.x + childLayout.contentRect.getMaxX()));
        }
        if (
            (child->_overflowY != NodeOverflow::Hidden) &&
            (child->_overflowY != NodeOverflow::Scroll)
        ) {
            childTop    = std::min(childTop,    (childLayout.borderRect.y + childLayout.contentRect.y));
            childBottom = std::max(childBottom, (childLayout.borderRect.y + childLayout.contentRect.getMaxY()));
        }

        contentMinX = std::min(contentMinX, childLeft);
        contentMinY = std::min(contentMinY, childTop);
        contentMaxX = std::max(contentMaxX, childRight);
        contentMaxY = std::max(contentMaxY, childBottom);
        scrollMaxX = std::max(scrollMaxX, childRight);
        scrollMaxY = std::max(scrollMaxY, childBottom);
    }

    node._layoutState.contentRect = {
        contentMinX,
        contentMinY,
        (contentMaxX - contentMinX),
        (contentMaxY - contentMinY)
    };

    node._layoutState.scrollOverflow = {
        _SnapToPixelGrid(std::max(0.0f, (scrollMaxX - (node._layoutState.borderRect.width  - node._layoutState.borderEdge.right  - padding.right))), _scale),
        _SnapToPixelGrid(std::max(0.0f, (scrollMaxY - (node._layoutState.borderRect.height - node._layoutState.borderEdge.bottom - padding.bottom))), _scale)
    };

    node._layoutState.scrollPosition = {
        std::clamp(node._layoutState.scrollPosition.x, 0.0f, node._layoutState.scrollOverflow.x),
        std::clamp(node._layoutState.scrollPosition.y, 0.0f, node._layoutState.scrollOverflow.y)
    };
}

void Document::_renderNode(Node& node, Vec2 const& offset, int zIndex) {
    PROFILE

    if (node._visible == false) {
        return;
    }

    if (node._layoutState.computedZIndex != zIndex) {
        return;
    }

    auto nodeOffset = offset;
    auto nodeClipRect = (node._layoutState.computedClipRect * _scale);
    auto nodeBorderRect = (node._layoutState.computedBorderRect * _scale);
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
        auto parentClipRect = (node._parent->_layoutState.computedClipRect * _scale);
        parentClipRect.origin += nodeOffset;
        scissorRect = parentClipRect;
    }

    /* The node's own box is clipped by its ancestors only; its own overflow
       clip applies to its children. Inside a layer the ancestor clip is
       applied when the layer is composited. */
    auto boxScissorRect = scissorRect;

    if (layerNeeded) {
        auto& paint = node._paintState;
        auto nodeContentRect = (node._layoutState.contentRect * _scale);
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
            (paint.layerImage == nullptr) ||
            (paint.layerImage->getSize() != layerRect.size)
        ) {
            paint.layerImage = std::make_unique<Image>(layerRect.size);
        }

        nodeBorderRect.origin -= layerOffset;
        nodeClipRect.origin -= layerOffset;
        nodeOffset -= layerOffset;
        boxScissorRect = std::nullopt;

        _painter.beginPaint(
            ImagePaintTarget{
                .image = *paint.layerImage,
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
            .leftBorder = (node._layoutState.borderEdge.left * _scale),
            .topBorder = (node._layoutState.borderEdge.top * _scale),
            .rightBorder = (node._layoutState.borderEdge.right * _scale),
            .bottomBorder = (node._layoutState.borderEdge.bottom * _scale)
        };
        _painter.paint(borderShape, borderBrush, { .scissor = boxScissorRect });
    }

    auto const text = node._textState.text.get();

    if (
        (node._display == NodeDisplay::Text) &&
        (text != nullptr)
    ) {
        auto const editable = text->getEditable();
        auto const singleLine = (text->getMultiLine() == false) && (node._parent != nullptr);

        text->setMaxWidth(std::nullopt);
        text->setMaxHeight(std::nullopt);
        text->setWidth(singleLine ? std::nullopt : std::optional<float>(std::floorf(node._layoutState.textRect.width * _scale)));
        text->setHeight(std::floorf(node._layoutState.textRect.height * _scale));

        auto const textOrigin = (nodeBorderRect.origin + (node._layoutState.textRect.origin * _scale));
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
            auto const& parentLayout = node._parent->_layoutState;
            auto const inset = (node._layoutState.computedBorderRect.x - parentLayout.computedBorderRect.x - parentLayout.borderEdge.left);
            auto const visibleMaxX = ((parentLayout.computedBorderRect.getMaxX() - parentLayout.borderEdge.right - inset) * _scale) + nodeOffset.x;
            auto const visibleWidth = std::max(1.0f, (visibleMaxX - nodeBorderRect.x));
            auto& scrollX = node._textState.scrollX;

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
            node._textState.scrollX = 0.0f;
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
            auto const caretBrush = ColorBrush{ .color = node._textState.textColor };
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
        auto& paint = node._paintState;

        _painter.endPaint();

        auto const shape = QuadShape{ layerRect };
        auto const brush = ImageBrush{
            .image = &*paint.layerImage,
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
                    (paint.shadowImage == nullptr) ||
                    (paint.shadowImage->getSize() != shadowSize)
                ) {
                    paint.shadowImage = std::make_unique<Image>(shadowSize);
                    rebake = true;
                }

                if (
                    rebake ||
                    (paint.shadowImageShadow != shadowKey) ||
                    (paint.shadowImageRadius != radiusKey) ||
                    (paint.shadowImageScale != _scale) ||
                    (paint.shadowImageClip != clipKey)
                ) {
                    paint.shadowImageShadow = shadowKey;
                    paint.shadowImageRadius = radiusKey;
                    paint.shadowImageScale = _scale;
                    paint.shadowImageClip = clipKey;

                    _painter.beginPaint(
                        ImagePaintTarget{
                            .image = *paint.shadowImage,
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
                    .image = &*paint.shadowImage,
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
        nextOldActive->_inputState.isActive = false;
        nextOldActive = nextOldActive->_parent;
    }

    Node* nextNewActive = targetNode;
    while (nextNewActive != nullptr) {
        nextNewActive->_inputState.isActive = true;
        nextNewActive = nextNewActive->_parent;
    }

    _activeNode = targetNode;
    _needsUpdate = true;
}

} /* namespace Rocket */
