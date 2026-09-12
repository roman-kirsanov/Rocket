/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/**
 * Context a parent (Node or Document component) exposes to the Node
 * components rendered inside it, so each child can find its retained parent
 * and its position among its siblings. Each child inserts itself at its
 * index (Node::insertChild) whenever that index changes, which keeps the
 * retained order equal to the render order. The provider resets
 * `childNextIndex` at the start of each of its renders.
 */
struct _NodeContext {
    /** Retained parent node, or nullptr when children parent to the Document. */
    class Node* node = nullptr;
    /** Index handed to the next child rendered in this pass. */
    std::int64_t childNextIndex = 0;
};

} /* namespace Rocket */
