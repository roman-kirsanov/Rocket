/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Node/Node.hpp>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/Window/Input.hpp>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

struct _Interface {
    class Window window;
    class Document document;
    Reconciler reconciler;

    _Interface(std::function<void()> const& content);

    void render();
};

std::vector<class Node*> _FindNodes(class Node* root, std::function<bool(class Node const&)> const& matches);
std::vector<class Node*> _FindClickableNodes(class Node* root);
std::vector<std::string> _FindTexts(class Node* root);
void _Click(class Node* node);
void _PressKey(class Node* node, Scancode scancode, std::string_view keycode);

inline _Interface::_Interface(std::function<void()> const& content)
    : window()
    , document(window) {
    window.setSize({ 640.0f, 480.0f });

    reconciler.setUpdateFn([&, content]{
        Context(document, content);
    });
}

inline void _Interface::render() {
    reconciler.update();
    document.update();
}

inline std::vector<class Node*> _FindNodes(class Node* root, std::function<bool(class Node const&)> const& matches) {
    auto nodes = std::vector<class Node*>();

    if (root == nullptr) {
        return nodes;
    }

    if (matches(*root)) {
        nodes.push_back(root);
    }

    for (auto child = root->getFirstChild(); child != nullptr; child = child->getNextSibling()) {
        auto const found = _FindNodes(child, matches);

        nodes.insert(nodes.end(), found.begin(), found.end());
    }

    return nodes;
}

inline std::vector<class Node*> _FindClickableNodes(class Node* root) {
    return _FindNodes(root, [](class Node const& node) {
        return (node.getCursor() == Cursor::Pointer);
    });
}

inline std::vector<std::string> _FindTexts(class Node* root) {
    auto texts = std::vector<std::string>();

    for (auto const node : _FindNodes(root, [](class Node const& node) { return node.getContent().has_value(); })) {
        texts.push_back(node->getContent().value());
    }

    return texts;
}

inline void _Click(class Node* node) {
    node->triggerEvent(MouseClickNodeEvent(*node, { 0.0f, 0.0f }, KeyModifiers{}));
}

inline void _PressKey(class Node* node, Scancode scancode, std::string_view keycode) {
    node->triggerEvent(KeyDownNodeEvent(*node, scancode, std::string(keycode), KeyModifiers{}));
}

} /* namespace Rocket */
