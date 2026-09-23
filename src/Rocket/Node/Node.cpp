/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <map>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <limits>
#include <yoga/Yoga.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Text.hpp>
#include <Rocket/Node/Node.hpp>
#include <Rocket/Node/Document.hpp>

namespace Rocket {

static auto _positionMap = std::map<NodePosition, ::YGPositionType>{
    { NodePosition::Relative, ::YGPositionTypeRelative },
    { NodePosition::Absolute, ::YGPositionTypeAbsolute },
    { NodePosition::Fixed,    ::YGPositionTypeAbsolute }
};

static auto _directionMap = std::map<NodeDirection, ::YGFlexDirection>{
    { NodeDirection::Horizontal,        ::YGFlexDirectionRow },
    { NodeDirection::HorizontalReverse, ::YGFlexDirectionRowReverse },
    { NodeDirection::Vertical,          ::YGFlexDirectionColumn },
    { NodeDirection::VerticalReverse,   ::YGFlexDirectionColumnReverse }
};

static auto _alignmentMap = std::map<NodeAlignment, ::YGAlign>{
    { NodeAlignment::Start,   ::YGAlignFlexStart },
    { NodeAlignment::Center,  ::YGAlignCenter },
    { NodeAlignment::Stretch, ::YGAlignStretch },
    { NodeAlignment::End,     ::YGAlignFlexEnd }
};

static auto _justifyMap = std::map<NodeJustify, ::YGJustify>{
    { NodeJustify::Start,        ::YGJustifyFlexStart },
    { NodeJustify::Center,       ::YGJustifyCenter },
    { NodeJustify::End,          ::YGJustifyFlexEnd },
    { NodeJustify::SpaceBetween, ::YGJustifySpaceBetween },
    { NodeJustify::SpaceAround,  ::YGJustifySpaceAround },
    { NodeJustify::SpaceEvenly,  ::YGJustifySpaceEvenly }
};

bool NodeTransform::operator==(NodeTransform const& other) const {
    PROFILE

    return (translateX == other.translateX)
        && (translateY == other.translateY);
}

bool NodeTransform::operator!=(NodeTransform const& other) const {
    PROFILE

    return !(*this == other);
}

Node::~Node() {
    PROFILE

    removeFromParent();
    removeAllChildren();

    _destroyTextNode();
    _resetTextState();
    _resetLayoutState();
    _resetPaintState();
    _resetInputState();

    if (auto parent = ::YGNodeGetParent((::YGNode*)_layoutNode)) {
        ::YGNodeRemoveChild(parent, (::YGNode*)_layoutNode);
    }

    ::YGNodeRemoveAllChildren((::YGNode*)_layoutNode);
    ::YGNodeFree((::YGNode*)_layoutNode);
}

Node::Node()
    : _document(nullptr)
    , _parent(nullptr)
    , _firstChild(nullptr)
    , _lastChild(nullptr)
    , _nextSibling(nullptr)
    , _prevSibling(nullptr)
    , _display(std::nullopt)
    , _overflowX(std::nullopt)
    , _overflowY(std::nullopt)
    , _position(std::nullopt)
    , _direction(std::nullopt)
    , _alignment(std::nullopt)
    , _justify(std::nullopt)
    , _selfAlignment(std::nullopt)
    , _width(std::nullopt)
    , _height(std::nullopt)
    , _minWidth(std::nullopt)
    , _maxWidth(std::nullopt)
    , _minHeight(std::nullopt)
    , _maxHeight(std::nullopt)
    , _top(std::nullopt)
    , _left(std::nullopt)
    , _right(std::nullopt)
    , _bottom(std::nullopt)
    , _padding(std::nullopt)
    , _paddingTop(std::nullopt)
    , _paddingLeft(std::nullopt)
    , _paddingRight(std::nullopt)
    , _paddingBottom(std::nullopt)
    , _margin(std::nullopt)
    , _marginTop(std::nullopt)
    , _marginLeft(std::nullopt)
    , _marginRight(std::nullopt)
    , _marginBottom(std::nullopt)
    , _gap(std::nullopt)
    , _gapX(std::nullopt)
    , _gapY(std::nullopt)
    , _borderWidth(std::nullopt)
    , _borderTopWidth(std::nullopt)
    , _borderLeftWidth(std::nullopt)
    , _borderRightWidth(std::nullopt)
    , _borderBottomWidth(std::nullopt)
    , _borderRadius(std::nullopt)
    , _borderTopLeftRadius(std::nullopt)
    , _borderTopRightRadius(std::nullopt)
    , _borderBottomLeftRadius(std::nullopt)
    , _borderBottomRightRadius(std::nullopt)
    , _visible(std::nullopt)
    , _zIndex(std::nullopt)
    , _offset(std::nullopt)
    , _opacity(std::nullopt)
    , _transform(std::nullopt)
    , _fontFamily(std::nullopt)
    , _fontWeight(std::nullopt)
    , _fontStyle(std::nullopt)
    , _fontSize(std::nullopt)
    , _lineHeight(std::nullopt)
    , _textColor(std::nullopt)
    , _textMarker(std::nullopt)
    , _background(std::nullopt)
    , _foreground(std::nullopt)
    , _border(std::nullopt)
    , _shadow(std::nullopt)
    , _cursor(std::nullopt)
    , _content(std::nullopt)
    , _contentEditable(false)
    , _contentSecure(false)
    , _contentMultiLine(false)
    , _tabIndex(0)
    , _skip(false)
    , _flex(false)
    , _keyEvents(true)
    , _mouseEvents(true)
    , _clipped(true)
    , _isHover()
    , _isActive()
    , _isFocused()
    , _isFocusedWithin()
    , _textScrollX()
    , _textObject()
    , _computedFontFamily()
    , _computedFontWeight()
    , _computedFontStyle()
    , _computedFontSize()
    , _computedLineHeight()
    , _computedTextColor()
    , _computedMarkerColor()
    , _computedTextRect()
    , _computedBorderEdge()
    , _computedBorderRect()
    , _computedMarginRect()
    , _computedContentRect()
    , _computedBorderRectInDocument()
    , _computedMarginRectInDocument()
    , _computedClipRectInDocument()
    , _computedZIndex()
    , _layerImage()
    , _shadowImage()
    , _shadowImageShadow()
    , _shadowImageRadius()
    , _shadowImageClipRect()
    , _shadowImageScale()
    , _scrollOverflow()
    , _scrollPosition()
    , _layoutNode(nullptr)
    , _textNode(nullptr)
    , _needsTextUpdate()
    , _needsLayoutUpdate()
{
    PROFILE

    _layoutNode = ::YGNodeNew();
    ::YGNodeStyleSetFlexGrow((::YGNode*)_layoutNode, 0.0f);
    ::YGNodeStyleSetFlexShrink((::YGNode*)_layoutNode, 0.0f);
    ::YGNodeStyleSetFlexBasis((::YGNode*)_layoutNode, ::YGUndefined);
    ::YGNodeStyleSetFlexWrap((::YGNode*)_layoutNode, ::YGWrapNoWrap);
    ::YGNodeStyleSetDisplay((::YGNode*)_layoutNode, ::YGDisplayFlex);
    ::YGNodeStyleSetOverflow((::YGNode*)_layoutNode, ::YGOverflowHidden);
    ::YGNodeStyleSetBoxSizing((::YGNode*)_layoutNode, ::YGBoxSizingBorderBox);
    ::YGNodeStyleSetPositionType((::YGNode*)_layoutNode, ::YGPositionTypeRelative);
    ::YGNodeStyleSetFlexDirection((::YGNode*)_layoutNode, ::YGFlexDirectionRow);
    ::YGNodeStyleSetAlignItems((::YGNode*)_layoutNode, ::YGAlignFlexStart);
    ::YGNodeStyleSetJustifyContent((::YGNode*)_layoutNode, ::YGJustifyFlexStart);
    ::YGNodeSetAlwaysFormsContainingBlock((::YGNode*)_layoutNode, true);

    _resetTextState();
    _resetLayoutState();
    _resetPaintState();
    _resetInputState();
}

Document* Node::getDocument() const {
    PROFILE

    return _document;
}

Node* Node::getParent() const {
    PROFILE

    return _parent;
}

Node* Node::getFirstChild() const {
    PROFILE

    return _firstChild;
}

Node* Node::getLastChild() const {
    PROFILE

    return _lastChild;
}

Node* Node::getPrevSibling() const {
    PROFILE

    return _prevSibling;
}

Node* Node::getNextSibling() const {
    PROFILE

    return _nextSibling;
}

Vec4 const& Node::getComputedBorderRect() const {
    PROFILE

    return _computedBorderRectInDocument;
}

Vec4 const& Node::getComputedMarginRect() const {
    PROFILE

    return _computedMarginRectInDocument;
}

Vec4 const& Node::getComputedClipRect() const {
    PROFILE

    return _computedClipRectInDocument;
}

std::int64_t Node::getComputedZIndex() const {
    PROFILE

    return _computedZIndex;
}

std::string const& Node::getComputedFontFamily() const {
    PROFILE

    return _computedFontFamily;
}

FontWeight Node::getComputedFontWeight() const {
    PROFILE

    return _computedFontWeight;
}

FontStyle Node::getComputedFontStyle() const {
    PROFILE

    return _computedFontStyle;
}

float Node::getComputedFontSize() const {
    PROFILE

    return _computedFontSize;
}

Vec4 const& Node::getComputedTextColor() const {
    PROFILE

    return _computedTextColor;
}

Vec4 const& Node::getComputedTextMarker() const {
    PROFILE

    return _computedMarkerColor;
}

float Node::getComputedLineHeight() const {
    PROFILE

    return _computedLineHeight;
}

std::optional<NodeDisplay> const& Node::getDisplay() const {
    PROFILE

    return _display;
}

std::optional<NodeDirection> const& Node::getDirection() const {
    PROFILE

    return _direction;
}

std::optional<NodeAlignment> const& Node::getAlignment() const {
    PROFILE

    return _alignment;
}

std::optional<NodeJustify> const& Node::getJustify() const {
    PROFILE

    return _justify;
}

std::optional<NodeOverflow> const& Node::getOverflowX() const {
    PROFILE

    return _overflowX;
}

std::optional<NodeOverflow> const& Node::getOverflowY() const {
    PROFILE

    return _overflowY;
}

std::optional<NodePosition> const& Node::getPosition() const {
    PROFILE

    return _position;
}

std::optional<NodeAlignment> const& Node::getSelfAlignment() const {
    PROFILE

    return _selfAlignment;
}

std::optional<NodeValue> const& Node::getWidth() const {
    PROFILE

    return _width;
}

std::optional<NodeValue> const& Node::getHeight() const {
    PROFILE

    return _height;
}

std::optional<NodeValue> const& Node::getMinWidth() const {
    PROFILE

    return _minWidth;
}

std::optional<NodeValue> const& Node::getMaxWidth() const {
    PROFILE

    return _maxWidth;
}

std::optional<NodeValue> const& Node::getMinHeight() const {
    PROFILE

    return _minHeight;
}

std::optional<NodeValue> const& Node::getMaxHeight() const {
    PROFILE

    return _maxHeight;
}

std::optional<NodeValue> const& Node::getTop() const {
    PROFILE

    return _top;
}

std::optional<NodeValue> const& Node::getLeft() const {
    PROFILE

    return _left;
}

std::optional<NodeValue> const& Node::getRight() const {
    PROFILE

    return _right;
}

std::optional<NodeValue> const& Node::getBottom() const {
    PROFILE

    return _bottom;
}

std::optional<NodeValue> const& Node::getPadding() const {
    PROFILE

    return _padding;
}

std::optional<NodeValue> const& Node::getPaddingTop() const {
    PROFILE

    return _paddingTop;
}

std::optional<NodeValue> const& Node::getPaddingLeft() const {
    PROFILE

    return _paddingLeft;
}

std::optional<NodeValue> const& Node::getPaddingRight() const {
    PROFILE

    return _paddingRight;
}

std::optional<NodeValue> const& Node::getPaddingBottom() const {
    PROFILE

    return _paddingBottom;
}

std::optional<NodeValue> const& Node::getMargin() const {
    PROFILE

    return _margin;
}

std::optional<NodeValue> const& Node::getMarginTop() const {
    PROFILE

    return _marginTop;
}

std::optional<NodeValue> const& Node::getMarginLeft() const {
    PROFILE

    return _marginLeft;
}

std::optional<NodeValue> const& Node::getMarginRight() const {
    PROFILE

    return _marginRight;
}

std::optional<NodeValue> const& Node::getMarginBottom() const {
    PROFILE

    return _marginBottom;
}

std::optional<NodeValue> const& Node::getGap() const {
    PROFILE

    return _gap;
}

std::optional<NodeValue> const& Node::getGapX() const {
    PROFILE

    return _gapX;
}

std::optional<NodeValue> const& Node::getGapY() const {
    PROFILE

    return _gapY;
}

std::optional<float> const& Node::getBorderWidth() const {
    PROFILE

    return _borderWidth;
}

std::optional<float> const& Node::getBorderTopWidth() const {
    PROFILE

    return _borderTopWidth;
}

std::optional<float> const& Node::getBorderLeftWidth() const {
    PROFILE

    return _borderLeftWidth;
}

std::optional<float> const& Node::getBorderRightWidth() const {
    PROFILE

    return _borderRightWidth;
}

std::optional<float> const& Node::getBorderBottomWidth() const {
    PROFILE

    return _borderBottomWidth;
}

std::optional<float> const& Node::getBorderRadius() const {
    PROFILE

    return _borderRadius;
}

std::optional<float> const& Node::getBorderTopLeftRadius() const {
    PROFILE

    return _borderTopLeftRadius;
}

std::optional<float> const& Node::getBorderTopRightRadius() const {
    PROFILE

    return _borderTopRightRadius;
}

std::optional<float> const& Node::getBorderBottomLeftRadius() const {
    PROFILE

    return _borderBottomLeftRadius;
}

std::optional<float> const& Node::getBorderBottomRightRadius() const {
    PROFILE

    return _borderBottomRightRadius;
}

std::optional<bool> const& Node::getVisible() const {
    PROFILE

    return _visible;
}

std::optional<int> const& Node::getZIndex() const {
    PROFILE

    return _zIndex;
}

std::optional<Vec2> const& Node::getOffset() const {
    PROFILE

    return _offset;
}

std::optional<float> const& Node::getOpacity() const {
    PROFILE

    return _opacity;
}

std::optional<NodeTransform> const& Node::getTransform() const {
    PROFILE

    return _transform;
}

std::optional<std::string> const& Node::getFontFamily() const {
    PROFILE

    return _fontFamily;
}

std::optional<FontWeight> const& Node::getFontWeight() const {
    PROFILE

    return _fontWeight;
}

std::optional<FontStyle> const& Node::getFontStyle() const {
    PROFILE

    return _fontStyle;
}

std::optional<float> const& Node::getFontSize() const {
    PROFILE

    return _fontSize;
}

std::optional<float> const& Node::getLineHeight() const {
    PROFILE

    return _lineHeight;
}

std::optional<Vec4> const& Node::getTextColor() const {
    PROFILE

    return _textColor;
}

std::optional<Vec4> const& Node::getTextMarker() const {
    PROFILE

    return _textMarker;
}

std::optional<Brush> const& Node::getBackground() const {
    PROFILE

    return _background;
}

std::optional<Brush> const& Node::getForeground() const {
    PROFILE

    return _foreground;
}

std::optional<Brush> const& Node::getBorder() const {
    PROFILE

    return _border;
}

std::optional<Shadow> const& Node::getShadow() const {
    PROFILE

    return _shadow;
}

std::optional<Cursor> const& Node::getCursor() const {
    PROFILE

    return _cursor;
}

std::optional<std::string> const& Node::getContent() const {
    PROFILE

    return _content;
}

bool Node::getContentEditable() const {
    PROFILE

    return _contentEditable;
}

bool Node::getContentSecure() const {
    PROFILE

    return _contentSecure;
}

bool Node::getContentMultiLine() const {
    PROFILE

    return _contentMultiLine;
}

bool Node::getSkip() const {
    PROFILE

    return _skip;
}

bool Node::getFlex() const {
    PROFILE

    return _flex;
}

bool Node::getKeyEvents() const {
    PROFILE

    return _keyEvents;
}

bool Node::getMouseEvents() const {
    PROFILE

    return _mouseEvents;
}

bool Node::getClipped() const {
    PROFILE

    return _clipped;
}

int Node::getTabIndex() const {
    PROFILE

    return _tabIndex;
}

bool Node::isHover() const {
    PROFILE

    return _isHover;
}

bool Node::isActive() const {
    PROFILE

    return _isActive;
}

bool Node::isFocused() const {
    PROFILE

    return _isFocused;
}

bool Node::isFocusedWithin() const {
    PROFILE

    return _isFocusedWithin;
}

void Node::getPath(std::vector<Node*>& path) const {
    PROFILE

    path.clear();

    auto next = const_cast<Node*>(this);
    while (next != nullptr) {
        path.push_back(next);
        next = next->_parent;
    }

    std::reverse(path.begin(), path.end());
}

void Node::setDisplay(std::optional<NodeDisplay> const& display) {
    PROFILE

    if (_display != display) {
        _display = display;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
        _updateLayout();
    }
}

void Node::setDirection(std::optional<NodeDirection> const& direction) {
    PROFILE

    if (_direction != direction) {
        _direction = direction;
        _needsLayoutUpdate = true;

        ::YGNodeStyleSetFlexDirection((::YGNode*)_layoutNode, direction.has_value() ? _directionMap[direction.value()] : ::YGFlexDirectionRow);
    }
}

void Node::setAlignment(std::optional<NodeAlignment> const& alignment) {
    PROFILE

    if (_alignment != alignment) {
        _alignment = alignment;
        _needsLayoutUpdate = true;

        ::YGNodeStyleSetAlignItems((::YGNode*)_layoutNode, alignment.has_value() ? _alignmentMap[alignment.value()] : ::YGAlignFlexStart);
    }
}

void Node::setJustify(std::optional<NodeJustify> const& justify) {
    PROFILE

    if (_justify != justify) {
        _justify = justify;
        _needsLayoutUpdate = true;

        ::YGNodeStyleSetJustifyContent((::YGNode*)_layoutNode, justify.has_value() ? _justifyMap[justify.value()] : ::YGJustifyFlexStart);
    }
}

void Node::setOverflowX(std::optional<NodeOverflow> const& overflowX) {
    PROFILE

    if (_overflowX != overflowX) {
        _overflowX = overflowX;
        _needsLayoutUpdate = true;
    }
}

void Node::setOverflowY(std::optional<NodeOverflow> const& overflowY) {
    PROFILE

    if (_overflowY != overflowY) {
        _overflowY = overflowY;
        _needsLayoutUpdate = true;
    }
}

void Node::setPosition(std::optional<NodePosition> const& position) {
    PROFILE

    if (_position != position) {
        _position = position;
        _needsLayoutUpdate = true;

        ::YGNodeStyleSetPositionType((::YGNode*)_layoutNode, position.has_value() ? _positionMap[position.value()] : ::YGPositionTypeRelative);
    }
}

void Node::setSelfAlignment(std::optional<NodeAlignment> const& selfAlignment) {
    PROFILE

    if (_selfAlignment != selfAlignment) {
        _selfAlignment = selfAlignment;
        _needsLayoutUpdate = true;

        ::YGNodeStyleSetAlignSelf((::YGNode*)_layoutNode, selfAlignment.has_value() ? _alignmentMap[selfAlignment.value()] : ::YGAlignAuto);
    }
}

void Node::setWidth(std::optional<NodeValue> const& width) {
    PROFILE

    if (_width != width) {
        _width = width;
        _needsLayoutUpdate = true;

        if (width.has_value()) {
            width->match(
                [&](PixelValue const& pixel) { ::YGNodeStyleSetWidth((::YGNode*)_layoutNode, pixel.value); },
                [&](PercentValue const& percent) { ::YGNodeStyleSetWidthPercent((::YGNode*)_layoutNode, percent.value); }
            );
        } else {
            ::YGNodeStyleSetWidth((::YGNode*)_layoutNode, ::YGUndefined);
        }
    }
}

void Node::setHeight(std::optional<NodeValue> const& height) {
    PROFILE

    if (_height != height) {
        _height = height;
        _needsLayoutUpdate = true;

        if (height.has_value()) {
            height->match(
                [&](PixelValue const& pixel) { ::YGNodeStyleSetHeight((::YGNode*)_layoutNode, pixel.value); },
                [&](PercentValue const& percent) { ::YGNodeStyleSetHeightPercent((::YGNode*)_layoutNode, percent.value); }
            );
        } else {
            ::YGNodeStyleSetHeight((::YGNode*)_layoutNode, ::YGUndefined);
        }
    }
}

void Node::setMinWidth(std::optional<NodeValue> const& minWidth) {
    PROFILE

    if (_minWidth != minWidth) {
        _minWidth = minWidth;
        _needsLayoutUpdate = true;

        if (minWidth.has_value()) {
            minWidth->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetMinWidth((::YGNode*)_layoutNode, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetMinWidthPercent((::YGNode*)_layoutNode, pct.value); }
            );
        } else {
            ::YGNodeStyleSetMinWidth((::YGNode*)_layoutNode, ::YGUndefined);
        }
    }
}

void Node::setMaxWidth(std::optional<NodeValue> const& maxWidth) {
    PROFILE

    if (_maxWidth != maxWidth) {
        _maxWidth = maxWidth;
        _needsLayoutUpdate = true;

        if (maxWidth.has_value()) {
            maxWidth->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetMaxWidth((::YGNode*)_layoutNode, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetMaxWidthPercent((::YGNode*)_layoutNode, pct.value); }
            );
        } else {
            ::YGNodeStyleSetMaxWidth((::YGNode*)_layoutNode, ::YGUndefined);
        }
    }
}

void Node::setMinHeight(std::optional<NodeValue> const& minHeight) {
    PROFILE

    if (_minHeight != minHeight) {
        _minHeight = minHeight;
        _needsLayoutUpdate = true;

        if (minHeight.has_value()) {
            minHeight->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetMinHeight((::YGNode*)_layoutNode, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetMinHeightPercent((::YGNode*)_layoutNode, pct.value); }
            );
        } else {
            ::YGNodeStyleSetMinHeight((::YGNode*)_layoutNode, ::YGUndefined);
        }
    }
}

void Node::setMaxHeight(std::optional<NodeValue> const& maxHeight) {
    PROFILE

    if (_maxHeight != maxHeight) {
        _maxHeight = maxHeight;
        _needsLayoutUpdate = true;

        if (maxHeight.has_value()) {
            maxHeight->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetMaxHeight((::YGNode*)_layoutNode, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetMaxHeightPercent((::YGNode*)_layoutNode, pct.value); }
            );
        } else {
            ::YGNodeStyleSetMaxHeight((::YGNode*)_layoutNode, ::YGUndefined);
        }
    }
}

void Node::setTop(std::optional<NodeValue> const& top) {
    PROFILE

    if (_top != top) {
        _top = top;
        _needsLayoutUpdate = true;

        if (top.has_value()) {
            top->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeTop, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetPositionPercent((::YGNode*)_layoutNode, ::YGEdgeTop, pct.value); }
            );
        } else {
            ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeTop, ::YGUndefined);
        }
    }
}

void Node::setLeft(std::optional<NodeValue> const& left) {
    PROFILE

    if (_left != left) {
        _left = left;
        _needsLayoutUpdate = true;

        if (left.has_value()) {
            left->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeLeft, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetPositionPercent((::YGNode*)_layoutNode, ::YGEdgeLeft, pct.value); }
            );
        } else {
            ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeLeft, ::YGUndefined);
        }
    }
}

void Node::setRight(std::optional<NodeValue> const& right) {
    PROFILE

    if (_right != right) {
        _right = right;
        _needsLayoutUpdate = true;

        if (right.has_value()) {
            right->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeRight, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetPositionPercent((::YGNode*)_layoutNode, ::YGEdgeRight, pct.value); }
            );
        } else {
            ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeRight, ::YGUndefined);
        }
    }
}

void Node::setBottom(std::optional<NodeValue> const& bottom) {
    PROFILE

    if (_bottom != bottom) {
        _bottom = bottom;
        _needsLayoutUpdate = true;

        if (bottom.has_value()) {
            bottom->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeBottom, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetPositionPercent((::YGNode*)_layoutNode, ::YGEdgeBottom, pct.value); }
            );
        } else {
            ::YGNodeStyleSetPosition((::YGNode*)_layoutNode, ::YGEdgeBottom, ::YGUndefined);
        }
    }
}

void Node::setPadding(std::optional<NodeValue> const& padding) {
    PROFILE

    if (_padding != padding) {
        _padding = padding;
        _needsLayoutUpdate = true;
        _setNodePadding();
    }
}

void Node::setPaddingTop(std::optional<NodeValue> const& paddingTop) {
    PROFILE

    if (_paddingTop != paddingTop) {
        _paddingTop = paddingTop;
        _needsLayoutUpdate = true;
        _setNodePadding();
    }
}

void Node::setPaddingLeft(std::optional<NodeValue> const& paddingLeft) {
    PROFILE

    if (_paddingLeft != paddingLeft) {
        _paddingLeft = paddingLeft;
        _needsLayoutUpdate = true;
        _setNodePadding();
    }
}

void Node::setPaddingRight(std::optional<NodeValue> const& paddingRight) {
    PROFILE

    if (_paddingRight != paddingRight) {
        _paddingRight = paddingRight;
        _needsLayoutUpdate = true;
        _setNodePadding();
    }
}

void Node::setPaddingBottom(std::optional<NodeValue> const& paddingBottom) {
    PROFILE

    if (_paddingBottom != paddingBottom) {
        _paddingBottom = paddingBottom;
        _needsLayoutUpdate = true;
        _setNodePadding();
    }
}

void Node::setMargin(std::optional<NodeValue> const& margin) {
    PROFILE

    if (_margin != margin) {
        _margin = margin;
        _needsLayoutUpdate = true;
        _setNodeMargin();
    }
}

void Node::setMarginTop(std::optional<NodeValue> const& marginTop) {
    PROFILE

    if (_marginTop != marginTop) {
        _marginTop = marginTop;
        _needsLayoutUpdate = true;
        _setNodeMargin();
    }
}

void Node::setMarginLeft(std::optional<NodeValue> const& marginLeft) {
    PROFILE

    if (_marginLeft != marginLeft) {
        _marginLeft = marginLeft;
        _needsLayoutUpdate = true;
        _setNodeMargin();
    }
}

void Node::setMarginRight(std::optional<NodeValue> const& marginRight) {
    PROFILE

    if (_marginRight != marginRight) {
        _marginRight = marginRight;
        _needsLayoutUpdate = true;
        _setNodeMargin();
    }
}

void Node::setMarginBottom(std::optional<NodeValue> const& marginBottom) {
    PROFILE

    if (_marginBottom != marginBottom) {
        _marginBottom = marginBottom;
        _needsLayoutUpdate = true;
        _setNodeMargin();
    }
}

void Node::setGap(std::optional<NodeValue> const& gap) {
    PROFILE

    if (_gap != gap) {
        _gap = gap;
        _needsLayoutUpdate = true;
        _setNodeGap();
    }
}

void Node::setGapX(std::optional<NodeValue> const& gapX) {
    PROFILE

    if (_gapX != gapX) {
        _gapX = gapX;
        _needsLayoutUpdate = true;
        _setNodeGap();
    }
}

void Node::setGapY(std::optional<NodeValue> const& gapY) {
    PROFILE

    if (_gapY != gapY) {
        _gapY = gapY;
        _needsLayoutUpdate = true;
        _setNodeGap();
    }
}

void Node::setBorderWidth(std::optional<float> const& borderWidth) {
    PROFILE

    if (_borderWidth != borderWidth) {
        _borderWidth = borderWidth;
        _needsLayoutUpdate = true;
        _setNodeBorder();
    }
}

void Node::setBorderTopWidth(std::optional<float> const& borderTopWidth) {
    PROFILE

    if (_borderTopWidth != borderTopWidth) {
        _borderTopWidth = borderTopWidth;
        _needsLayoutUpdate = true;
        _setNodeBorder();
    }
}

void Node::setBorderLeftWidth(std::optional<float> const& borderLeftWidth) {
    PROFILE

    if (_borderLeftWidth != borderLeftWidth) {
        _borderLeftWidth = borderLeftWidth;
        _needsLayoutUpdate = true;
        _setNodeBorder();
    }
}

void Node::setBorderRightWidth(std::optional<float> const& borderRightWidth) {
    PROFILE

    if (_borderRightWidth != borderRightWidth) {
        _borderRightWidth = borderRightWidth;
        _needsLayoutUpdate = true;
        _setNodeBorder();
    }
}

void Node::setBorderBottomWidth(std::optional<float> const& borderBottomWidth) {
    PROFILE

    if (_borderBottomWidth != borderBottomWidth) {
        _borderBottomWidth = borderBottomWidth;
        _needsLayoutUpdate = true;
        _setNodeBorder();
    }
}

void Node::setBorderRadius(std::optional<float> const& borderRadius) {
    PROFILE

    if (_borderRadius != borderRadius) {
        _borderRadius = borderRadius;
        _needsLayoutUpdate = true;
    }
}

void Node::setBorderTopLeftRadius(std::optional<float> const& borderTopLeftRadius) {
    PROFILE

    if (_borderTopLeftRadius != borderTopLeftRadius) {
        _borderTopLeftRadius = borderTopLeftRadius;
        _needsLayoutUpdate = true;
    }
}

void Node::setBorderTopRightRadius(std::optional<float> const& borderTopRightRadius) {
    PROFILE

    if (_borderTopRightRadius != borderTopRightRadius) {
        _borderTopRightRadius = borderTopRightRadius;
        _needsLayoutUpdate = true;
    }
}

void Node::setBorderBottomLeftRadius(std::optional<float> const& borderBottomLeftRadius) {
    PROFILE

    if (_borderBottomLeftRadius != borderBottomLeftRadius) {
        _borderBottomLeftRadius = borderBottomLeftRadius;
        _needsLayoutUpdate = true;
    }
}

void Node::setBorderBottomRightRadius(std::optional<float> const& borderBottomRightRadius) {
    PROFILE

    if (_borderBottomRightRadius != borderBottomRightRadius) {
        _borderBottomRightRadius = borderBottomRightRadius;
        _needsLayoutUpdate = true;
    }
}

void Node::setVisible(std::optional<bool> const& visible) {
    PROFILE

    if (_visible != visible) {
        _visible = visible;
        _needsLayoutUpdate = true;
    }
}

void Node::setZIndex(std::optional<int> const& zIndex) {
    PROFILE

    if (_zIndex != zIndex) {
        _zIndex = zIndex;
        _needsLayoutUpdate = true;
    }
}

void Node::setOffset(std::optional<Vec2> const& offset) {
    PROFILE

    if (_offset != offset) {
        _offset = offset;
        _needsLayoutUpdate = true;
    }
}

void Node::setOpacity(std::optional<float> const& opacity) {
    PROFILE

    if (_opacity != opacity) {
        _opacity = opacity;
        _needsLayoutUpdate = true;
    }
}

void Node::setTransform(std::optional<NodeTransform> const& transform) {
    PROFILE

    if (_transform != transform) {
        _transform = transform;
        _needsLayoutUpdate = true;
    }
}

void Node::setFontFamily(std::optional<std::string> const& fontFamily) {
    PROFILE

    if (_fontFamily != fontFamily) {
        _fontFamily = fontFamily;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setFontWeight(std::optional<FontWeight> const& fontWeight) {
    PROFILE

    if (_fontWeight != fontWeight) {
        _fontWeight = fontWeight;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setFontStyle(std::optional<FontStyle> const& fontStyle) {
    PROFILE

    if (_fontStyle != fontStyle) {
        _fontStyle = fontStyle;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setFontSize(std::optional<float> const& fontSize) {
    PROFILE

    if (_fontSize != fontSize) {
        _fontSize = fontSize;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setLineHeight(std::optional<float> const& lineHeight) {
    PROFILE

    if (_lineHeight != lineHeight) {
        _lineHeight = lineHeight;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setTextMarker(std::optional<Vec4> const& textMarker) {
    PROFILE

    if (_textMarker != textMarker) {
        _textMarker = textMarker;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setTextColor(std::optional<Vec4> const& textColor) {
    PROFILE

    if (_textColor != textColor) {
        _textColor = textColor;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setBackground(std::optional<Brush> const& background) {
    PROFILE

    if (_background != background) {
        _background = background;
        _needsLayoutUpdate = true;
    }
}

void Node::setForeground(std::optional<Brush> const& foreground) {
    PROFILE

    if (_foreground != foreground) {
        _foreground = foreground;
        _needsLayoutUpdate = true;
    }
}

void Node::setBorder(std::optional<Brush> const& border) {
    PROFILE

    if (_border != border) {
        _border = border;
        _needsLayoutUpdate = true;
    }
}

void Node::setShadow(std::optional<Shadow> const& shadow) {
    PROFILE

    if (_shadow != shadow) {
        _shadow = shadow;
        _needsLayoutUpdate = true;
        _shadowImageShadow = std::nullopt;
        _shadowImage = nullptr;
    }
}

void Node::setCursor(std::optional<Cursor> const& cursor) {
    PROFILE

    if (_cursor != cursor) {
        _cursor = cursor;
        _needsLayoutUpdate = true;
    }
}

void Node::setContent(std::optional<std::string> const& content) {
    PROFILE

    if (_content != content) {
        _content = content;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setContentEditable(bool editable) {
    PROFILE

    if (_contentEditable != editable) {
        _contentEditable = editable;
        _needsLayoutUpdate = true;

        if (
            (_document != nullptr) &&
            (_document->_focusedNode == this) &&
            (_firstChild != nullptr) &&
            (_firstChild->_textObject != nullptr)
        ) {
            _firstChild->_textObject->setEditable(editable);
            _firstChild->_needsTextUpdate = true;
        }
    }
}

void Node::setContentSecure(bool secure) {
    PROFILE

    if (_contentSecure != secure) {
        _contentSecure = secure;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
    }
}

void Node::setContentMultiLine(bool multiLine) {
    PROFILE

    _contentMultiLine = multiLine;
}

void Node::setTabIndex(int tabIndex) {
    PROFILE

    if (_tabIndex != tabIndex) {
        _tabIndex = tabIndex;
        _needsLayoutUpdate = true;
    }
}

void Node::setSkip(bool skip) {
    PROFILE

    if (_skip != skip) {
        _skip = skip;
        _needsTextUpdate = true;
        _needsLayoutUpdate = true;
        _updateLayout();
    }
}

void Node::setFlex(bool flex) {
    PROFILE

    if (_flex != flex) {
        _flex = flex;
        _needsLayoutUpdate = true;

        if (flex) {
            ::YGNodeStyleSetFlexGrow((::YGNode*)_layoutNode, 1.0f);
            ::YGNodeStyleSetFlexShrink((::YGNode*)_layoutNode, 1.0f);
            ::YGNodeStyleSetFlexBasis((::YGNode*)_layoutNode, ::YGUndefined);
        } else {
            ::YGNodeStyleSetFlexGrow((::YGNode*)_layoutNode, 0.0f);
            ::YGNodeStyleSetFlexShrink((::YGNode*)_layoutNode, 0.0f);
            ::YGNodeStyleSetFlexBasis((::YGNode*)_layoutNode, ::YGUndefined);
        }
    }
}

void Node::setKeyEvents(bool keyEvents) {
    PROFILE

    if (_keyEvents != keyEvents) {
        _keyEvents = keyEvents;
        _needsLayoutUpdate = true;
    }
}

void Node::setMouseEvents(bool mouseEvents) {
    PROFILE

    if (_mouseEvents != mouseEvents) {
        _mouseEvents = mouseEvents;
        _needsLayoutUpdate = true;
    }
}

void Node::setClipped(bool clipped) {
    PROFILE

    if (_clipped != clipped) {
        _clipped = clipped;
        _needsLayoutUpdate = true;
    }
}

bool Node::containsNode(Node const& node) {
    PROFILE

    for (auto next = &node; next != nullptr; next = next->_parent) {
        if (next == this) {
            return true;
        }
    }

    return false;
}

void Node::setParent(Node& parent) {
    PROFILE

    parent.appendChild(*this);
}

void Node::appendChild(Node& child) {
    PROFILE

    insertChild(child, std::numeric_limits<std::int64_t>::max());
}

void Node::insertChild(Node& child, std::int64_t index) {
    PROFILE

    if (&child == this) {
        return;
    }

    if (child.containsNode(*this)) {
        return;
    }

    if (child._parent == this) {
        auto position = 0ll;
        for (auto sibling = child._prevSibling; sibling != nullptr; sibling = sibling->_prevSibling) {
            position++;
        }

        auto count = position + 1;
        for (auto sibling = child._nextSibling; sibling != nullptr; sibling = sibling->_nextSibling) {
            count++;
        }

        if (std::clamp(index, 0ll, count - 1) == position) {
            return;
        }
    }

    if (child._parent != nullptr) {
        child._parent->removeChild(child);
    }

    auto before = _firstChild;
    auto position = 0ll;

    while (before != nullptr && position < index) {
        before = before->_nextSibling;
        position++;
    }

    child._parent = this;
    child._nextSibling = before;
    child._prevSibling = (before != nullptr) ? before->_prevSibling : _lastChild;

    if (child._prevSibling != nullptr) {
        child._prevSibling->_nextSibling = &child;
    } else {
        _firstChild = &child;
    }

    if (before != nullptr) {
        before->_prevSibling = &child;
    } else {
        _lastChild = &child;
    }

    ::YGNodeInsertChild((::YGNode*)_layoutNode, (::YGNode*)child._layoutNode, (std::size_t)position);

    if (_document != nullptr) {
        child._attach(*_document);
    }

    child._updateLayout();
    child._needsTextUpdate = true;
    child._needsLayoutUpdate = true;
}

void Node::removeChild(Node& child) {
    PROFILE

    if (child._parent != this) {
        return;
    }

    if (child._prevSibling != nullptr) {
        child._prevSibling->_nextSibling = child._nextSibling;
    } else {
        _firstChild = child._nextSibling;
    }

    if (child._nextSibling != nullptr) {
        child._nextSibling->_prevSibling = child._prevSibling;
    } else {
        _lastChild = child._prevSibling;
    }

    child._parent = nullptr;
    child._prevSibling = nullptr;
    child._nextSibling = nullptr;

    ::YGNodeRemoveChild((::YGNode*)_layoutNode, (::YGNode*)child._layoutNode);

    child._detach();
    child._updateLayout();

    _needsLayoutUpdate = true;
}

void Node::removeFromParent() {
    PROFILE

    if (_parent != nullptr) {
        _parent->removeChild(*this);
    }
}

void Node::removeAllChildren() {
    PROFILE

    while (_firstChild != nullptr) {
        removeChild(*_firstChild);
    }
}

void Node::triggerEvent(NodeEvent const& event, bool capture) {
    PROFILE

    if (capture) {
        onCaptureEvent.publish(event);
    } else {
        onEvent.publish(event);
    }
}

void Node::dispatchEvent(NodeEvent const& event) {
    PROFILE

    static auto const _dispatchCapture = [](Node& node, NodeEvent const& event, auto&& dispatchCapture) -> void {
        if (node._parent != nullptr) {
            dispatchCapture(*node._parent, event, dispatchCapture);
        }

        if (event._stopPropagation == false) {
            node.triggerEvent(event, true);
        }
    };

    static auto const _dispatchNormal = [](Node& node, NodeEvent const& event, auto&& dispatchNormal) -> void {
        node.triggerEvent(event, false);

        if (event._stopPropagation == false) {
            if (node._parent != nullptr) {
                dispatchNormal(*node._parent, event, dispatchNormal);
            }
        }
    };

    _dispatchCapture(*this, event, _dispatchCapture);
    _dispatchNormal(*this, event, _dispatchNormal);
}

void Node::broadcastEvent(NodeEvent const& event) {
    PROFILE

    triggerEvent(event);

    for (auto child = _firstChild; child != nullptr; child = child->_nextSibling) {
        child->broadcastEvent(event);
    }
}

void Node::_attach(Document& root) {
    PROFILE

    _document = &root;

    _resetTextState();
    _resetLayoutState();

    ::YGNodeSetConfig((::YGNode*)_layoutNode, (::YGConfig*)_document->_yogaConfig);

    if (_textNode != nullptr) {
        ::YGNodeSetConfig((::YGNode*)_textNode, (::YGConfig*)_document->_yogaConfig);
        ::YGNodeMarkDirty((::YGNode*)_textNode);
    }

    for (auto child = _firstChild; child != nullptr; child = child->_nextSibling) {
        child->_attach(root);
    }
}

void Node::_detach() {
    PROFILE

    for (auto child = _firstChild; child != nullptr; child = child->_nextSibling) {
        child->_detach();
    }

    _resetTextState();
    _resetLayoutState();
    _resetPaintState();
    _resetInputState();

    if (_document != nullptr) {
        std::erase(_document->_hoverPath, this);
        if (_document->_hoverNode == this) _document->_hoverNode = nullptr;
        if (_document->_activeNode == this) _document->_activeNode = nullptr;
        if (_document->_focusedNode == this) _document->_focusedNode = nullptr;
        _document = nullptr;
    }
}

void Node::_updateLayout() {
    PROFILE

    auto const display = _display.value_or(NodeDisplay::Box);

    auto isBoxLayout = false;
    auto isTextLayout = false;

    if (display == NodeDisplay::Box) {
        if (auto parent = _parent) {
            isBoxLayout = (parent->_display.value_or(NodeDisplay::Box) == NodeDisplay::Box);
            isTextLayout = false;
        } else {
            isBoxLayout = true;
            isTextLayout = false;
        }
    } else if (display == NodeDisplay::Text) {
        if (auto parent = _parent) {
            isTextLayout = (parent->_display.value_or(NodeDisplay::Box) == NodeDisplay::Box);
            isBoxLayout = false;
        } else {
            isTextLayout = true;
            isBoxLayout = false;
        }
    }

    if (_skip) {
        isBoxLayout = false;
        isTextLayout = false;
    }

    if (isTextLayout) {
        _createTextNode();
        ::YGNodeStyleSetDisplay((::YGNode*)_layoutNode, ::YGDisplayFlex);
    } else if (isBoxLayout) {
        _destroyTextNode();
        _resetTextState();
        ::YGNodeStyleSetDisplay((::YGNode*)_layoutNode, ::YGDisplayFlex);
    } else {
        _destroyTextNode();
        _resetTextState();
        ::YGNodeStyleSetDisplay((::YGNode*)_layoutNode, ::YGDisplayNone);
    }
}

void Node::_createTextNode() {
    PROFILE

    static auto const _countCodepoints = [](std::string const& string) -> std::int64_t {
        PROFILE

        auto count = std::int64_t(0);
        for (auto const byte : string) {
            if ((static_cast<unsigned char>(byte) & 0xC0) != 0x80) {
                count += 1;
            }
        }
        return count;
    };

    static auto const _collectFunc = [](Node& node, std::string& string, std::vector<TextStyleRange>& ranges, auto&& collectFunc) -> void {
        PROFILE

        if (node._display != NodeDisplay::Text) {
            return;
        }

        auto const startIndex = _countCodepoints(string);
        auto const rangeIndex = (std::ptrdiff_t)ranges.size();

        if (
            (node._content.has_value() == true) &&
            (node._content->empty() == false)
        ) {
            string += *node._content;
        }

        for (auto child = node._firstChild; child != nullptr; child = child->_nextSibling) {
            collectFunc(*child, string, ranges, collectFunc);
        }

        auto const endIndex = _countCodepoints(string);
        ranges.insert(ranges.begin() + rangeIndex, TextStyleRange{
            .startIndex = startIndex,
            .endIndex = endIndex,
            .style = TextStyle{
                .fontFamily = node._computedFontFamily,
                .fontWeight = node._computedFontWeight,
                .fontStyle  = node._computedFontStyle,
                .fontSize   = node._computedFontSize,
                .color      = node._computedTextColor,
                .marker     = node._computedMarkerColor
            }
        });
    };

    static auto const _measureFunc = [](::YGNodeConstRef node, float width, ::YGMeasureMode widthMode, float height, ::YGMeasureMode heightMode) -> ::YGSize {
        PROFILE

        auto textNode = (Node*)::YGNodeGetContext(node);
        if (textNode == nullptr) {
            return ::YGSize{ 0.0f, 0.0f };
        }

        auto const parent = textNode->_parent;
        auto const editableParent = (
            (parent != nullptr) &&
            (parent->_firstChild == textNode) &&
            (parent->_contentEditable == true) &&
            (parent->_display.value_or(NodeDisplay::Box) == NodeDisplay::Box)
        );

        if (textNode->_textObject == nullptr) {
            textNode->_textObject = std::make_unique<Text>();

            /* a text child swapped in under a focused editable box is the editing surface from now on */
            if (editableParent && (textNode->_document != nullptr) && (textNode->_document->_focusedNode == parent)) {
                textNode->_textObject->setEditable(true);
            }
        }

        textNode->_textObject->setMultiLine(editableParent ? std::optional<bool>(parent->_contentMultiLine) : std::nullopt);

        auto const scale = (textNode->_document != nullptr) ? textNode->_document->_scale : 1.0f;

        auto string = std::string();
        auto ranges = std::vector<TextStyleRange>();

        _collectFunc(*textNode, string, ranges, _collectFunc);

        textNode->_textObject->setScale(scale);
        textNode->_textObject->setSecure(textNode->_contentSecure);
        textNode->_textObject->setString(string);
        textNode->_textObject->setStyles(ranges);

        if (widthMode == ::YGMeasureModeUndefined) {
            textNode->_textObject->setWidth(std::nullopt);
            textNode->_textObject->setMaxWidth(std::nullopt);
        } else if (widthMode == ::YGMeasureModeExactly) {
            textNode->_textObject->setWidth(std::floorf(width * scale));
            textNode->_textObject->setMaxWidth(std::nullopt);
        } else if (widthMode == ::YGMeasureModeAtMost) {
            textNode->_textObject->setWidth(std::nullopt);
            textNode->_textObject->setMaxWidth(std::floorf(width * scale));
        }

        if (heightMode == ::YGMeasureModeUndefined) {
            textNode->_textObject->setHeight(std::nullopt);
            textNode->_textObject->setMaxHeight(std::nullopt);
        } else if (heightMode == ::YGMeasureModeExactly) {
            textNode->_textObject->setHeight(std::floorf(height * scale));
            textNode->_textObject->setMaxHeight(std::nullopt);
        } else if (heightMode == ::YGMeasureModeAtMost) {
            textNode->_textObject->setHeight(std::nullopt);
            textNode->_textObject->setMaxHeight(std::floorf(height * scale));
        }

        return ::YGSize{
            std::ceilf(textNode->_textObject->getSize().width / scale),
            std::ceilf(textNode->_textObject->getSize().height / scale)
        };
    };

    if (_textNode == nullptr) {
        _textNode = ::YGNodeNew();
        ::YGNodeSetContext((::YGNode*)_textNode, this);
        ::YGNodeSetNodeType((::YGNode*)_textNode, ::YGNodeTypeText);
        ::YGNodeSetMeasureFunc((::YGNode*)_textNode, _measureFunc);
        ::YGNodeStyleSetFlexGrow((::YGNode*)_textNode, 0.0f);
        ::YGNodeStyleSetFlexShrink((::YGNode*)_textNode, 0.0f);

        if (_document != nullptr) {
            ::YGNodeSetConfig((::YGNode*)_textNode, (::YGConfig*)_document->_yogaConfig);
        }

        ::YGNodeInsertChild((::YGNode*)_layoutNode, (::YGNode*)_textNode, 0);
    }
}

void Node::_destroyTextNode() {
    PROFILE

    if (_textNode != nullptr) {
        if (auto parent = ::YGNodeGetParent((::YGNode*)_textNode)) {
            ::YGNodeRemoveChild(parent, (::YGNode*)_textNode);
        }

        ::YGNodeFree((::YGNode*)_textNode);
        _textNode = nullptr;
    }
}

void Node::_setNodePadding() {
    PROFILE

    auto const apply = [&](::YGEdge edge, std::optional<NodeValue> const& value) {
        auto const& padding = value.has_value() ? value : _padding;

        if (padding.has_value()) {
            padding->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetPadding((::YGNode*)_layoutNode, edge, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetPaddingPercent((::YGNode*)_layoutNode, edge, pct.value); }
            );
        } else {
            ::YGNodeStyleSetPadding((::YGNode*)_layoutNode, edge, ::YGUndefined);
        }
    };

    apply(::YGEdgeTop, _paddingTop);
    apply(::YGEdgeLeft, _paddingLeft);
    apply(::YGEdgeRight, _paddingRight);
    apply(::YGEdgeBottom, _paddingBottom);
}

void Node::_setNodeMargin() {
    PROFILE

    auto const apply = [&](::YGEdge edge, std::optional<NodeValue> const& value) {
        auto const& margin = value.has_value() ? value : _margin;

        if (margin.has_value()) {
            margin->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetMargin((::YGNode*)_layoutNode, edge, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetMarginPercent((::YGNode*)_layoutNode, edge, pct.value); }
            );
        } else {
            ::YGNodeStyleSetMargin((::YGNode*)_layoutNode, edge, ::YGUndefined);
        }
    };

    apply(::YGEdgeTop, _marginTop);
    apply(::YGEdgeLeft, _marginLeft);
    apply(::YGEdgeRight, _marginRight);
    apply(::YGEdgeBottom, _marginBottom);
}

void Node::_setNodeGap() {
    PROFILE

    auto const apply = [&](::YGGutter gutter, std::optional<NodeValue> const& value) {
        auto const& gap = value.has_value() ? value : _gap;

        if (gap.has_value()) {
            gap->match(
                [&](PixelValue const& px) { ::YGNodeStyleSetGap((::YGNode*)_layoutNode, gutter, px.value); },
                [&](PercentValue const& pct) { ::YGNodeStyleSetGapPercent((::YGNode*)_layoutNode, gutter, pct.value); }
            );
        } else {
            ::YGNodeStyleSetGap((::YGNode*)_layoutNode, gutter, ::YGUndefined);
        }
    };

    apply(::YGGutterColumn, _gapX);
    apply(::YGGutterRow, _gapY);
}

void Node::_setNodeBorder() {
    PROFILE

    auto const apply = [&](::YGEdge edge, std::optional<float> const& value) {
        auto const& width = value.has_value() ? value : _borderWidth;

        ::YGNodeStyleSetBorder((::YGNode*)_layoutNode, edge, width.value_or(::YGUndefined));
    };

    apply(::YGEdgeTop, _borderTopWidth);
    apply(::YGEdgeLeft, _borderLeftWidth);
    apply(::YGEdgeRight, _borderRightWidth);
    apply(::YGEdgeBottom, _borderBottomWidth);
}

void Node::_resetTextState() {
    PROFILE

    _computedFontFamily  = TEXT_DEFAULT_FONT_FAMILY;
    _computedFontWeight  = TEXT_DEFAULT_FONT_WEIGHT;
    _computedFontStyle   = TEXT_DEFAULT_FONT_STYLE;
    _computedFontSize    = TEXT_DEFAULT_FONT_SIZE;
    _computedTextColor   = TEXT_DEFAULT_COLOR;
    _computedMarkerColor = {};
    _computedLineHeight  = TEXT_DEFAULT_LINE_HEIGHT;
    _textObject          = nullptr;
    _needsTextUpdate     = true;
    _textScrollX         = 0.0f;
}

void Node::_resetLayoutState() {
    PROFILE

    _computedTextRect             = {};
    _computedBorderEdge           = {};
    _computedBorderRect           = {};
    _computedMarginRect           = {};
    _computedContentRect          = {};
    _scrollOverflow               = {};
    _scrollPosition               = {};
    _computedBorderRectInDocument = {};
    _computedMarginRectInDocument = {};
    _computedClipRectInDocument   = {};
    _computedZIndex               = 0;
    _needsLayoutUpdate            = true;
}

void Node::_resetPaintState() {
    PROFILE

    _layerImage          = nullptr;
    _shadowImage         = nullptr;
    _shadowImageShadow   = std::nullopt;
    _shadowImageRadius   = std::nullopt;
    _shadowImageScale    = std::nullopt;
    _shadowImageClipRect = std::nullopt;
}

void Node::_resetInputState() {
    PROFILE

    _isHover         = false;
    _isActive        = false;
    _isFocused       = false;
    _isFocusedWithin = false;
}

} /* namespace Rocket */
