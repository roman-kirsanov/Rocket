#include <vector>
#include <cassert>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

static auto _windows = std::vector<Window*>();

static void _AddWindow(Window* window) {
    PROFILE

    assert(window != nullptr);

    _windows.push_back(window);
}

static void _RemoveWindow(Window* window) {
    PROFILE

    assert(window != nullptr);

    std::erase(_windows, window);
}

Window::~Window() {
    PROFILE

    _RemoveWindow(this);

    __done();
}

Window::Window()
    : onEvent()
    , _title()
    , _cursor(Cursor::Default)
    , _impl(nullptr)
{
    PROFILE

    __init();

    _AddWindow(this);
}

std::string const& Window::getTitle() const {
    PROFILE

    return _title;
}

Cursor Window::getCursor() const {
    PROFILE

    return _cursor;
}

Vec2 const& Window::getPosition() const {
    PROFILE

    return __getPosition();
}

Vec2 const& Window::getSize() const {
    PROFILE

    return __getSize();
}

float Window::getScale() const {
    PROFILE

    return __getScale();
}

void* Window::getHandle() const {
    PROFILE

    return __getHandle();
}

bool Window::getVisible() const {
    PROFILE

    return __getVisible();
}

bool Window::getClosable() const {
    PROFILE

    return __getClosable();
}

bool Window::getSizable() const {
    PROFILE

    return __getSizable();
}

bool Window::getMaximizable() const {
    PROFILE

    return __getMaximizable();
}

bool Window::getMinimizable() const {
    PROFILE

    return __getMinimizable();
}

bool Window::isMaximized() const {
    PROFILE

    return __isMaximized();
}

bool Window::isMinimized() const {
    PROFILE

    return __isMinimized();
}

bool Window::getTopmost() const {
    PROFILE

    return __getTopmost();
}

bool Window::isHover() const {
    PROFILE

    return __isHover();
}

bool Window::isFocused() const {
    PROFILE

    return __isFocused();
}

void Window::center() const {
    PROFILE

    __center();
}

void Window::paint() const {
    PROFILE

    __paint();
}

void Window::setPosition(Vec2 const& position) {
    PROFILE

    __setPosition(position);
}

void Window::setSize(Vec2 const& size) {
    PROFILE

    __setSize(size);
}

void Window::setTitle(std::string const& title) {
    PROFILE

    _title = title;
    __setTitle(title);
}

void Window::setCursor(Cursor cursor) {
    PROFILE

    _cursor = cursor;
    __setCursor(cursor);
}

void Window::setVisible(bool visible) {
    PROFILE

    __setVisible(visible);
}

void Window::setClosable(bool closable) {
    PROFILE

    __setClosable(closable);
}

void Window::setSizable(bool sizable) {
    PROFILE

    __setSizable(sizable);
}

void Window::setMaximizable(bool maximizable) {
    PROFILE

    __setMaximizable(maximizable);
}

void Window::setMinimizable(bool minimizable) {
    PROFILE

    __setMinimizable(minimizable);
}

void Window::setMaximized(bool maximized) {
    PROFILE

    __setMaximized(maximized);
}

void Window::setMinimized(bool minimized) {
    PROFILE

    __setMinimized(minimized);
}

void Window::setTopmost(bool topmost) {
    PROFILE

    __setTopmost(topmost);
}

void Window::setFocus() {
    PROFILE

    __setFocus();
}

void Window::runModal() {
    PROFILE

    __runModal();
}

void Window::stopModal() {
    PROFILE

    __stopModal();
}

Window* Window::GetFocusedWindow() {
    PROFILE

    for (auto& window : _windows) {
        if (window->isFocused()) {
            return window;
        }
    }

    return nullptr;
}

std::vector<Window*> const& Window::GetWindows() {
    PROFILE

    return _windows;
}

void Window::_show() {
    PROFILE

    auto event = ShowWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_hide() {
    PROFILE

    auto event = HideWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_close() {
    PROFILE

    auto event = CloseWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_resize() {
    PROFILE

    auto event = ResizeWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_maximize() {
    PROFILE

    auto event = MaximizeWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_minimize() {
    PROFILE

    auto event = MinimizeWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_demaximize() {
    PROFILE

    auto event = DemaximizeWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_deminimize() {
    PROFILE

    auto event = DeminimizeWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_dpiChange() {
    PROFILE

    auto event = DPIChangeWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_mouseMove(Vec2 const& position, KeyModifiers const& modifiers) {
    PROFILE

    auto event = MouseMoveWindowEvent(*this, position, modifiers);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_mouseEnter(Vec2 const& position, KeyModifiers const& modifiers) {
    PROFILE

    auto event = MouseEnterWindowEvent(*this, position, modifiers);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_mouseExit(Vec2 const& position, KeyModifiers const& modifiers) {
    PROFILE

    auto event = MouseExitWindowEvent(*this, position, modifiers);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_mouseWheel(Vec2 const& wheel, KeyModifiers const& modifiers) {
    PROFILE

    auto event = MouseWheelWindowEvent(*this, wheel, modifiers);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_mouseDown(Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers, int clickCount) {
    PROFILE

    auto event = MouseDownWindowEvent(*this, mouse, position, modifiers, clickCount);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_mouseUp(Mouse const& mouse, Vec2 const& position, KeyModifiers const& modifiers) {
    PROFILE

    auto event = MouseUpWindowEvent(*this, mouse, position, modifiers);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_keyDown(Key const& key, KeyModifiers const& modifiers, std::string const& input) {
    PROFILE

    auto event = KeyDownWindowEvent(*this, key, modifiers, input);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_keyUp(Key const& key, KeyModifiers const& modifiers) {
    PROFILE

    auto event = KeyUpWindowEvent(*this, key, modifiers);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_focus() {
    PROFILE

    auto event = FocusWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_blur() {
    PROFILE

    auto event = BlurWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

void Window::_paint() {
    PROFILE

    auto event = PaintWindowEvent(*this);

    _onEvent(event);
    onEvent.publish(event);
}

} /* namespace Rocket */
