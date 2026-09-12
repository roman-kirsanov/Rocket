#pragma once

#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Math/Vec4.hpp>
#include <Rocket/Base/Object.hpp>
#include <Rocket/Base/PubSub.hpp>
#include <Rocket/Window/Cursor.hpp>
#include <Rocket/Window/WindowEvent.hpp>

namespace Rocket {

/**
 * A native top-level window.
 *
 * The underlying platform window is created by the constructor (initially
 * hidden) and destroyed by the destructor. Every window registers itself in
 * the process-wide list returned by GetWindows. Non-copyable and
 * non-movable. Subclasses may override _onEvent, which sees each event
 * before onEvent subscribers do.
 */
class Window {
public:
    /** Event source for this window (see the WindowEvent subclasses). */
    Pub<WindowEvent const&> onEvent;

    /** Destroys the native window and removes it from GetWindows. */
    ~Window();

    /** Creates a hidden native window and adds it to GetWindows. */
    Window();

    Window(Window &&) = delete;
    Window(Window const&) = delete;
    Window& operator=(Window &&) = delete;
    Window& operator=(Window const&) = delete;

    /** Returns the window title. */
    std::string const& getTitle() const;

    /** Returns the mouse cursor shape shown over the window's content area; Cursor::Default initially. */
    Cursor getCursor() const;

    /** Returns the position of the window frame's top-left corner, in points, in a top-left-origin coordinate system of the window's screen (the primary screen while hidden or off-screen). */
    Vec2 const& getPosition() const;

    /** Returns the size of the window's content area in points. */
    Vec2 const& getSize() const;

    /** Returns the backing scale factor (e.g. 2.0 on Retina displays). */
    float getScale() const;

    /** Returns the native window handle (NSWindow* on macOS; an HWND is intended on Windows, which has no backend yet). */
    void* getHandle() const;

    /** Returns whether the window is currently visible. */
    bool getVisible() const;

    /** Returns whether the window shows a close control. */
    bool getClosable() const;

    /** Returns whether the window is user-resizable. */
    bool getSizable() const;

    /** Returns whether maximizing is allowed (true until setMaximizable(false); see setMaximizable). */
    bool getMaximizable() const;

    /** Returns whether the window can be minimized. */
    bool getMinimizable() const;

    /** Returns whether the window stays above normal windows. */
    bool getTopmost() const;

    /** Returns whether the window is currently maximized (full-screen on macOS; a zoomed but not full-screen window reports false). */
    bool isMaximized() const;

    /** Returns whether the window is currently minimized. */
    bool isMinimized() const;

    /** Returns whether the mouse is currently over the window's content area (tracks MouseEnter/MouseExitWindowEvent). */
    bool isHover() const;

    /** Returns whether the window has keyboard focus. */
    bool isFocused() const;

    /** Centers the window on its screen. */
    void center() const;

    /** Marks the native content view as needing display. Note: painting is driven by the display link (PaintWindowEvent fires every vsync regardless), so on macOS this has no observable effect. */
    void paint() const;

    /**
     * Moves the window so its frame's top-left corner lands at `position`.
     *
     * @param position The new position, in points, in a top-left-origin coordinate system of the window's screen (the primary screen while hidden or off-screen).
     */
    void setPosition(Vec2 const& position);

    /**
     * Resizes the window's content area, in points.
     *
     * @param size The new content area size, in points.
     */
    void setSize(Vec2 const& size);

    /**
     * Sets the window title.
     *
     * @param title The new window title.
     */
    void setTitle(std::string const& title);

    /**
     * Sets the mouse cursor shape shown while the pointer is over the window's content area.
     *
     * Cursor::None hides the cursor application-wide (not per window) until
     * another cursor is set or the window is destroyed. Values without a
     * native equivalent fall back to the closest available system cursor.
     *
     * @param cursor The new cursor shape.
     */
    void setCursor(Cursor cursor);

    /**
     * Shows or hides the window; showing also makes it the key window.
     * Publishes ShowWindowEvent / HideWindowEvent on change.
     *
     * @param visible True to show the window, false to hide it.
     */
    void setVisible(bool visible);

    /**
     * Enables or disables the close control.
     *
     * @param closable True to show a close control, false to hide it.
     */
    void setClosable(bool closable);

    /**
     * Enables or disables user resizing.
     *
     * @param sizable True to allow user resizing, false to disallow it.
     */
    void setSizable(bool sizable);

    /**
     * Enables or disables maximizing (on macOS: the title-bar zoom control;
     * setMaximized is unaffected).
     *
     * @param maximizable True to allow maximizing, false to disallow it.
     */
    void setMaximizable(bool maximizable);

    /**
     * Enables or disables minimizing.
     *
     * @param minimizable True to allow minimizing, false to disallow it.
     */
    void setMinimizable(bool minimizable);

    /**
     * Maximizes or restores the window (toggles full-screen mode on macOS).
     *
     * @param maximized True to maximize the window, false to restore it.
     */
    void setMaximized(bool maximized);

    /**
     * Minimizes or restores the window.
     *
     * @param minimized True to minimize the window, false to restore it.
     */
    void setMinimized(bool minimized);

    /**
     * Keeps the window above normal windows (or stops doing so).
     *
     * @param topmost True to keep the window above normal windows, false to stop.
     */
    void setTopmost(bool topmost);

    /** Gives the window keyboard focus and orders it to the front. On a hidden window this also shows it, without publishing ShowWindowEvent. */
    void setFocus();

    /** Runs a modal session for this window, blocking until stopModal is called. App timeouts, intervals and UpdateAppEvent do not run while the session is active. */
    void runModal();

    /** Ends the modal session started by runModal. */
    void stopModal();

    /** Returns the currently focused window, or nullptr if none is focused. */
    static Window* GetFocusedWindow();

    /** Returns all windows currently alive in this process, in creation order. */
    static std::vector<Window*> const& GetWindows();
protected:
    /** Called for every event of this window, before onEvent subscribers. */
    virtual void _onEvent(WindowEvent const&) {};
private:
    struct _Window;

    std::string _title;
    Cursor _cursor;
    _Window* _impl;

    void _show();
    void _hide();
    void _close();
    void _resize();
    void _maximize();
    void _minimize();
    void _demaximize();
    void _deminimize();
    void _dpiChange();
    void _mouseMove(Vec2 const&, KeyModifiers const&);
    void _mouseEnter(Vec2 const&, KeyModifiers const&);
    void _mouseExit(Vec2 const&, KeyModifiers const&);
    void _mouseWheel(Vec2 const&, KeyModifiers const&);
    void _mouseDown(Mouse const&, Vec2 const&, KeyModifiers const&, int clickCount = 1);
    void _mouseUp(Mouse const&, Vec2 const&, KeyModifiers const&);
    void _keyDown(Key const&, KeyModifiers const&, std::string const&);
    void _keyUp(Key const&, KeyModifiers const&);
    void _focus();
    void _blur();
    void _paint();

    void __init();
    void __done();
    Vec2 const& __getSize() const;
    Vec2 const& __getPosition() const;
    float __getScale() const;
    void* __getHandle() const;
    bool __getVisible() const;
    bool __getClosable() const;
    bool __getSizable() const;
    bool __getMaximizable() const;
    bool __getMinimizable() const;
    bool __getTopmost() const;
    bool __isMaximized() const;
    bool __isMinimized() const;
    bool __isHover() const;
    bool __isFocused() const;
    void __center() const;
    void __paint() const;
    void __setPosition(Vec2 const&);
    void __setSize(Vec2 const&);
    void __setTitle(std::string const&);
    void __setCursor(Cursor);
    void __setVisible(bool);
    void __setClosable(bool);
    void __setSizable(bool);
    void __setMaximizable(bool);
    void __setMinimizable(bool);
    void __setMaximized(bool);
    void __setMinimized(bool);
    void __setTopmost(bool);
    void __setFocus();
    void __runModal();
    void __stopModal();

#ifdef _WIN32
    static std::intptr_t __stdcall __WindowProc(void*, std::uint32_t, std::uintptr_t, std::intptr_t);
#endif
};

} /* namespace Rocket */
