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

enum class _TextCommand {
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    MoveWordLeft,
    MoveWordRight,
    MoveLineStart,
    MoveLineEnd,
    MoveDocumentStart,
    MoveDocumentEnd,
    DeleteBackward,
    DeleteForward,
    DeleteWordBackward,
    DeleteWordForward,
    DeleteLineBackward,
    DeleteLineForward,
    InsertLineBreak,
    InsertTab,
    Undo,
    Redo,
    SelectAll,
    Copy,
    Cut,
    Paste
};

struct _TextCommandMatch {
    _TextCommand command;
    bool extendSelection;
};

static std::optional<_TextCommandMatch> _ResolveTextCommand(std::string const& keycode, KeyModifiers const& modifiers) {
    PROFILE

    auto const shift = modifiers.shift;
    auto const match = [&](_TextCommand command, bool extendSelection = false) {
        return std::optional<_TextCommandMatch>(_TextCommandMatch{ command, extendSelection });
    };

    /* Enter and Tab are commands everywhere: typed text never carries them */
    if (keycode == Keycode::Enter) return match(_TextCommand::InsertLineBreak);
    if (keycode == Keycode::Tab) return match(_TextCommand::InsertTab);

#ifdef __APPLE__
    auto const command = modifiers.meta;
    auto const option = modifiers.alt;
    auto const controlOnly = modifiers.control && (modifiers.meta == false) && (modifiers.alt == false);

    if (controlOnly) {
        if (keycode == Keycode::A) return match(_TextCommand::MoveLineStart, shift);
        if (keycode == Keycode::E) return match(_TextCommand::MoveLineEnd, shift);
        if (keycode == Keycode::F) return match(_TextCommand::MoveRight, shift);
        if (keycode == Keycode::B) return match(_TextCommand::MoveLeft, shift);
        if (keycode == Keycode::N) return match(_TextCommand::MoveDown, shift);
        if (keycode == Keycode::P) return match(_TextCommand::MoveUp, shift);
        if (keycode == Keycode::D) return match(_TextCommand::DeleteForward);
        if (keycode == Keycode::H) return match(_TextCommand::DeleteBackward);
        if (keycode == Keycode::K) return match(_TextCommand::DeleteLineForward);
    }

    if (keycode == Keycode::ArrowLeft) return match(command ? _TextCommand::MoveLineStart : option ? _TextCommand::MoveWordLeft : _TextCommand::MoveLeft, shift);
    if (keycode == Keycode::ArrowRight) return match(command ? _TextCommand::MoveLineEnd : option ? _TextCommand::MoveWordRight : _TextCommand::MoveRight, shift);
    if (keycode == Keycode::ArrowUp) return match(command ? _TextCommand::MoveDocumentStart : _TextCommand::MoveUp, shift);
    if (keycode == Keycode::ArrowDown) return match(command ? _TextCommand::MoveDocumentEnd : _TextCommand::MoveDown, shift);
    if (keycode == Keycode::Home) return match(_TextCommand::MoveLineStart, shift);
    if (keycode == Keycode::End) return match(_TextCommand::MoveLineEnd, shift);
    if (keycode == Keycode::Backspace) return match(command ? _TextCommand::DeleteLineBackward : option ? _TextCommand::DeleteWordBackward : _TextCommand::DeleteBackward);
    if (keycode == Keycode::Delete) return match(command ? _TextCommand::DeleteLineForward : option ? _TextCommand::DeleteWordForward : _TextCommand::DeleteForward);

    if (command) {
        if (keycode == Keycode::Z) return match(shift ? _TextCommand::Redo : _TextCommand::Undo);
        if (keycode == Keycode::A) return match(_TextCommand::SelectAll);
        if (keycode == Keycode::C) return match(_TextCommand::Copy);
        if (keycode == Keycode::X) return match(_TextCommand::Cut);
        if (keycode == Keycode::V) return match(_TextCommand::Paste);
    }
#else
    auto const control = modifiers.control;

    if (keycode == Keycode::ArrowLeft) return match(control ? _TextCommand::MoveWordLeft : _TextCommand::MoveLeft, shift);
    if (keycode == Keycode::ArrowRight) return match(control ? _TextCommand::MoveWordRight : _TextCommand::MoveRight, shift);
    if (keycode == Keycode::ArrowUp) return match(_TextCommand::MoveUp, shift);
    if (keycode == Keycode::ArrowDown) return match(_TextCommand::MoveDown, shift);
    if (keycode == Keycode::Home) return match(control ? _TextCommand::MoveDocumentStart : _TextCommand::MoveLineStart, shift);
    if (keycode == Keycode::End) return match(control ? _TextCommand::MoveDocumentEnd : _TextCommand::MoveLineEnd, shift);
    if (keycode == Keycode::Backspace) return match(control ? _TextCommand::DeleteWordBackward : _TextCommand::DeleteBackward);
    if (keycode == Keycode::Delete) return match(control ? _TextCommand::DeleteWordForward : _TextCommand::DeleteForward);

    if (control) {
        if (keycode == Keycode::Z) return match(shift ? _TextCommand::Redo : _TextCommand::Undo);
        if (keycode == Keycode::Y) return match(_TextCommand::Redo);
        if (keycode == Keycode::A) return match(_TextCommand::SelectAll);
        if (keycode == Keycode::C) return match(_TextCommand::Copy);
        if (keycode == Keycode::X) return match(_TextCommand::Cut);
        if (keycode == Keycode::V) return match(_TextCommand::Paste);
    }
#endif

    return std::nullopt;
}

static float _SnapToPixelGrid(float value, float scale) {
    return (std::roundf(value * scale) / scale);
}

static float _SnapBorderToPixelGrid(float value, float scale) {
    return (value > 0.0f) ? std::max((1.0f / scale), _SnapToPixelGrid(value, scale)) : 0.0f;
}

Document::~Document() {
    PROFILE

    if (_config != nullptr) {
        ::YGConfigFree((::YGConfig*)_config);
    }
}

Document::Document(Window& window)
    : Node()
    , _window(window)
    , _windowSub()
    , _windowInputArea()
    , _windowCursor(Cursor::Default)
    , _painter()
    , _mouseState()
    , _hoverState()
    , _pressState()
    , _focusState()
    , _dragState()
    , _caretState({ .visible = true, .blinkStart = GetTime(), .follow = false })
    , _config(nullptr)
    , _scale(1.0f)
    , _isUpdating(false)
    , _isFlushing(false)
    , _isRendering(false)
    , _needsUpdate(true)
    , _needsRender(true)
    , _needsCursor(true)
    , _renderList()
    , _eventQueue()
{
    PROFILE

    _document = this;
    _scale = window.getScale();
    _config = ::YGConfigNew();

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

void Document::needsUpdate() {
    PROFILE

    _needsUpdate = true;
}

void Document::needsRender() {
    PROFILE

    _needsRender = true;
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
    } else if (auto inputEvent = event.as<InputWindowEvent>()) {
        _handleInputEvent(*inputEvent);
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

    _triggerKeyDown(event.getScancode(), event.getKeycode(), event.getModifiers(), event.isRepeat(), defaultPrevented);

    if (defaultPrevented == false) {
        _processKey(event.getScancode(), event.getKeycode(), event.getModifiers());
    }

    _updateAll();
}

void Document::_handleKeyUpEvent(KeyUpWindowEvent const& event) {
    PROFILE

    _triggerKeyUp(event.getScancode(), event.getKeycode(), event.getModifiers());
    _updateAll();
}

void Document::_handleInputEvent(InputWindowEvent const& event) {
    PROFILE

    auto defaultPrevented = false;

    _triggerBeforeInput(event.getText(), defaultPrevented);

    if (defaultPrevented == false) {
        _insertText(event.getText());
    }

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
        _needsCursor = true;
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

    if (_mouseState.defaultPrevented == true) return;
    if (
        _mouseState.mouse != Mouse::LeftButton &&
        _mouseState.mouse != Mouse::RightButton
    ) {
        return;
    }

    if (
        _mouseState.mouse == Mouse::RightButton &&
        focusChanged == false
    ) {
        return;
    }

    if (auto inputState = _ensureInputState()) {
        auto const position = _getTextLocalPosition(*inputState, _convertPoint(_mouseState.position));

        if (_mouseState.mouse == Mouse::LeftButton) {
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

void Document::_processKey(Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers) {
    PROFILE

    auto processed = false;
    auto changed = false;

    if (
        (_focusState.focusedNode != nullptr) &&
        (_focusState.focusedNode->_keyEvents == true)
    ) {
        if (auto const match = _ResolveTextCommand(GetShortcutKeycode(scancode, keycode), modifiers)) {
            if (auto inputState = _ensureInputState()) {
                inputState->textObject.setMultiLine(inputState->boxNode._contentMultiLine);

                switch (match->command) {
                    case _TextCommand::MoveLeft: {
                        inputState->textObject.moveLeft(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveRight: {
                        inputState->textObject.moveRight(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveUp: {
                        inputState->textObject.moveUp(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveDown: {
                        inputState->textObject.moveDown(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveWordLeft: {
                        inputState->textObject.moveWordLeft(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveWordRight: {
                        inputState->textObject.moveWordRight(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveLineStart: {
                        inputState->textObject.moveLineStart(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveLineEnd: {
                        inputState->textObject.moveLineEnd(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveDocumentStart: {
                        inputState->textObject.moveDocumentStart(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::MoveDocumentEnd: {
                        inputState->textObject.moveDocumentEnd(match->extendSelection);
                        processed = true;
                        break;
                    }
                    case _TextCommand::DeleteBackward: {
                        changed = inputState->textObject.deleteBackward();
                        processed = true;
                        break;
                    }
                    case _TextCommand::DeleteForward: {
                        changed = inputState->textObject.deleteForward();
                        processed = true;
                        break;
                    }
                    case _TextCommand::DeleteWordBackward: {
                        changed = inputState->textObject.deleteWordBackward();
                        processed = true;
                        break;
                    }
                    case _TextCommand::DeleteWordForward: {
                        changed = inputState->textObject.deleteWordForward();
                        processed = true;
                        break;
                    }
                    case _TextCommand::DeleteLineBackward: {
                        changed = inputState->textObject.deleteLineBackward();
                        processed = true;
                        break;
                    }
                    case _TextCommand::DeleteLineForward: {
                        changed = inputState->textObject.deleteLineForward();
                        processed = true;
                        break;
                    }
                    case _TextCommand::Undo: {
                        changed = inputState->textObject.undo();
                        processed = true;
                        break;
                    }
                    case _TextCommand::Redo: {
                        changed = inputState->textObject.redo();
                        processed = true;
                        break;
                    }
                    case _TextCommand::Paste: {
                        changed = inputState->textObject.paste(GetClipboardString());
                        processed = true;
                        break;
                    }
                    case _TextCommand::SelectAll: {
                        inputState->textObject.selectAll();
                        processed = true;
                        break;
                    }
                    case _TextCommand::Copy:
                    case _TextCommand::Cut: {
                        if (
                            inputState->boxNode._contentSecure == false &&
                            inputState->textNode._contentSecure == false
                        ) {
                            auto string = std::string();
                            if (match->command == _TextCommand::Copy) {
                                inputState->textObject.copy(string);
                            } else {
                                changed = inputState->textObject.cut(string);
                            }
                            if (string.empty() == false) {
                                SetClipboardString(string);
                            }
                        }
                        processed = true;
                        break;
                    }
                    case _TextCommand::InsertLineBreak: {
                        if (inputState->boxNode._contentMultiLine == true) {
                            changed = inputState->textObject.input("\n");
                            processed = true;
                        }
                        break;
                    }
                    case _TextCommand::InsertTab: {
                        if (inputState->boxNode._contentMultiLine == true) {
                            changed = inputState->textObject.input("    ");
                            processed = true;
                        }
                        break;
                    }
                }

                if (processed) {
                    _needsRender = true;
                    _restartCaretBlink();
                }

                if (changed) {
                    inputState->textNode.setContent(inputState->textObject.getString());
                    _queueEvent(
                        std::make_unique<InputNodeEvent>(inputState->boxNode, inputState->textObject.getString())
                    );
                }
            }
        }
    }

    if (
        (processed == false) &&
        (keycode == Keycode::Tab) &&
        (modifiers.meta == false) &&
        (modifiers.control == false) &&
        (modifiers.alt == false)
    ) {
        _focusNext(modifiers.shift);
    }
}

void Document::_insertText(std::string const& text) {
    PROFILE

    if (_getTextInputNode() == nullptr) {
        return;
    }

    if (auto inputState = _ensureInputState()) {
        inputState->textObject.setMultiLine(inputState->boxNode._contentMultiLine);
        _needsRender = true;
        _restartCaretBlink();

        if (inputState->textObject.input(text)) {
            inputState->textNode.setContent(inputState->textObject.getString());
            _queueEvent(
                std::make_unique<InputNodeEvent>(inputState->boxNode, inputState->textObject.getString())
            );
        }
    }
}

float Document::_getTextVisibleWidth(Node const& node) const {
    PROFILE

    /* the slot a box gives its single-line text, in pixels: the box's border
       rect minus its border and, mirrored, the text's left inset */
    auto const& parent = *node._parent;
    auto const inset = (node._computedBorderRectInDocument.x - parent._computedBorderRectInDocument.x - parent._computedBorderEdge.left);
    auto const visibleMaxX = (parent._computedBorderRectInDocument.getMaxX() - parent._computedBorderEdge.right - inset);

    return std::max(1.0f, ((visibleMaxX - node._computedBorderRectInDocument.x) * _scale));
}

Vec2 Document::_getTextLocalPosition(_InputState const& inputState, Vec2 const& position) const {
    PROFILE

    return ((position - (inputState.textNode._computedBorderRectInDocument.origin + inputState.textNode._computedTextRect.origin)) * _scale)
        + Vec2{ inputState.textNode._textScrollX, 0.0f };
}

Node* Document::_getTextInputNode() const {
    PROFILE

    /* the focused node takes text when it has key events and is an editable or draws its own text (an input area) */
    if (
        (_focusState.focusedNode == nullptr) ||
        (_focusState.focusedNode->_keyEvents == false) ||
        (
            (_isNodeEditable(*_focusState.focusedNode) == false) &&
            (_focusState.focusedNode->_inputArea.has_value() == false)
        )
    ) {
        return nullptr;
    }

    return _focusState.focusedNode;
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

    _caretState.blinkStart = GetTime();
    _caretState.visible = true;
    _caretState.follow = true;
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

    ::YGConfigSetPointScaleFactor((::YGConfig*)_config, _scale);
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

    if (
        (node._display == NodeDisplay::Text) &&
        (node._textObject != nullptr)
    ) {
        node._textObject->setMaxWidth(std::nullopt);
        node._textObject->setMaxHeight(std::nullopt);
        node._textObject->setHeight(std::floorf(node._computedTextRect.height * _scale));
        node._textObject->setWidth(
            _isNodeSingleLineText(node)
                ? std::nullopt
                : std::optional<float>(
                    std::floorf(node._computedTextRect.width * _scale)
                )
        );

        if (node._textObject->getEditable() == false) {
            _updateTextScroll(node);
        }
    }
}

void Document::_updateTextScroll(Node& node) {
    PROFILE

    auto const text = node._textObject.get();
    if (text == nullptr) return;

    auto const singleLine = _isNodeSingleLineText(node);
    auto scrollX = node._textScrollX;

    /* A single-line surface never wraps. While it is being edited it scrolls
       horizontally so the caret stays inside the slot its box gives it;
       unfocused, it shows its start. */
    if (singleLine && text->getEditable()) {
        auto const visibleWidth = _getTextVisibleWidth(node);
        auto const& caretRect = text->getCaretRect();

        if ((caretRect.getMaxX() - scrollX) > visibleWidth) {
            scrollX = (caretRect.getMaxX() - visibleWidth);
        }
        if ((caretRect.x - scrollX) < 0.0f) {
            scrollX = caretRect.x;
        }

        scrollX = std::ceilf(std::clamp(scrollX, 0.0f, std::max(0.0f, (std::max(text->getSize().width, caretRect.getMaxX()) - visibleWidth))));
    } else {
        scrollX = 0.0f;
    }

    if (node._textScrollX != scrollX) {
        node._textScrollX = scrollX;
        _needsRender = true;
    }

    /* A multi-line surface wraps and grows instead: once its caret has been
       touched, the nearest ancestor scrolling vertically brings it back
       inside its padding. */
    if ((singleLine == false) && text->getEditable() && _caretState.follow) {
        auto container = static_cast<Node*>(nullptr);

        for (auto ancestor = node._parent; ancestor != nullptr; ancestor = ancestor->_parent) {
            if (ancestor->getOverflowY() == NodeOverflow::Scroll) {
                container = ancestor;
                break;
            }
        }

        if (container != nullptr) {
            auto const& caretRect = text->getCaretRect();
            auto const& containerRect = container->_computedBorderRectInDocument;
            auto const& borderEdge = container->_computedBorderEdge;
            auto const paddingTop = ::YGNodeLayoutGetPadding((::YGNode*)container->_layoutNode, ::YGEdgeTop);
            auto const paddingBottom = ::YGNodeLayoutGetPadding((::YGNode*)container->_layoutNode, ::YGEdgeBottom);

            auto const caretMinY = (node._computedBorderRectInDocument.y + node._computedTextRect.y + (caretRect.y / _scale));
            auto const caretHeight = (caretRect.height / _scale);
            auto const viewMinY = (containerRect.y + borderEdge.top + paddingTop);
            auto const viewHeight = std::max(0.0f, (containerRect.height - borderEdge.top - borderEdge.bottom - paddingTop - paddingBottom));

            auto delta = 0.0f;

            if (
                (caretHeight > viewHeight) ||
                (caretMinY < viewMinY)
            ) {
                delta = (caretMinY - viewMinY);
            } else if ((caretMinY + caretHeight) > (viewMinY + viewHeight)) {
                delta = ((caretMinY + caretHeight) - (viewMinY + viewHeight));
            }

            auto const scrollY = _SnapToPixelGrid(std::clamp((container->_scrollPosition.y + delta), 0.0f, container->_scrollOverflow.y), _scale);

            if (container->_scrollPosition.y != scrollY) {
                container->_scrollPosition.y = scrollY;
                _needsUpdate = true;
            }
        }
    }
}

void Document::_updateInputArea() {
    PROFILE

    auto area = std::optional<Vec4>();

    if (auto const node = _getTextInputNode()) {
        if (node->_inputArea.has_value()) {
            area = Vec4{ node->convertPointToDocument(node->_inputArea->origin), node->_inputArea->size };
        } else {
            /* an editable: its caret in document coordinates, the inverse of _getTextLocalPosition */
            auto const& textNode = *node->_firstChild;
            auto const textOrigin = (textNode._computedBorderRectInDocument.origin + textNode._computedTextRect.origin);

            if (textNode._textObject != nullptr) {
                auto const& caretRect = textNode._textObject->getCaretRect();
                area = Vec4{
                    (textOrigin + ((caretRect.origin - Vec2{ textNode._textScrollX, 0.0f }) * (1.0f / _scale))),
                    (caretRect.size * (1.0f / _scale))
                };
            } else {
                area = Vec4{ textOrigin, Vec2{ 1.0f, textNode._computedTextRect.height } };
            }
        }
    }

    /* document coordinates to window points: the inverse of _convertPoint */
    if (area.has_value()) {
        auto const factor = (_scale / _window.getScale());
        area = Vec4{ (area->origin * factor), (area->size * factor) };
    }

    if (area != _windowInputArea) {
        _windowInputArea = area;
        _window.setInputArea(area);
    }
}

void Document::_updateCursor() {
    PROFILE

    if (_needsCursor == false) return;
    else _needsCursor = false;

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

            _renderList[_computedZIndex].push_back(this);

            _hoverNode();
        }

        if (_eventQueue.empty() == false) {
            if (_isFlushing == false) {
                _flushEvents();
                continue;
            }
        }

        if (_focusState.focusedNode != nullptr) {
            if (_isNodeEditable(*_focusState.focusedNode)) {
                _updateTextScroll(*_focusState.focusedNode->_firstChild);
            }
        }

        _caretState.follow = false;

        if (_needsUpdate == false) {
            break;
        }
    }

    _updateCursor();
    _updateInputArea();

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
    _renderNodePaint(node, info);

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

void Document::_renderNodeText(Node& node, _RenderInfo const& info) {
    PROFILE

    if (node._display != NodeDisplay::Text) return;
    if (node._textObject == nullptr) return;

    auto const text = node._textObject.get();
    auto const editable = text->getEditable();
    auto const singleLine = _isNodeSingleLineText(node);
    auto const textOrigin = (info.borderShape.rect.origin + (node._computedTextRect.origin * _scale));
    auto textClipRect = info.clipRect;
    auto textRect = Vec4{
        Vec2{ std::roundf(textOrigin.x), std::roundf(textOrigin.y) },
        text->getSize()
    };

    if (singleLine) {
        textRect.x -= node._textScrollX;
        auto const visibleWidth = _getTextVisibleWidth(node);
        auto const clipMinX = std::max(info.clipRect.x, info.borderShape.rect.x);
        auto const clipMaxX = std::min(info.clipRect.getMaxX(), (info.borderShape.rect.x + visibleWidth));
        textClipRect = { clipMinX, info.clipRect.y, std::max(0.0f, (clipMaxX - clipMinX)), info.clipRect.height };
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
        _caretState.visible &&
        (text->isSelectedRange() == false)
    ) {
        auto const& caretRect = text->getCaretRect();
        auto const caretShape = QuadShape{ Vec4{ (textRect.origin + caretRect.origin), caretRect.size } };
        auto const caretBrush = ColorBrush{ .color = node._computedTextColor };
        _painter.paint(caretShape, caretBrush, { .scissor = textClipRect });
    }
}

void Document::_renderNodePaint(Node& node, _RenderInfo const& info) {
    PROFILE

    if (node.onPaint.hasSubs()) {
        node.onPaint.publish(_painter, info.borderShape.rect);
    }
}

void Document::_renderNodeForeground(Node& node, _RenderInfo const& info) {
    PROFILE

    if (node._foreground) {
        _painter.paint(info.borderShape, *node._foreground, { .scissor = info.scissorRect });
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
        auto const phase = ((GetTime() - _caretState.blinkStart) / _caretBlinkPeriod);
        auto const visible = text.isSelectedRange() || ((phase % 2) == 0);

        if (_caretState.visible != visible) {
            _caretState.visible = visible;
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

    _isRendering = true;

    try {
        for (auto& [ zIndex, nodes ] : _renderList) {
            for (auto& node : nodes) {
                _renderNode(*node, { 0.0f, 0.0f }, zIndex);
            }
        }
    } catch (...) {
        _isRendering = false;
        throw;
    }

    _isRendering = false;

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

void Document::_triggerKeyDown(Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers, bool repeat, bool& defaultPrevented) {
    PROFILE

    _flushEvents();

    auto targetNode = (
        (_focusState.focusedNode != nullptr && _focusState.focusedNode->_keyEvents == true)
            ? _focusState.focusedNode
            : this
    );

    auto const event = KeyDownNodeEvent(*targetNode, scancode, keycode, modifiers, repeat);
    targetNode->dispatchEvent(event);
    defaultPrevented = event.isDefaultPrevented();
}

void Document::_triggerBeforeInput(std::string const& text, bool& defaultPrevented) {
    PROFILE

    _flushEvents();

    auto const targetNode = _getTextInputNode();
    if (targetNode == nullptr) return;

    auto const event = BeforeInputNodeEvent(*targetNode, text);
    targetNode->dispatchEvent(event);
    defaultPrevented = event.isDefaultPrevented();
}

void Document::_triggerKeyUp(Scancode scancode, std::string const& keycode, KeyModifiers const& modifiers) {
    PROFILE

    _flushEvents();

    auto targetNode = (
        (_focusState.focusedNode != nullptr && _focusState.focusedNode->_keyEvents == true)
            ? _focusState.focusedNode
            : this
    );

    targetNode->dispatchEvent(
        KeyUpNodeEvent(*targetNode, scancode, keycode, modifiers)
    );
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

    find(this, find);

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

bool Document::_isNodeSingleLineText(Node const& node) const {
    PROFILE

    return (node._textObject != nullptr)
        && (node._textObject->getMultiLine() == false)
        && (node._parent != nullptr);
}

} /* namespace Rocket */
