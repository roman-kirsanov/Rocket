# Writing UI components

This is the guide for adding a component to the declarative UI layer: how a
component function is shaped, how it keeps state, how props flow into the
retained `Node` tree, and which conventions the existing components follow.

## 1. The model in one paragraph

A component is a plain function that is called on every render. It takes a
props struct by const reference, opens a component scope with `COMPONENT`,
pulls its persistent state through hooks, and then calls other components,
usually ending in one or more `Node(...)` calls. The reconciler matches each
call to the component instance from the previous render (by key, or by name
and position), so `UseState` returns the same object every time and `Node`
keeps syncing the same retained `class Node`. Nothing is diffed at the
component level: a component simply runs, and the retained nodes below it
only receive the fields that actually changed.

## 2. File layout

Every component is a pair `X.hpp` / `X.cpp`. Both start with the MIT license
header used everywhere in the tree.

### Header

```cpp
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Visual style of a button. */
enum class ButtonKind { Solid, Outline, Flat, Transparent };

/** Props for the Button component. */
struct ButtonProps {
    /** Reconciliation key. */
    std::string key;
    /** Optional text label, rendered before `children`. */
    std::optional<std::string> label;
    /** Visual style (default Solid). */
    std::optional<ButtonKind> kind;
    ...
    /** Invoked on click (unless disabled). */
    std::function<void()> onClick;
    /** Props forwarded to the label text Node. */
    NodeProps textProps;
    /** Props forwarded to the wrapping Node; any field set here overrides the computed style. */
    NodeProps nodeProps;
};

/**
 * One-paragraph description of what it renders and how it behaves.
 *
 * @param props    Desired button configuration for this render.
 * @param children Rendered inside the button after the label; default no-op.
 */
void Button(ButtonProps const& props, std::function<void()> const& children = []{});

} /* namespace Rocket */
```

The header contains only what a caller needs: enums, the props struct and the
function. No theme values, no helpers, no state types.

### Source

```cpp
#include <map>
#include <tuple>
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Button.hpp>

namespace Rocket {

/* 1. private enums                    (none in Button) */
/* 2. private structs                  _ButtonStyle, _ButtonMetrics, _ButtonState */
/* 3. static variables                 palette, tables, metrics */
/* 4. static functions                 (none in Button) */
/* 5. public functions                 Button() */

void Button(ButtonProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    /* hooks */
    /* derive values */
    /* build node props */
    /* render */
}

} /* namespace Rocket */
```

Include order is system headers, then `Rocket/...` headers alphabetically by
path, then the component's own header last.

### Symbol order inside a source file

Declarations in a `.cpp` appear in this fixed order, each group complete
before the next starts:

1. **Enums** private to the file.
2. **Structs** private to the file: style and metrics records, then the
   component state struct.
3. **Static variables**: palette colours, lookup tables, metric constants.
4. **Static functions**: helpers used by the component body.
5. **Public functions**: the component function(s) declared in the header.

The order is also a dependency order. Tables (3) may refer to structs (2)
and to earlier variables, and functions (4) may refer to anything above
them, but a table initialiser must never call a static function, because
that function would have to be declared before the variables. If a value
needs computing, give it its own static variable built from earlier ones
(`_blue50050 = Vec4{ _blue500.red, _blue500.green, _blue500.blue, 0.5f }`)
rather than a helper call.

### Naming of file-private symbols

Every symbol that is not declared in the header is prefixed with an
underscore, and types carry the component name so they cannot collide when
several components are compiled into one library:

| Kind | Form | Examples |
|---|---|---|
| Private struct / enum | `_<Component><Role>` | `_ButtonState`, `_ButtonStyle`, `_ButtonMetrics` |
| Static variable | `_camelCase` | `_blue500`, `_style`, `_metrics`, `_disabledOpacity` |
| Static function | `_PascalCase` | `_GetMapped`, `_OptionalChain` |
| Local variable | `camelCase` (no prefix) | `state`, `nodeRef`, `isHover`, `node` |

Public names never carry an underscore. The state struct is
`_<Component>State` and is declared at file level in the structs group, not
inside the function. (Some older core components still declare a local
`_State` inside the function; new code follows the file-level form.)

## 3. The props struct

Field order is fixed so every component reads the same way:

1. `std::string key;` always. `COMPONENT` reads `props.key`, so the struct
   must have this exact member.
2. Component-specific props, each `std::optional<T>` (or `std::function`
   for callbacks). Use enums for closed sets (`ButtonKind`, `ButtonIntent`,
   `ButtonSize`, `PopupVerticalOrigin`), never a cluster of booleans. The TSX
   original of Button had `primary`, `success`, `outline`, `flat` ...
   flags; the port collapses each group into one enum.
3. One `NodeProps` per underlying node the caller may want to reach, named
   `<role>Props`: `textProps`, `iconProps`, and always `nodeProps` last for
   the wrapping node.

There is no `ref` field. Only the retained-object components (`Node`,
`Document`, `Window`) expose a `ref`; a composite component's node is
reached through `nodeProps.ref`, which the component forwards (see 4.3).
A second `ref` on the props struct would only duplicate that path.

Why `std::optional` everywhere: unset means "use the component's default",
and the component can tell the difference between "caller passed false" and
"caller said nothing". Plain `bool` fields only make sense in props structs
that are never merged, which is none of them.

Name enums `<Component><Aspect>` and document each enumerator that is not
self-evident. Give the default enumerator first.

## 4. The function body, step by step

### 4.1 Open the scope

```cpp
PROFILE
COMPONENT
```

`PROFILE` is the tracing macro used on every function in the tree.
`COMPONENT` opens the reconciliation scope keyed by `props.key` and closes it
when the function returns. Use `COMPONENT_NO_KEY` for app-level components
with no props struct (see `example/BeatMaker/App/Project/Project.cpp`).

Matching rules you have to know:

- A keyed component is matched by type and key among its parent's
  children. Rendering two siblings with the same key under one parent
  throws, whatever their types.
- A keyless component is matched by function name plus its ordinal among
  same-named siblings. Reordering keyless siblings therefore swaps their
  state.
- **Set `key` only where it is needed: inside loops.** A fixed sequence of
  siblings written out in the source is matched by name and position and
  needs no keys, so `Node({ .key = "nav" })` next to `Node({ .key =
  "story" })` is noise. Items rendered from an array must carry a key,
  because their number and order can change between renders and only the
  key keeps each item's state and retained node with it. One caveat: a
  sibling rendered conditionally in front of same-typed siblings is also
  matched by ordinal, so when it disappears its component is reused for
  the next sibling; that is harmless for the stateless case, but give the
  conditional one a key if the siblings hold user state you must not mix
  up (an input's text, a scroll position).

### 4.2 Declare state

State is a file-level struct named `_<Component>State` (`_ButtonState`),
declared in the structs group above the statics, one per component, fetched
once with `UseState<_ButtonState>()`. Keep in it:

- Copies of every callback from props that an event handler will call later
  (`onClick`, `onClose`, `nodeOnEvent`). Handlers registered on a node
  outlive the render, and `props` is a reference to a temporary, so
  handlers must read from `state`, never from `props`.
- Cached derived values, plus the `Effect<...>` that gates recomputing them
  (section 6).
- Anything else that must survive across renders: drag-in-progress flags,
  the node being dragged, subscriptions (`Sub<...>`).

Members are plain values; the state struct has no constructor unless a
member needs constructor arguments (see `Document`'s state).

`UseState`, `UseRef`, `UseEffect` and `UseSubscription` are matched by
call position, so never call them inside a branch or loop. `UseContext`
and `UseContextIf` are plain lookups with no slot of their own and may be
called anywhere during the render.

### 4.3 Get the ref

```cpp
auto& nodeRef = UseRef<class Node>(props.nodeProps.ref);
```

`UseRef` returns a stable `Node*&` slot. When the caller set
`nodeProps.ref` the component writes through to that slot, otherwise it
uses its own. The returned reference is then handed to the node as
`node.ref = &nodeRef`, so the same slot is filled by `Node` on mount and the
caller sees the node exactly as if they had rendered it themselves. Name
the local after the node it points to (`nodeRef`, `textRef`). Read it at
the top of the render for interactive state:

```cpp
auto const isHover = (nodeRef != nullptr && nodeRef->isHover());
auto const isActive = (props.active.value_or(false) || (nodeRef != nullptr && nodeRef->isActive()));
```

`nodeRef` is `nullptr` on the very first render (the node does not exist
yet); always guard it.

### 4.4 Store callbacks

```cpp
state.onClick = props.onClick;
state.nodeOnEvent = props.nodeProps.onEvent;
```

Do this on every render, before anything else uses them, so the handler
installed on the node always sees the latest closures.

### 4.5 Derive values

Resolve defaults once with `value_or`, then compute whatever the node needs.
Prefer a nested conditional expression over an `if` ladder for small
mappings, laid out one case per line:

```cpp
auto const justify = (
    alignment == ButtonAlignment::Start ? NodeJustify::Start :
    alignment == ButtonAlignment::End ? NodeJustify::End :
                                        NodeJustify::Center
);
```

Anything that depends only on a few props and involves table lookups goes
under an Effect gate (section 6). Anything that depends on hover, pressed,
focus or a boolean that flips often is computed inline every render.

### 4.6 Build the node props

This is the core pattern and it is the same in every component:

```cpp
auto node = props.nodeProps;
node.ref = &nodeRef;
node.direction = node.direction.value_or(NodeDirection::Horizontal);
node.alignment = node.alignment.value_or(NodeAlignment::Center);
node.paddingLeft = node.paddingLeft.value_or(node.padding.value_or(state.padding.x));
node.minHeight = node.minHeight.value_or(state.minHeight);
node.textColor = node.textColor.value_or(text);
```

Copy the forwarded `NodeProps`, then fill each field with
`field = field.value_or(computed)`. That single line encodes the precedence
rule of the whole system: **a value set by the caller in `nodeProps` wins
over the component's computed value**, exactly as a `className` or spread
prop would in React. The component never overwrites a caller's field, with
two exceptions:

- `node.ref` is always set to the slot returned by `UseRef` (which is the
  caller's `nodeProps.ref` when they set one, see 4.3).
- `node.onEvent` is replaced by the component's handler, which calls the
  caller's stored handler first (a component that needs the capture phase
  treats `onCaptureEvent` the same way; otherwise it passes through):

```cpp
node.onEvent = [&, disabled](NodeEvent const& event) {
    if (state.nodeOnEvent) {
        state.nodeOnEvent(event);
    }

    if (event.is<MouseClickNodeEvent>()) {
        if (disabled == false) {
            if (state.onClick) {
                state.onClick();
            }
            event.stopPropagation();
        }
    }
};
```

The lambda captures `state` by reference (it lives as long as the
component) and small render-time values by copy. Capture `props` never.

Shorthand versus per-edge fields: `NodeProps` has `padding` and
`paddingLeft` etc. The component sets the per-edge fields, and lets a
caller's shorthand win over the computed value:
`node.paddingLeft.value_or(node.padding.value_or(x))`. Same for `gap` /
`gapX` and `margin` / `marginLeft`.

Optional fields that should stay unset when not applicable are only set
inside a condition:

```cpp
if (background.alpha > 0.0f) {
    node.background = node.background.value_or(ColorBrush{ background });
}

if (disabled) {
    node.mouseEvents = node.mouseEvents.value_or(false);
    node.opacity = node.opacity.value_or(_disabledOpacity);
} else {
    node.cursor = node.cursor.value_or(Cursor::Pointer);
}
```

### 4.7 Render

```cpp
Node(node, [&]{
    if (props.label.has_value()) {
        auto text = props.textProps;
        text.display = NodeDisplay::Text;
        text.content = text.content.value_or(props.label.value());

        Node(text);
    }

    children();
});
```

Child nodes are created inside the children lambda of the wrapping `Node`.
Sub-nodes get the same treatment: copy the forwarded `<role>Props`, fill
with `value_or`, render. A `children` parameter is declared as
`std::function<void()> const& children = []{}` and called exactly once, after
the component's own children, so callers can nest arbitrary content.

Rendering a text node: `display = NodeDisplay::Text` plus `content`. Font
size and text colour are inherited, so set them on the wrapper when they
apply to everything inside, and on the text node only when they are
specific to it.

## 5. Theme data

All look-and-feel data is a set of file-level statics at the top of the
`.cpp`, above the component function. Nothing is imported from a theme
module; the component is self-contained.

Conventions:

- `static auto const _name = ...;` for values; names carry a leading
  underscore and are camelCase (`_blue550`, `_borderRadius`,
  `_disabledOpacity`).
- Colours are compile-time literals, one per line, named after the palette
  entry they come from and carrying the original hex in a trailing comment:
  `auto constexpr _blue500 = Vec4{ 59.0f / 255.0f, 130.0f / 255.0f,
  246.0f / 255.0f, 1.0f }; /* #3b82f6 */`. A short comment above the block
  says which stylesheet they follow (Button follows fuego's `colors.scss`).
  Never `ColorFromHex` in a static: it runs at start-up and is not
  `constexpr`. Derived
  variants are their own statics built from the base one, never a second
  hard-coded colour and never a helper call inside a table initialiser
  (see symbol order in section 2).
- Lookups keyed by enums are `std::map<std::tuple<Kind, Intent>, Value>`
  (or `std::map<Size, Value>`), one row per entry, aligned into columns so a
  table reads like the original stylesheet.
- A per-state style is a small struct of `std::optional<Vec4>`
  (`_ButtonStyle` with `background`, `border`, `text`). The normal state
  sets every field; hover and active set only what they change and are
  layered on in that order. This mirrors how `hover:` and `active:` classes cascade, and it
  keeps the table honest: a blank cell means "unchanged", not "transparent".
- Metrics (padding, min height, gap, radius, font size) are in points and
  are separate statics or a `_ButtonMetrics` struct table. Use literal
  suffixes: `6.0f`, not `6`.
- Every struct, variable and function in this section is file-private,
  `static` where applicable, and `_Prefixed` (section 2).

## 6. Effect gating

`Effect<Types...>` is a change detector: its `operator()` returns true on the
first call and whenever any argument differs from the previous call.
`UseEffect(deps...)` is the same thing stored in a state slot.

Use it to split a component's work into a cold path and a hot path:

```cpp
struct _ButtonState {
    std::tuple<_ButtonStyle, _ButtonStyle, _ButtonStyle> const* style;
    float borderWidth;
    Vec2 padding;
    ...
    Effect<
        std::optional<ButtonKind>,
        std::optional<ButtonIntent>,
        std::optional<ButtonSize>,
        std::optional<bool>
    > shouldUpdate;
};

if (state.shouldUpdate(props.kind, props.intent, props.size, props.narrow)) {
    /* table lookups, scale-dependent maths, image selection */
    state.style = &_style.at({ kind, intent });
    ...
}

/* hover / pressed / disabled cascade: runs every render */
```

Guidelines:

- Gate: map lookups, anything multiplied by `document.getScale()`, image or
  slice selection, string formatting. Key the Effect on the raw
  `std::optional` props, not on the resolved values, so the dependency list
  is a straight copy of the props.
- Do not gate: hover, pressed, focus, `disabled`, `active`, and the merge
  into `NodeProps`. These are a handful of branches and they
  change on every mouse move anyway.
- Cache pointers into static tables (`_ButtonStyle const*`) rather than
  copying tuples; the tables are `static const`, so the pointers are stable.
- Include `document.getScale()` in the dependency list of anything that
  depends on it, and `&theme` if a theme object is ever introduced.

The copy of `props.nodeProps` into the local `node` is the dominant
per-render cost of a component and cannot be gated: `Node()` needs the merged
struct every render. Do not try to cache it.

## 7. Events and re-rendering

The two objects a component talks to outside its own node tree are
contexts, obtained with the `UseContext` hook at the top of the render,
next to the other hooks:

```cpp
auto& document = UseContext<class Document>();
auto& reconciler = UseContext<Reconciler>();
```

`Document` provides the `class Document` it owns (via `SetContext`), and
`Reconciler::update()` provides the reconciler itself as the root context.
Never reach either of them through a global, a static, or a pointer stashed
in props; the context is what ties a component to the document and
reconciler that are actually rendering it, which matters as soon as there
is more than one window. `UseContext` throws when no provider is mounted,
so a component that can render outside a `Document` uses `UseContextIf`
and checks for `nullptr`. Neither call occupies a state slot, so unlike
`UseState` they are not positional.

- Hover and pressed state live on the retained node (`isHover()`,
  `isActive()`), not in component state. The example apps re-run the
  reconciler on every `UpdateAppEvent`, so reading them at render time is
  fresh; an app that renders less often has to request a re-render on
  enter, exit, press and release itself. Do not track hover in state.
- Node events bubble through `onEvent` and are seen in the capture phase
  through `onCaptureEvent`. The wrapper's handler is installed by the
  component; the caller's handler runs first via `state.nodeOnEvent`.
- Call `event.stopPropagation()` after handling a click so an ancestor
  button does not also fire.
- For document-wide events (press outside, key shortcuts) subscribe once
  with `UseSubscription(document.onCaptureEvent, [&](NodeEvent const& e){ ... })`
  on the `document` context. The handler is fixed on mount, so it must read
  everything it needs from `state` (see `Popup`, which stores `onClose` in
  its state for exactly this reason).
- A component that changes its own state from an event handler and needs a
  re-render outside the normal update cycle calls `reconciler.needsUpdate()`
  on the `reconciler` context, gated so it does not request on every
  render. Calling it during a render schedules one follow-up pass after the
  current one; requests coalesce.
- The document also answers layout questions at render time
  (`document.getScale()` for anything that must be recomputed when the
  backing scale changes; include it in the Effect dependency list).

## 8. Context

`UseContext<T>()` finds the nearest provider of `T` and throws if there is
none; `UseContextIf<T>()` returns `nullptr` instead. Providers are either
`Context(value, children)` (a wrapper component) or `SetContext(value)`
inside a component, which exposes the value to the component itself and its
descendants for the rest of that render.

Things always available below a `Document`:

- `UseContext<Reconciler>()`
- `UseContext<class Window>()`
- `UseContext<class Document>()`

Only a pointer is stored, so the value must outlive the render. Component
state is a fine place to keep it (`Document` keeps its `class Document` in
its state struct and calls `SetContext` on it).

A component may expose several contexts of different types, up to five;
a sixth throws. `Document` uses two: the retained document and the child
context for top-level nodes. A second `SetContext` of the same type in the
same component replaces the first. Contexts are cleared at the start of
every render, so a provider calls `SetContext` on each render, and they
stop being visible the moment the providing component returns.

Lookup walks from the current component up through its ancestors and
scans each one's inline context entries, so the nearest provider wins.
The entries live in the component record itself, not behind a heap
pointer, so the walk touches one record per level and nothing else.

## 9. Reconciler gotchas

- **Sibling order.** Retained order always equals render order, for
  remounts and keyed reorders alike. Each Node component takes its index
  from the parent's `_NodeContext` (`src/Rocket/UI/Node_Private.hpp`) and,
  whenever that index changes, places itself with
  `Node::insertChild(node, index)`; a node already at its index is a no-op. The context comes from the nearest `Node` or from the
  `Document` component. Root nodes, and nodes rendered under a manual
  `Context(document, ...)` (a test-only shortcut), have no sibling
  bookkeeping and are appended to the document in mount order. Covered by
  `tests/Rocket/UI/Node_Test.cpp`.
- **Keys and types.** Keyed components are matched by type and key; two
  siblings with the same key under one parent throw, whatever their types.
- **Hooks are positional.** No hook inside a branch or loop, and no early
  return before the last hook.
- **Handlers outlive renders.** Never capture `props`, or any other local
  of the render (a helper lambda, a loop variable by reference), in a
  lambda stored on a node, passed to a child as a callback, or given to a
  subscription; copy what you need into the state struct each render and
  capture `state` and `reconciler` explicitly. The failure is a crash on a
  later click, not at render time, and it can stay hidden while the dead
  stack memory happens to still hold the old values.
- **First render has no node.** `nodeRef` is null until `Node` has mounted;
  guard every use.
- **Optional shorthand.** When a caller sets `padding`, the per-edge fields
  must fall back to it before falling back to the component default.

## 10. Code style checklist

- `auto` first: `auto const x = ...;`, `auto& state = UseState<_ButtonState>();`.
  Spell the type only when it cannot be deduced.
- Literal suffixes always: `0.0f`, `9999.0f`, `1.5f`.
- `disabled == false`, not `!disabled`.
- Braces on every `if`, even one-liners, with a blank line before an `if`
  that follows assignments.
- Nested ternaries are parenthesised and laid out one case per line, the
  final fallback indented to line up with the values.
- File-private symbols are `_Prefixed`, and private types carry the
  component name: `_ButtonState`, `_ButtonStyle`, `_ButtonMetrics`,
  `_style`, `_metrics`, `_GetMapped`. Nothing in a public header has an
  underscore; an internal type that two translation units must share goes
  in a `<Module>_Private.hpp` next to the module (see
  `src/Rocket/UI/Node_Private.hpp`).
- Symbol order in a `.cpp`: enums, structs, static variables, static
  functions, public functions.
- One component per file; the function name equals the file name.
- `key` only inside loops; never on a fixed sequence of siblings.
- Doc comments on every public enum, struct, field and function, `/** ... */`
  style, with `@param` for function parameters. Field comments say what the
  default is when the field is unset.
- Comments in the body explain a non-obvious decision (why outline padding
  differs from solid by one point), not what the code does.

## 11. Adding a new component, end to end

1. Create `src/Rocket/UI/X.hpp` and `X.cpp` with the license header.
2. Define `XKind` / `XIntent` / `XSize` enums as needed, then `XProps` in
   the order: `key`, component props, callbacks, `<role>Props`,
   `nodeProps`. No `ref` field.
3. Declare `void X(XProps const& props, std::function<void()> const& children = []{});`
   (drop `children` if the component cannot contain arbitrary content).
4. In the `.cpp`, lay the file out in order: private enums, private structs
   (`_XStyle`, `_XMetrics`, `_XState`), static variables (palette, tables,
   metrics), static functions, then the component function.
5. Write the body in the fixed order: `PROFILE`, `COMPONENT`, hooks
   (`UseState<_XState>()`, `UseRef`), store callbacks, Effect-gated
   resolution, per-render cascade, `node` merge with `value_or`, handler,
   `Node(node, children)`.
6. Add `src/Rocket/UI/X.cpp` to `ROCKET_SOURCE` in the root
   `CMakeLists.txt` and `#include <Rocket/UI/X.hpp>` to
   `src/Rocket/Rocket.hpp`.
7. Add `tests/Rocket/UI/X_Test.cpp`, modelled on `Button_Test.cpp`: render
   the component under a manually provided Document, assert the computed
   node fields for each prop, and drive hover, press and click through
   `window.onEvent`.
8. Add a story: `src/Rocket/UI/Storybook/X.hpp` declaring `_XStory()` and
   `Storybook/X.cpp` defining it, with one state field per prop driven by
   the `_StoryRadio` / `_StoryToggle` / `_StoryInput` controls from
   `Storybook/_Helpers.hpp`, plus an entry in the `_stories` list in
   `Storybook.cpp`. Add the `.cpp` to `ROCKET_SOURCE`.
9. Build with `./make.sh --test`, then `./make.sh --example` and run
   `.build/Debug/example/Storybook/Storybook` to look at it.
