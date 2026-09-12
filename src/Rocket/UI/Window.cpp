#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Window.hpp>

namespace Rocket {

void Window(WindowProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    struct _State {
        class Window** ref = nullptr;
        class Window window;
        Effect<std::string> shouldUpdateTitle;
        Effect<std::optional<Vec2>> shouldUpdateSize;
        Effect<std::optional<Vec2>> shouldUpdatePosition;
        Effect<std::optional<bool>> shouldUpdateClosable;
        Effect<std::optional<bool>> shouldUpdateSizable;
        Effect<std::optional<bool>> shouldUpdateMaximizable;
        Effect<std::optional<bool>> shouldUpdateMinimizable;
        Effect<std::optional<bool>> shouldUpdateTopmost;
        Effect<std::optional<bool>> shouldUpdateVisible;
        Sub<WindowEvent const&> eventSub;
        std::function<void(WindowEvent const&)> onEvent;

        _State() {
            PROFILE

            eventSub.on(window.onEvent, [this](WindowEvent const& event) {
                if (onEvent) {
                    onEvent(event);
                }
            });
        }

        ~_State() {
            PROFILE

            if (ref != nullptr) {
                *ref = nullptr;
            }
        }
    };

    auto& state = UseState<_State>();

    state.onEvent = props.onEvent;

    if (state.ref != props.ref) {
        if (state.ref != nullptr) {
            *state.ref = nullptr;
        }

        state.ref = props.ref;

        if (state.ref != nullptr) {
            *state.ref = &state.window;
        }
    }

    if (state.shouldUpdateTitle(props.title)) state.window.setTitle(props.title);
    if (state.shouldUpdateSize(props.size) && props.size.has_value()) state.window.setSize(props.size.value());
    if (state.shouldUpdatePosition(props.position) && props.position.has_value()) state.window.setPosition(props.position.value());
    if (state.shouldUpdateClosable(props.closable)) state.window.setClosable(props.closable.value_or(false));
    if (state.shouldUpdateSizable(props.sizable)) state.window.setSizable(props.sizable.value_or(false));
    if (state.shouldUpdateMaximizable(props.maximizable)) state.window.setMaximizable(props.maximizable.value_or(false));
    if (state.shouldUpdateMinimizable(props.minimizable)) state.window.setMinimizable(props.minimizable.value_or(false));
    if (state.shouldUpdateTopmost(props.topmost)) state.window.setTopmost(props.topmost.value_or(false));
    if (state.shouldUpdateVisible(props.visible)) state.window.setVisible(props.visible.value_or(false));

    SetContext(state.window);

    if (children) {
        children();
    }
}

} /* namespace Rocket */