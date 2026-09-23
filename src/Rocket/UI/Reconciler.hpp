#pragma once

#include <any>
#include <memory>
#include <format>
#include <cstdint>
#include <typeindex>
#include <functional>
#include <Rocket/Base/Effect.hpp>
#include <Rocket/Base/PubSub.hpp>
#include <Rocket/Base/Profile.hpp>

/**
 * Opens a component scope for the enclosing function, keyed by `props.key`.
 *
 * Place at the top of a component function (which must have a `props` argument
 * with a `key` field). Establishes the reconciliation scope that the Use*
 * hooks operate on, and closes it automatically when the function returns.
 */
#define COMPONENT \
    static auto __componentName = std::string(__func__); \
    static auto __componentType = ::Rocket::GetComponentType(__componentName); \
    auto __componentScope = ::Rocket::_ComponentScope(__componentType, __componentName, props.key);

/** Like COMPONENT, but for component functions that take no key (identity by name plus order among same-named siblings). */
#define COMPONENT_NO_KEY \
    static auto __componentName = std::string(__func__); \
    static auto __componentType = ::Rocket::GetComponentType(__componentName); \
    auto __componentScope = ::Rocket::_ComponentScope(__componentType, __componentName, "");

namespace Rocket {

/** RAII scope that opens a component on construction and closes it on destruction (used by COMPONENT). */
struct _ComponentScope {
    /**
     * Opens the component on the current reconciler.
     *
     * @param type The component's registered type id (from GetComponentType).
     * @param name The component's function name.
     * @param key  The reconciliation key ("" for keyless components).
     */
    _ComponentScope(std::int64_t type, std::string const& name, std::string const& key);
    ~_ComponentScope();
};

/**
 * Selects when a reconciler re-renders.
 *
 * Note: the mode is currently stored and reported only; update() always
 * re-renders regardless of mode.
 */
enum ReconcilerUpdateMode {
    OnUpdate,
    OnRequest
};

/**
 * Owns a tree of components and reconciles it against the update function.
 *
 * Each update() call re-runs the update function; components keep their
 * state across renders, newly rendered components mount, and components
 * absent from a render unmount with their state destroyed. A component is
 * matched among its parent's children by its type plus its key when keyed,
 * or by its type plus its ordinal among same-named siblings when keyless.
 * Rendering two keyed siblings with the same key under one parent throws
 * std::runtime_error, whatever their types. Non-copyable and non-movable.
 */
class Reconciler {
public:
    /**
     * Published once after update() finishes a complete reconciliation (all
     * passes settled and stale components unmounted), before update() returns.
     */
    Pub<> onAfterUpdate;

    /** Unmounts every component and destroys all component state. */
    ~Reconciler();

    /** Creates an empty reconciler: no update function, mode OnUpdate. */
    Reconciler();
    Reconciler(Reconciler &&) = delete;
    Reconciler(Reconciler const&) = delete;
    Reconciler& operator=(Reconciler &&) = delete;
    Reconciler& operator=(Reconciler const&) = delete;

    /** Returns the current update mode (see ReconcilerUpdateMode). */
    ReconcilerUpdateMode getUpdateMode() const;

    /**
     * Sets the update mode (see ReconcilerUpdateMode).
     *
     * @param updateMode The new update mode.
     */
    void setUpdateMode(ReconcilerUpdateMode updateMode);

    /**
     * Sets the function that renders the component tree on each update().
     *
     * @param updateFn The function that renders the component tree.
     */
    void setUpdateFn(std::function<void()> const& updateFn);

    /**
     * Marks the component tree as needing another render.
     *
     * This is the declarative way for a component (via
     * UseContext<Reconciler>()) to request a re-render after changing state.
     * When called during a render, the running update() performs one
     * follow-up render after the current pass completes; any number of
     * requests within a pass coalesce into that single follow-up. A
     * component that requests an update on every render never settles;
     * update() stops after 5 passes and returns with the request still
     * pending, so gate such requests (e.g. behind UseEffect).
     *
     * When called outside a render the request currently has no effect,
     * because update() always renders unconditionally; the flag is the
     * intended driver for ReconcilerUpdateMode::OnRequest.
     */
    void needsUpdate();

    /**
     * Renders the component tree once.
     *
     * Runs the update function with this reconciler as the innermost active
     * one (so hooks resolve to it) and provides the reconciler itself as a
     * root context, reachable via UseContext<Reconciler>() from any
     * component. Afterwards unmounts every component that was not rendered
     * this pass. Nested update() calls on other reconcilers are allowed.
     *
     * Calling update() re-entrantly on the same reconciler (e.g. from an
     * event published synchronously during a render) does not start a nested
     * pass; it behaves exactly like needsUpdate(): the request coalesces
     * into a single follow-up render after the current pass completes.
     * Prefer needsUpdate() when requesting a re-render from inside the tree.
     */
    void update();
private:
    struct _Component;

    ReconcilerUpdateMode _updateMode;
    std::function<void()> _updateFn;
    _Component* _rootComponent;
    _Component* _currentComponent;
    bool _isUpdating;
    bool _needsUpdate;

    std::any& _useState();
    void* _useContext(std::type_index const&);
    std::int64_t const& _useComponentIndex();
    std::string const& _useComponentName();
    std::string const& _useComponentKey();
    std::string const& _useComponentID();
    std::int64_t _getComponentType(std::string const&);
    void _setContext(std::type_index const&, void*);
    void _openComponent(std::int64_t type, std::string const& name, std::string const& key);
    void _closeComponent();
    void _addComponentChild(_Component*, _Component*);
    void _removeComponentChild(_Component*, _Component*);
    void _removeComponentFromParent(_Component*);
    void _unmountComponent(_Component*);
    bool _checkKeyDuplicate(std::string const& key);
    void _beginUpdate();
    void _endUpdate();
    void _abortUpdate();

    friend struct _ComponentScope;
    friend void SetContextAny(std::type_index const&, void*);
    friend void* UseContextAny(std::type_index const&);
    friend std::any& UseStateAny();
    friend std::string const& UseComponentName();
    friend std::string const& UseComponentKey();
    friend std::string const& UseComponentID();
    friend std::int64_t const& UseComponentIndex();
};

/*
 * Free functions below operate on the innermost reconciler currently running
 * an update() and throw std::runtime_error when no update is in progress
 * (except GetComponentType, which needs no reconciler).
 */

/**
 * Type-erased core of SetContext: exposes `pointer` as the context of type
 * `typeIndex` on the current component (see SetContext).
 *
 * @param typeIndex The key the context is looked up by.
 * @param pointer   The context value; only the pointer is stored.
 */
void SetContextAny(std::type_index const& typeIndex, void* pointer);

/**
 * Type-erased core of Context: mounts a provider component exposing the value and renders `children` inside it.
 *
 * @param typeIndex The key the context is looked up by.
 * @param pointer   The context value; only the pointer is stored.
 * @param children  The function that renders the descendants that can see the context.
 */
void ContextAny(std::type_index const& typeIndex, void* pointer, std::function<void()> const& children);

/** Type-erased core of UseState: returns the current component's next state slot (empty on first render). */
std::any& UseStateAny();

/**
 * Type-erased core of UseContext: returns the nearest context exposed under
 * the given key by the current component or one of its ancestors, or
 * nullptr.
 *
 * @param typeIndex The key of the context type to look up.
 */
void* UseContextAny(std::type_index const& typeIndex);

/**
 * Returns a stable process-wide integer id for a component type name (ids start at 1).
 *
 * @param name The component type name.
 */
std::int64_t GetComponentType(std::string const& name);

/** Returns the current component's function name (as captured by COMPONENT). */
std::string const& UseComponentName();

/** Returns the current component's key, or an empty string when keyed by position. */
std::string const& UseComponentKey();

/** Returns the current component's id, unique among same-typed siblings: the key, or — when keyless — name + sequence number. */
std::string const& UseComponentID();

/** Returns the current component's zero-based position among its parent's children this render. */
std::int64_t const& UseComponentIndex();

/**
 * Mounts a provider component that exposes `value` as a context and renders
 * `children` inside it; descendants access it via UseContext<Type>().
 *
 * Only a pointer is stored — the value must outlive the render.
 *
 * @param value    The context value to expose to descendants.
 * @param children The function that renders the descendants that can see the context.
 */
template<typename Type>
void Context(Type& value, std::function<void()> const& children);

/**
 * Returns persistent per-component state.
 *
 * Constructed from `arguments` (as by make_shared) on the first render,
 * preserved across renders, and destroyed when the component unmounts. Call
 * order matters: state slots are matched by call position within the
 * component, so hooks must not run conditionally. Throws std::runtime_error
 * if the slot was already constructed with a different Type.
 *
 * @param arguments Constructor arguments for Type, used only on the first render.
 */
template<typename Type, typename... Arguments>
Type& UseState(Arguments&&... arguments);

/**
 * Returns the nearest enclosing context of `Type`; throws std::runtime_error
 * if none is mounted.
 */
template<typename Type>
Type& UseContext();

/** Returns the nearest enclosing context of `Type`, or nullptr if none is mounted. */
template<typename Type>
Type* UseContextIf();

/**
 * Returns true on the component's first render and whenever any dependency
 * differs (via operator==) from the previous render; false otherwise.
 * With no arguments: true exactly once, on the first render.
 *
 * @param deps Dependency values compared against the previous render.
 */
template<typename... Types>
bool UseEffect(Types const& ...deps);

/**
 * Returns a stable pointer slot owned by the component (initially nullptr).
 *
 * @param forwardedRef When non-null, that external slot is used instead, so
 *                     a parent can observe a ref filled in by a child.
 */
template<typename Type>
Type*& UseRef(Type** forwardedRef = nullptr);

/**
 * Subscribes to `pub` on the component's first render and invokes `fn` on
 * each publish; unsubscribes automatically when the component unmounts.
 * The handler is fixed on mount — later renders do not replace it.
 *
 * @param pub The publisher to subscribe to.
 * @param fn  The handler invoked with each publish's arguments.
 */
template<typename... Arguments>
void UseSubscription(Pub<Arguments...> const& pub, std::type_identity_t<std::function<void(Arguments...)>> const& fn);

/**
 * Exposes `value` as a context on the current component itself, visible to
 * the component and its descendants for the rest of this render (see
 * Context).
 *
 * A component can expose several contexts of different types, up to a small
 * fixed limit (five); exceeding it throws std::runtime_error. A later
 * SetContext of the same type in the same component replaces the earlier
 * value. Contexts are cleared at the start of every render, so a provider
 * calls SetContext on each render. Only a pointer is stored — the value must
 * outlive the render.
 *
 * @param value The context value to expose.
 */
template<typename Type>
inline void SetContext(Type& value) {
    PROFILE

    SetContextAny(std::type_index(typeid(Type*)), const_cast<void*>(static_cast<void const*>(&value)));
}

template<typename Type>
inline void Context(Type& value, std::function<void()> const& children) {
    PROFILE

    ContextAny(std::type_index(typeid(Type*)), const_cast<void*>(static_cast<void const*>(&value)), children);
}

template<typename Type, typename... Arguments>
inline Type& UseState(Arguments&&... arguments) {
    PROFILE

    auto& state = UseStateAny();

    if (state.has_value() == false) {
        state = std::make_shared<Type>(std::forward<Arguments>(arguments)...);
    }

    if (auto typedState = std::any_cast<std::shared_ptr<Type>>(&state)) {
        return *typedState->get();
    } else {
        throw std::runtime_error("Wrong state type");
    }
}

template<typename Type>
inline Type& UseContext() {
    PROFILE

    auto value = UseContextAny(std::type_index(typeid(Type*)));

    if (value != nullptr) {
        return *static_cast<Type*>(value);
    } else {
        throw std::runtime_error(
            std::format("Context `{}` is not mounted", std::type_index(typeid(Type*)).name())
        );
    }
}

template<typename Type>
inline Type* UseContextIf() {
    PROFILE

    return static_cast<Type*>(
        UseContextAny(std::type_index(typeid(Type*)))
    );
}

template<typename... Types>
inline bool UseEffect(Types const& ...deps) {
    PROFILE

    auto& effect = UseState<Effect<Types...>>();
    return effect(deps...);
}

template<typename Type>
inline Type*& UseRef(Type** forwardedRef) {
    PROFILE

    struct _State {
        Type** forwardedRef;
        Type* ownRef;
    };

    auto& state = UseState<_State>();
    state.forwardedRef = forwardedRef;

    if (state.forwardedRef != nullptr) {
        return *state.forwardedRef;
    } else {
        return state.ownRef;
    }
}

template<typename... Arguments>
inline void UseSubscription(Pub<Arguments...> const& pub, std::type_identity_t<std::function<void(Arguments...)>> const& fn) {
    PROFILE

    auto& subscription = UseState<Sub<Arguments...>>();
    auto shouldSubscribe = UseEffect();

    if (shouldSubscribe) {
        subscription.on(pub, fn);
    }
}

} /* namespace Rocket */