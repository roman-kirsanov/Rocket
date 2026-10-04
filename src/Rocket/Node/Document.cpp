/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */


#include <cassert>
#include <memory>
#include <cmath>
#include <ranges>
#include <algorithm>
#include <optional>
#include <yoga/Yoga.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Base/Time.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/Paint/Text.hpp>
#include <Rocket/Window/Clipboard.hpp>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/Node.hpp>

namespace Rocket {

static auto constexpr _selectionColor = Vec4{ 0.4f, 0.6f, 1.0f, 0.4f };
static auto constexpr _caretBlinkPeriod = 530ll;

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
    , _yogaConfig(nullptr)
    , _scale(1.0f)
    , _mouseState()
    , _hoverState()
    , _pressState()
    , _focusState()
    , _dragState()
    , _caretBlinkStart(GetTime())
    , _caretVisible(true)
    , _isUpdating(false)
    , _isFlushing(false)
    , _needsUpdate(true)
    , _needsRender(true)
    , _needsCursorUpdate(true)
    , _windowCursor(Cursor::Default)
    , _renderList()
    , _eventQueue()
{
    PROFILE

    _document = this;
    _scale = window.getScale();
    _yogaConfig = ::YGConfigNew();

    _windowSub.on(window.onEvent, [&](WindowEvent const& event) {
        _handleEvent(event);
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

    _scrollNode(_TargetScrollNode{ node }, wheel);
}

void Document::scrollNodeIntoView(Node& node) {
    PROFILE

    _scrollNodeIntoView(node);
}

void Document::focusNode(Node* targetNode) {
    PROFILE

    if (targetNode != nullptr) {
        _focusNode(_TargetFocusNode{ *targetNode });
    } else {
        _focusNode(_NoneFocusNode{});
    }

    _flushEvents();
}

void Document::update() {
    PROFILE

    _updateAll();
}

void Document::render() {
    PROFILE

    _renderAll();
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

void Document::_handleEvent(WindowEvent const& event) {
    PROFILE

    if (auto mouseMoveEvent = event.as<MouseMoveWindowEvent>()) {
        _handleMouseMoveEvent(*mouseMoveEvent);
    } else if (auto mouseEnterEvent = event.as<MouseEnterWindowEvent>()) {
        _handleMouseEnterEvent(*mouseEnterEvent);
    } else if (auto mouseExitEvent = event.as<MouseExitWindowEvent>()) {
        _handleMouseExitEvent(*mouseExitEvent);
    } else if (auto mouseDownEvent = event.as<MouseDownWindowEvent>()) {
        _handleMouseDownEvent(*mouseDownEvent);
    } else if (auto mouseUpEvent = event.as<MouseUpWindowEvent>()) {
        _handleMouseUpEvent(*mouseUpEvent);
    } else if (auto mouseWheelEvent = event.as<MouseWheelWindowEvent>()) {
        _handleMouseWheelEvent(*mouseWheelEvent);
    } else if (auto keyDownEvent = event.as<KeyDownWindowEvent>()) {
        _handleKeyDownEvent(*keyDownEvent);
    } else if (auto keyUpEvent = event.as<KeyUpWindowEvent>()) {
        _handleKeyUpEvent(*keyUpEvent);
    } else if (
        event.is<ResizeWindowEvent>() ||
        event.is<DPIChangeWindowEvent>()
    ) {
        _needsUpdate = true;
        _updateAll();
        _renderAll();
    } else if (event.is<PaintWindowEvent>()) {
        _updateAll();
        _renderAll();
    }
}

void Document::_handleMouseMoveEvent(MouseMoveWindowEvent const& event) {
    PROFILE

    _handleMousePosition(event.getPosition(), event.getModifiers(), true);
}

void Document::_handleMouseEnterEvent(MouseEnterWindowEvent const& event) {
    PROFILE

    _handleMousePosition(event.getPosition(), event.getModifiers(), true);
}

void Document::_handleMouseExitEvent(MouseExitWindowEvent const& event) {
    PROFILE

    _handleMousePosition(event.getPosition(), event.getModifiers(), false);
}

void Document::_handleMouseDownEvent(MouseDownWindowEvent const& event) {
    PROFILE

    if (_mouseState.down == true) return;

    _mouseState.down = true;
    _mouseState.mouse = event.getMouse();
    _mouseState.position = event.getPosition();
    _mouseState.downPosition = event.getPosition();
    _mouseState.modifiers = event.getModifiers();
    _mouseState.clickCount = event.getClickCount();
    _mouseState.defaultPrevented = false;

    auto const focusedNode = _focusState.focusedNode;

    _pressNode();
    _triggerMouseDown(event.getMouse(), event.getModifiers(), _mouseState.defaultPrevented);

    if (
        event.getMouse() == Mouse::LeftButton ||
        event.getMouse() == Mouse::RightButton
    ) {
        if (_mouseState.defaultPrevented == false) {
            _focusNode(_PressFocusNode{});
        }
    }

    _pressText(_focusState.focusedNode != focusedNode);
    _updateAll();
}

void Document::_handleMouseUpEvent(MouseUpWindowEvent const& event) {
    PROFILE

    if (_mouseState.down == false) return;
    if (_mouseState.mouse != event.getMouse()) return;

    _mouseState.down = false;
    _mouseState.position = event.getPosition();
    _mouseState.modifiers = event.getModifiers();

    _releaseText();
    _triggerMouseUp(event.getMouse(), event.getModifiers());
    _clickNode();
    _releaseNode();
    _dropNode();
    _hoverNode();
    _updateAll();
}

void Document::_handleMouseWheelEvent(MouseWheelWindowEvent const& event) {
    PROFILE

    auto const wheel = _convertPoint(event.getPosition());
    auto defaultPrevented = false;

    _triggerMouseWheel(wheel, event.getModifiers(), defaultPrevented);

    if (defaultPrevented == false) {
        _scrollNode(_HoverScrollNode{}, wheel);
    }

    _updateAll();
}

void Document::_handleKeyDownEvent(KeyDownWindowEvent const& event) {
    PROFILE

    auto defaultPrevented = false;

    _triggerKeyDown(event.getKey(), event.getModifiers(), event.getInput(), defaultPrevented);

    if (defaultPrevented == false) {
        _processKey(event.getKey(), event.getModifiers(), event.getInput());
    }

    _updateAll();
}

void Document::_handleKeyUpEvent(KeyUpWindowEvent const& event) {
    PROFILE

    _triggerKeyUp(event.getKey(), event.getModifiers());
    _updateAll();
}

void Document::_handleMousePosition(Vec2 const& position, KeyModifiers const& modifiers, bool inside) {
    PROFILE

    _mouseState.inside = inside;
    _mouseState.position = position;
    _mouseState.modifiers = modifiers;
    _hoverNode();
    _dragNode();
    _dragText();
    _updateAll();
}

void Document::_hoverNode() {
    PROFILE

    static thread_local auto hoverPath = std::vector<Node*>();

    if (_mouseState.down == true) return;

    hoverPath.clear();

    auto const modifiers = _mouseState.modifiers;
    auto const position = _convertPoint(_mouseState.position);

    auto hoverNode = (_mouseState.inside == true) ? _findNodeAtPosition(position) : nullptr;
    if (hoverNode != nullptr) {
        hoverNode->getPathToRoot(hoverPath);
    }

    auto const positionChanged = (_hoverState.mousePosition != _mouseState.position);
    auto const hoverNodeChanged = (_hoverState.hoverNode != hoverNode);

    _hoverState.mousePosition = _mouseState.position;

    if (
        hoverNodeChanged ||
        _hoverState.hoverPath != hoverPath
    ) {
        for (auto node : _hoverState.hoverPath) {
            if (std::ranges::contains(hoverPath, node) == false) {
                node->_isHover = false;
                _queueEvent(
                    std::make_unique<MouseExitNodeEvent>(*node, position, modifiers)
                );
            }
        }

        for (auto node : std::views::reverse(hoverPath)) {
            if (std::ranges::contains(_hoverState.hoverPath, node) == false) {
                node->_isHover = true;
                _queueEvent(
                    std::make_unique<MouseEnterNodeEvent>(*node, position, modifiers)
                );
            }
        }

        _hoverState.hoverNode = hoverNode;
        _hoverState.hoverPath = hoverPath;
        _needsCursorUpdate = true;
        _needsRender = true;
    }

    if (positionChanged || hoverNodeChanged) {
        if (hoverNode != nullptr) {
            _queueEvent(
                std::make_unique<MouseMoveNodeEvent>(*hoverNode, position, modifiers)
            );
        }
    }
}

void Document::_pressNode() {
    PROFILE

    if (_hoverState.hoverNode != nullptr) {
        _pressState.pressNode = _hoverState.hoverNode;
        _pressState.pressNode->getPathToRoot(_pressState.pressPath);

        if (_mouseState.mouse == Mouse::LeftButton) {
            for (auto node : _pressState.pressPath) {
                node->_isActive = true;
            }

            _needsRender = true;
        }
    }
}

void Document::_releaseNode() {
    PROFILE

    if (_pressState.pressPath.empty() == false) {
        if (_mouseState.mouse == Mouse::LeftButton) {
            for (auto node : _pressState.pressPath) {
                node->_isActive = false;
            }

            _needsRender = true;
        }

        _pressState.pressNode = nullptr;
        _pressState.pressPath.clear();
    }
}

void Document::_focusNode(_FocusNodeVariant const& variant) {
    PROFILE

    static thread_local auto focusPath = std::vector<Node*>();

    auto focusNode = variant.match(
        [&](_NoneFocusNode const&) { return (Node*)nullptr; },
        [&](_PressFocusNode const&) { return _pressState.pressNode; },
        [&](_TargetFocusNode const& v) { return &v.node; }
    );

    for (; focusNode != nullptr; focusNode = focusNode->_parent) {
        if (_isNodeFocusable(*focusNode) || _isNodeEditable(*focusNode)) break;
    }

    if (focusNode != _focusState.focusedNode) {
        auto blurNode = _focusState.focusedNode;

        if (focusNode != nullptr) {
            focusNode->_isFocused = true;
            focusNode->getPathToRoot(focusPath);
        } else {
            focusPath.clear();
        }

        if (blurNode != nullptr) {
            blurNode->_isFocused = false;

            if (
                _isNodeEditable(*blurNode) &&
                (blurNode->_firstChild->_textObject != nullptr)
            ) {
                blurNode->_firstChild->_textObject->setEditable(false);
                blurNode->_firstChild->_invalidateText();
                blurNode->_invalidateLayout();
            }
        }

        if (
            (focusNode != nullptr) &&
            _isNodeEditable(*focusNode)
        ) {
            if (focusNode->_firstChild->_textObject == nullptr) {
                focusNode->_firstChild->_textObject = std::make_unique<Text>();
            }
            focusNode->_firstChild->_textObject->setEditable(true);
            focusNode->_firstChild->_invalidateText();
            focusNode->_invalidateLayout();
        }

        for (auto node : _focusState.focusedPath) {
            if (std::ranges::contains(focusPath, node) == false) {
                node->_isFocusedWithin = false;
            }
        }

        for (auto node : focusPath) {
            if (std::ranges::contains(_focusState.focusedPath, node) == false) {
                node->_isFocusedWithin = true;
            }
        }

        _focusState.focusedNode = focusNode;
        _focusState.focusedPath = focusPath;
        _needsRender = true;

        _restartCaretBlink();

        if (blurNode != nullptr) {
            _queueEvent(
                std::make_unique<BlurNodeEvent>(*blurNode)
            );
        }

        if (focusNode != nullptr) {
            _queueEvent(
                std::make_unique<FocusNodeEvent>(*focusNode)
            );
        }
    }
}

void Document::_scrollNode(_ScrollNodeVariant const& variant, Vec2 const& wheel) {
    PROFILE

    static thread_local auto scrollPath = std::vector<Node*>();

    auto scrollNode = variant.match(
        [&](_HoverScrollNode const&) { return _hoverState.hoverNode; },
        [&](_TargetScrollNode const& v) { return &v.node; }
    );

    if (scrollNode == nullptr) return;

    Node* xOverflowNode = nullptr;
    Node* yOverflowNode = nullptr;

    scrollNode->getPathFromRoot(scrollPath);

    for (auto n : scrollPath) {
        if (
            n->_scrollOverflow.x > 0.0f &&
            n->getOverflowX() == NodeOverflow::Scroll
        ) {
            xOverflowNode = n;
        }

        if (
            n->_scrollOverflow.y > 0.0f &&
            n->getOverflowY() == NodeOverflow::Scroll
        ) {
            yOverflowNode = n;
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

void Document::_scrollNodeIntoView(Node& node) {
    PROFILE

    auto container = static_cast<Node*>(nullptr);

    for (auto ancestor = node._parent; ancestor != nullptr; ancestor = ancestor->_parent) {
        if (
            (ancestor->getOverflowX() == NodeOverflow::Scroll) ||
            (ancestor->getOverflowY() == NodeOverflow::Scroll)
        ) {
            container = ancestor;
            break;
        }
    }

    if (container == nullptr) {
        return;
    }

    auto const& nodeRect = node._computedBorderRectInDocument;
    auto const& containerRect = container->_computedBorderRectInDocument;
    auto const& borderEdge = container->_computedBorderEdge;

    auto const scrollAxis = [&](float nodeMin, float nodeSize, float viewMin, float viewSize, float& position, float overflow) {
        auto delta = 0.0f;

        if (
            (nodeSize > viewSize) ||
            (nodeMin < viewMin)
        ) {
            delta = (nodeMin - viewMin);
        } else if ((nodeMin + nodeSize) > (viewMin + viewSize)) {
            delta = ((nodeMin + nodeSize) - (viewMin + viewSize));
        }

        if (delta != 0.0f) {
            position = _SnapToPixelGrid(std::clamp((position + delta), 0.0f, overflow), _scale);
            _needsUpdate = true;
        }
    };

    if (container->getOverflowX() == NodeOverflow::Scroll) {
        scrollAxis(
            nodeRect.x, nodeRect.width,
            (containerRect.x + borderEdge.left), std::max(0.0f, (containerRect.width - borderEdge.left - borderEdge.right)),
            container->_scrollPosition.x, container->_scrollOverflow.x
        );
    }

    if (container->getOverflowY() == NodeOverflow::Scroll) {
        scrollAxis(
            nodeRect.y, nodeRect.height,
            (containerRect.y + borderEdge.top), std::max(0.0f, (containerRect.height - borderEdge.top - borderEdge.bottom)),
            container->_scrollPosition.y, container->_scrollOverflow.y
        );
    }
}

void Document::_dragNode() {
    PROFILE

    if (_mouseState.down == false) return;
    if (_mouseState.mouse != Mouse::LeftButton) return;

    auto const modifiers = _mouseState.modifiers;
    auto const position = _convertPoint(_mouseState.position);
    auto const translate = (position - _convertPoint(_mouseState.downPosition));

    if (_dragState.dragNode == nullptr) {
        if (
            std::fabsf(translate.x) > 2.0f ||
            std::fabsf(translate.y) > 2.0f
        ) {
            if (_pressState.pressNode != nullptr) {
                _dragState.dragNode = _pressState.pressNode;
                _dragState.mousePosition = _mouseState.position;

                _queueEvent(
                    std::make_unique<MouseBeginDragNodeEvent>(*_dragState.dragNode, position, translate, modifiers)
                );

                _queueEvent(
                    std::make_unique<MouseDragNodeEvent>(*_dragState.dragNode, position, translate, modifiers)
                );
            }
        }
    } else if (_dragState.mousePosition != _mouseState.position) {
        _dragState.mousePosition = _mouseState.position;
        _queueEvent(
            std::make_unique<MouseDragNodeEvent>(*_dragState.dragNode, position, translate, modifiers)
        );
    }
}

void Document::_dropNode() {
    PROFILE

    if (_dragState.dragNode != nullptr) {
        auto const modifiers = _mouseState.modifiers;
        auto const position = _convertPoint(_mouseState.position);
        auto const translate = (position - _convertPoint(_mouseState.downPosition));

        _queueEvent(
            std::make_unique<MouseEndDragNodeEvent>(*_dragState.dragNode, position, translate, modifiers)
        );

        _dragState.dragNode = nullptr;
    }
}

void Document::_clickNode() {
    PROFILE

    static thread_local auto clickPath = std::vector<Node*>();

    if (_mouseState.mouse != Mouse::LeftButton) return;
    if (_pressState.pressNode == nullptr) return;
    if (_dragState.dragNode != nullptr) return;

    auto const modifiers = _mouseState.modifiers;
    auto const position = _convertPoint(_mouseState.position);

    if (auto clickNode = _findNodeAtPosition(position)) {
        clickNode->getPathToRoot(clickPath);

        for (auto node : clickPath) {
            if (std::ranges::contains(_pressState.pressPath, node)) {
                _queueEvent(
                    std::make_unique<MouseClickNodeEvent>(*node, position, modifiers)
                );
                break;
            }
        }
    }
}

void Document::_pressText(bool focusChanged) {
    PROFILE

    auto const leftButton = (_mouseState.mouse == Mouse::LeftButton);
    auto const rightButton = (_mouseState.mouse == Mouse::RightButton);

    if (_mouseState.defaultPrevented == true) return;
    if ((leftButton == false) && (rightButton == false)) return;

    /* like the web, a right press leaves an existing caret alone and only
       places it when the press is what focused the editable */
    if (rightButton && (focusChanged == false)) return;

    if (auto inputState = _ensureInputState()) {
        auto const position = _getTextLocalPosition(*inputState, _convertPoint(_mouseState.position));

        if (leftButton) {
            inputState->textObject.mouseDown(
                position,
                _mouseState.modifiers.shift,
                _mouseState.clickCount
            );
        } else {
            inputState->textObject.mouseDown(position, false, 1);
            inputState->textObject.mouseUp(position);
        }

        _restartCaretBlink();
        _needsRender = true;
    }
}

void Document::_dragText() {
    PROFILE

    if (_mouseState.down == false) return;
    if (_mouseState.mouse != Mouse::LeftButton) return;
    if (_mouseState.defaultPrevented == true) return;

    if (auto inputState = _ensureInputState()) {
        auto const position = _convertPoint(_mouseState.position);

        inputState->textObject.mouseMove(
            _getTextLocalPosition(*inputState, position)
        );

        _restartCaretBlink();
        _needsRender = true;
    }
}

void Document::_releaseText() {
    PROFILE

    if (_mouseState.mouse != Mouse::LeftButton) return;
    if (_mouseState.defaultPrevented == true) return;

    if (auto inputState = _ensureInputState()) {
        auto const position = _convertPoint(_mouseState.position);

        inputState->textObject.mouseUp(
            _getTextLocalPosition(*inputState, position)
        );

        _needsRender = true;
    }
}

void Document::_focusNext(bool reverse) {
    PROFILE

    static thread_local auto candidates = std::vector<Node*>();

    candidates.clear();

    auto const collect = [&](Node& node, auto&& collect) -> void {
        if (node._visible == false) {
            return;
        }
        if ((&node != this) && (_isNodeFocusable(node) || _isNodeEditable(node))) {
            candidates.push_back(&node);
        }
        for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
            collect(*child, collect);
        }
    };

    collect(*this, collect);

    if (candidates.empty()) {
        return;
    }

    auto const count = (std::int64_t)candidates.size();
    auto current = (std::int64_t)-1;

    for (auto i = 0ll; i < count; i++) {
        if (candidates[(std::size_t)i] == _focusState.focusedNode) {
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

    _focusNode(_TargetFocusNode{ *candidates[(std::size_t)next] });

    if (auto inputState = _ensureInputState()) {
        inputState->textObject.selectAll();
    }

    _needsRender = true;
}

void Document::_processKey(Key key, KeyModifiers const& modifiers, std::string const& input) {
    PROFILE

    auto const focusedNode = _focusState.focusedNode;
    auto consumed = false;

    if (
        (focusedNode != nullptr) &&
        (focusedNode->_keyEvents == true)
    ) {
        consumed = _inputText(key, modifiers, input);
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

bool Document::_inputText(Key key, KeyModifiers const& modifiers, std::string const& input) {
    PROFILE

    if (auto inputState = _ensureInputState()) {
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

        _needsRender = true;
        _restartCaretBlink();

        auto const& contentAfter = inputState->textObject.getString();
        if (contentAfter != contentBefore) {
            inputState->textNode.setContent(contentAfter);
            _queueEvent(
                std::make_unique<InputNodeEvent>(inputState->boxNode, contentAfter)
            );
        }

        return true;
    }

    return false;
}

std::optional<Document::_InputState> Document::_ensureInputState() {
    PROFILE

    if (
        _focusState.focusedNode != nullptr &&
        _isNodeEditable(*_focusState.focusedNode)
    ) {
        auto& textNode = *_focusState.focusedNode->_firstChild;

        /* the text child may have been swapped since focus: adopt it as the surface */
        if (textNode._textObject == nullptr) {
            textNode._textObject = std::make_unique<Text>();
            textNode._textObject->setString(textNode._content.value_or(""));
            textNode._invalidateText();
            textNode._invalidateLayout();
        }

        if (textNode._textObject->getEditable() == false) {
            textNode._textObject->setEditable(true);
            textNode._invalidateText();
            textNode._invalidateLayout();
        }

        return _InputState{ *_focusState.focusedNode, textNode, *textNode._textObject };
    } else {
        return std::nullopt;
    }
}

void Document::_restartCaretBlink() {
    PROFILE

    _caretBlinkStart = GetTime();
    _caretVisible = true;
}

Vec2 Document::_convertPoint(Vec2 const& point) const {
    PROFILE

    return (point * (_window.getScale() / _scale));
}

void Document::_queueEvent(std::unique_ptr<NodeEvent> event) {
    PROFILE

    _eventQueue.push_back(std::move(event));
}

void Document::_flushEvents() {
    PROFILE

    if (_isFlushing == true) return;
    else _isFlushing = true;

    for (auto i = 0uz; i < _eventQueue.size(); i++) {
        auto const event = std::move(_eventQueue[i]);

        if (event != nullptr) {
            event->getNode().dispatchEvent(*event);
        }
    }

    _eventQueue.clear();
    _isFlushing = false;
}

void Document::_updateLayout() {
    PROFILE

    auto const size = getSize();

    ::YGConfigSetPointScaleFactor((::YGConfig*)_yogaConfig, _scale);
    ::YGNodeStyleSetWidth((::YGNode*)_layoutNode, size.width);
    ::YGNodeStyleSetHeight((::YGNode*)_layoutNode, size.height);
    ::YGNodeCalculateLayout((::YGNode*)_layoutNode, size.width, size.height, YGDirectionLTR);
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
        node._computedBorderRect.origin.x += _SnapToPixelGrid(
            node._transform->translateX.match(
                [](PixelValue const& pixel) { return pixel.value; },
                [&](PercentValue const& percent) { return ((percent.value / 100.0f) * node._computedBorderRect.width); }
            ),
            _scale
        );

        node._computedBorderRect.origin.y += _SnapToPixelGrid(
            node._transform->translateY.match(
                [](PixelValue const& pixel) { return pixel.value; },
                [&](PercentValue const& percent) { return ((percent.value / 100.0f) * node._computedBorderRect.height); }
            ),
            _scale
        );
    }

    node._computedMarginRect = {
        (node._computedBorderRect.x - margin.left),
        (node._computedBorderRect.y - margin.top),
        (node._computedBorderRect.width + margin.left + margin.right),
        (node._computedBorderRect.height + margin.top + margin.bottom)
    };

    if (
        node._display == NodeDisplay::Text &&
        node._textNode != nullptr
    ) {
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

void Document::_updateCursor() {
    PROFILE

    if (_needsCursorUpdate == false) return;
    else _needsCursorUpdate = false;

    auto cursor = Cursor::Default;

    for (auto node : _hoverState.hoverPath) {
        if (node->_cursor.has_value()) {
            cursor = node->_cursor.value();
            break;
        }
    }

    if (_windowCursor != cursor) {
        _windowCursor = cursor;
        _window.setCursor(cursor);
    }
}

void Document::_updateAll() {
    PROFILE

    if (_isUpdating == true) return;
    else _isUpdating = true;

    for (;;) {
        if (_needsUpdate == true) {
            _needsUpdate = false;
            _needsRender = true;

            _renderList.clear();

            _updateLayout();
            _updateNode(*this);
            _hoverNode();

            _renderList[_computedZIndex].push_back(this);
        }

        if (_eventQueue.empty() || _isFlushing) break;
        else _flushEvents();
    }

    _updateCursor();

    _isUpdating = false;
}

void Document::_renderNode(Node& node, Vec2 const& offset, int zIndex) {
    PROFILE

    if (node._visible == false) return;
    if (node._computedZIndex != zIndex) return;

    auto const info = _beginNodeRender(node, offset);
    if (info.empty == true) return;

    _renderNodeBackground(node, info);
    _renderNodeBorder(node, info);
    _renderNodeText(node, info);

    for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
        _renderNode(*child, info.offset, zIndex);
    }

    _renderNodeForeground(node, info);
    _endNodeRender(node, info);
}

Document::_RenderInfo Document::_beginNodeRender(Node& node, Vec2 const& offset) {
    PROFILE

    auto const scaledRadius = [this](std::optional<float> const& radius) -> std::optional<float> {
        return radius.has_value() ? std::optional<float>{ (*radius * _scale) } : std::nullopt;
    };

    auto info = _RenderInfo{
        .empty = false,
        .offset = offset,
        .borderShape = QuadShape{
            .rect = (node._computedBorderRectInDocument * _scale),
            .borderRadius = scaledRadius(node._borderRadius),
            .borderTopLeftRadius = scaledRadius(node._borderTopLeftRadius),
            .borderTopRightRadius = scaledRadius(node._borderTopRightRadius),
            .borderBottomLeftRadius = scaledRadius(node._borderBottomLeftRadius),
            .borderBottomRightRadius = scaledRadius(node._borderBottomRightRadius)
        },
        .clipRect = (node._computedClipRectInDocument * _scale)
    };

    info.borderShape.rect.origin += offset;
    info.clipRect.origin += offset;

    if (
        (node._clipped == true) &&
        (node._parent != nullptr)
    ) {
        auto parentClipRect = (node._parent->_computedClipRectInDocument * _scale);
        parentClipRect.origin += offset;
        info.scissorRect = parentClipRect;
        info.compositeScissor = parentClipRect;
    }

    auto const needsLayer = (
        node._opacity.value_or(1.0f) < 1.0f ||
        node._shadow.has_value()
    );

    if (needsLayer) {
        auto const contentRect = (node._computedContentRect * _scale);
        auto const contentOffset = Vec2{
            std::min(0.0f, contentRect.x),
            std::min(0.0f, contentRect.y)
        };

        auto const layerOffset = (info.borderShape.rect.origin + contentOffset);
        auto const layerRect = Vec4{
            (info.borderShape.rect.x + contentOffset.width),
            (info.borderShape.rect.y + contentOffset.height),
            std::max(info.borderShape.rect.width, contentRect.width),
            std::max(info.borderShape.rect.height, contentRect.height)
        };

        if (
            (layerRect.width < 1.0f) ||
            (layerRect.height < 1.0f)
        ) {
            return { .empty = true };
        }

        if (
            (node._layerImage == nullptr) ||
            (node._layerImage->getSize() != layerRect.size)
        ) {
            node._layerImage = std::make_unique<Image>(layerRect.size);
        }

        info.borderShape.rect.origin -= layerOffset;
        info.clipRect.origin -= layerOffset;
        info.offset -= layerOffset;
        info.scissorRect = std::nullopt;
        info.layerRect = layerRect;

        _painter.beginPaint(
            ImagePaintTarget{
                .image = *node._layerImage,
                .clearColor = COLOR_TRANSPARENT
            }
        );
    }

    return info;
}

void Document::_endNodeRender(Node& node, _RenderInfo const& info) {
    PROFILE

    if (info.layerRect.has_value()) {
        _painter.endPaint();

        auto const shape = QuadShape{ *info.layerRect };
        auto const brush = ImageBrush{
            .image = &*node._layerImage,
            .positionX = ImagePosition::Start,
            .positionY = ImagePosition::Start,
            .filterMag = ImageFilter::Nearest,
            .filterMin = ImageFilter::Nearest
        };

        _renderNodeShadow(node, info, brush);

        _painter.paint(shape, brush, {
            .scissor = info.compositeScissor,
            .opacity = node._opacity
        });
    }
}

void Document::_renderNodeBackground(Node& node, _RenderInfo const& info) {
    PROFILE

    if (node._background) {
        _painter.paint(info.borderShape, *node._background, { .scissor = info.scissorRect });
    }
}

void Document::_renderNodeBorder(Node& node, _RenderInfo const& info) {
    PROFILE

    if (node._border) {
        auto const borderShape = QuadOutlineShape{
            .rect = info.borderShape.rect,
            .borderRadius = info.borderShape.borderRadius,
            .borderTopLeftRadius = info.borderShape.borderTopLeftRadius,
            .borderTopRightRadius = info.borderShape.borderTopRightRadius,
            .borderBottomLeftRadius = info.borderShape.borderBottomLeftRadius,
            .borderBottomRightRadius = info.borderShape.borderBottomRightRadius,
            .leftBorder = (node._computedBorderEdge.left * _scale),
            .topBorder = (node._computedBorderEdge.top * _scale),
            .rightBorder = (node._computedBorderEdge.right * _scale),
            .bottomBorder = (node._computedBorderEdge.bottom * _scale)
        };
        _painter.paint(borderShape, *node._border, { .scissor = info.scissorRect });
    }
}

void Document::_renderNodeForeground(Node& node, _RenderInfo const& info) {
    PROFILE

    if (node._foreground) {
        _painter.paint(info.borderShape, *node._foreground, { .scissor = info.scissorRect });
    }
}

void Document::_renderNodeText(Node& node, _RenderInfo const& info) {
    PROFILE

    if (node._display != NodeDisplay::Text) return;
    if (node._textObject == nullptr) return;

    auto const text = node._textObject.get();

    auto const editable = text->getEditable();
    auto const singleLine = (text->getMultiLine() == false) && (node._parent != nullptr);

    text->setMaxWidth(std::nullopt);
    text->setMaxHeight(std::nullopt);
    text->setWidth(singleLine ? std::nullopt : std::optional<float>(std::floorf(node._computedTextRect.width * _scale)));
    text->setHeight(std::floorf(node._computedTextRect.height * _scale));

    auto const textOrigin = (info.borderShape.rect.origin + (node._computedTextRect.origin * _scale));
    auto textClipRect = info.clipRect;
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
        auto const visibleMaxX = ((node._parent->_computedBorderRectInDocument.getMaxX() - node._parent->_computedBorderEdge.right - inset) * _scale) + info.offset.x;
        auto const visibleWidth = std::max(1.0f, (visibleMaxX - info.borderShape.rect.x));
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

        auto const clipMinX = std::max(info.clipRect.x, info.borderShape.rect.x);
        auto const clipMaxX = std::min(info.clipRect.getMaxX(), (info.borderShape.rect.x + visibleWidth));
        textClipRect = { clipMinX, info.clipRect.y, std::max(0.0f, (clipMaxX - clipMinX)), info.clipRect.height };
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

void Document::_renderNodeShadow(Node& node, _RenderInfo const& info, ImageBrush const& brush) {
    PROFILE

    auto const& layerRect = *info.layerRect;

    if (node._shadow.has_value() == false) return;

    auto const& shadow = *node._shadow;
    auto const shadowColor = shadow.color.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 1.0f });

    if (shadowColor.alpha <= 0.0f) return;

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

    auto const clipKey = info.clipRect.getIntersection(
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
        .scissor = info.compositeScissor,
        .opacity = node._opacity
    });
}

void Document::_renderAll() {
    PROFILE

    if (
        (_focusState.focusedNode != nullptr) &&
        (_focusState.focusedNode->_firstChild != nullptr) &&
        (_focusState.focusedNode->_firstChild->_textObject != nullptr) &&
        _isNodeEditable(*_focusState.focusedNode)
    ) {
        auto const& text = *_focusState.focusedNode->_firstChild->_textObject;
        auto const phase = ((GetTime() - _caretBlinkStart) / _caretBlinkPeriod);
        auto const visible = text.isSelectedRange() || ((phase % 2) == 0);

        if (_caretVisible != visible) {
            _caretVisible = visible;
            _needsRender = true;
        }
    }

    if (_needsRender == false) return;
    else _needsRender = false;

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

void Document::_triggerMouseWheel(Vec2 const& delta, KeyModifiers const& modifiers, bool& defaultPrevented) {
    PROFILE

    _flushEvents();

    auto targetNode = (
        _hoverState.hoverNode != nullptr
            ? _hoverState.hoverNode
            : this
    );

    auto const event = MouseWheelNodeEvent(*targetNode, delta, modifiers);
    targetNode->dispatchEvent(event);
    defaultPrevented = event.isDefaultPrevented();
}

void Document::_triggerMouseDown(Mouse mouse, KeyModifiers const& modifiers, bool& defaultPrevented) {
    PROFILE

    _flushEvents();

    auto const position = _convertPoint(_mouseState.position);

    auto targetNode = (
        _pressState.pressNode != nullptr
            ? _pressState.pressNode
            : this
    );

    auto const event = MouseDownNodeEvent(*targetNode, mouse, position, modifiers, _mouseState.clickCount);
    targetNode->dispatchEvent(event);
    defaultPrevented = event.isDefaultPrevented();
}

void Document::_triggerMouseUp(Mouse mouse, KeyModifiers const& modifiers) {
    PROFILE

    _flushEvents();

    auto const position = _convertPoint(_mouseState.position);

    auto targetNode = (
        _pressState.pressNode != nullptr
            ? _pressState.pressNode
            : this
    );

    targetNode->dispatchEvent(
        MouseUpNodeEvent(*targetNode, mouse, position, modifiers)
    );
}

void Document::_triggerKeyDown(Key key, KeyModifiers const& modifiers, std::string const& input, bool& defaultPrevented) {
    PROFILE

    _flushEvents();

    auto targetNode = (
        (_focusState.focusedNode != nullptr && _focusState.focusedNode->_keyEvents == true)
            ? _focusState.focusedNode
            : this
    );

    auto const event = KeyDownNodeEvent(*targetNode, key, modifiers, input);
    targetNode->dispatchEvent(event);
    defaultPrevented = event.isDefaultPrevented();
}

void Document::_triggerKeyUp(Key key, KeyModifiers const& modifiers) {
    PROFILE

    _flushEvents();

    auto targetNode = (
        (_focusState.focusedNode != nullptr && _focusState.focusedNode->_keyEvents == true)
            ? _focusState.focusedNode
            : this
    );

    targetNode->dispatchEvent(
        KeyUpNodeEvent(*targetNode, key, modifiers)
    );
}

} /* namespace Rocket */
