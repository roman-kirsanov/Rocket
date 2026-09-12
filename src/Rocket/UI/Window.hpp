#pragma once

#include <string>
#include <optional>
#include <functional>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

/**
 * Declarative props for Window.
 *
 * Rendering Window owns a native window for as long as the call site stays
 * mounted, and syncs it to these props on every render. Boolean fields left
 * unset default to false (so an unset `visible` keeps the window hidden);
 * size and position are left at the native window's own default until first
 * given a value.
 */
struct WindowProps {
    /** When non-null, filled with a pointer to the underlying Window while mounted; reset to nullptr on unmount or when a different ref is passed. */
    class Window** ref = nullptr;

    /** Reconciliation key for this component instance (see COMPONENT). */
    std::string key;

    /** Window title (see Window::setTitle). */
    std::string title;

    /** Content-area size in points; unset keeps the native default (see Window::setSize). */
    std::optional<Vec2> size;

    /** Frame top-left position in points, top-left-origin screen coordinates; unset keeps the native default (see Window::setPosition). */
    std::optional<Vec2> position;

    /** Shows a close control (default false; see Window::setClosable). */
    std::optional<bool> closable;

    /** Allows user resizing (default false; see Window::setSizable). */
    std::optional<bool> sizable;

    /** Allows maximizing (default false; see Window::setMaximizable). */
    std::optional<bool> maximizable;

    /** Allows minimizing (default false; see Window::setMinimizable). */
    std::optional<bool> minimizable;

    /** Keeps the window above normal windows (default false; see Window::setTopmost). */
    std::optional<bool> topmost;

    /** Shows the window (default false, i.e. hidden; see Window::setVisible). */
    std::optional<bool> visible;

    /** Receives every WindowEvent of the underlying window (see Window::onEvent). */
    std::function<void(WindowEvent const&)> onEvent;
};

/**
 * Renders a window: mounts a native Window on first render, keeps it in
 * sync with `props` on every render, and unmounts (destroying it) when this
 * call site stops rendering.
 *
 * Must be called during a render (see COMPONENT). Exposes the window as a
 * context via SetContext, so `children` and its descendants can retrieve it
 * with UseContext<Window>().
 *
 * @param props    Desired window configuration for this render.
 * @param children Rendered inside the window's context; default no-op.
 */
void Window(WindowProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */