# Writing UI components

This is the guide for adding a component to the declarative UI layer: how a
component function is shaped, how it keeps state, how props flow into the
retained `Node` tree, and which conventions the existing components follow.

The examples use a `Button` component to show every pattern in one place.
`Button` is not part of the library; `Popup` and `Splitter` in
`src/Rocket/UI/` are the components to read alongside this guide.

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
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <Rocket/UI/Material.hpp>
#include <Rocket/UI/Node.hpp>

namespace Rocket {

/** Visual style of a button. */
enum class ButtonKind { Solid, Outline, Flat, Transparent };

/** Everything Button takes from the theme: a list of rules, as in a CSS stylesheet. */
struct ButtonTheme {
    /** State of the button a rule can match. */
    enum class State { Normal, Hover, Active, Disabled };

    /** What a rule matches; an unset field matches any value. */
    struct Selector {
        /** Kind of the button. */
        std::optional<ButtonKind> kind;
        /** State of the button. */
        std::optional<State> state;
        ...
    };

    /** What a rule sets; an unset field is left to other rules. */
    struct Style {
        /** Kind of the material drawn; unset draws the plain colors. */
        std::optional<MaterialKind> materialKind;
        /** Fill color, without a material. */
        std::optional<Vec4> background;
        /** Color of the label and icon. */
        std::optional<Vec4> color;
        ...
    };

    /** A selector and the style it applies. */
    struct Rule {
        /** What the rule matches. */
        Selector selector;
        /** What the rule sets. */
        Style style;
        /** Beats every rule that is not important, whatever its selector. */
        bool important = false;
    };

    /** The rules, in order. */
    std::vector<Rule> rules;
};

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

The header contains only what a caller needs: enums, the theme struct
`<Component>Theme` with the public structs it is made of, the props struct
and the component function. The theme struct is public because every theme
fills it in (section 5). No look values, no helpers, no state types.

### Source

```cpp
#include <Rocket/Node/NodeEvent.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Button.hpp>

namespace Rocket {

/* 1. private enums                    (none in Button) */
/* 2. private structs                  _ButtonLook, _ButtonState */
/* 3. static variables                 (none in Button) */
/* 4. static functions                 (none in Button) */
/* 5. public functions                 Button() */

void Button(ButtonProps const& props, std::function<void()> const& children) {
    PROFILE
    COMPONENT

    ...
}

} /* namespace Rocket */
```

Includes follow the order in [Code style §10](Codestyle.md#10-layout). The
body follows the steps of section 4.

### Symbol order inside a source file

Declarations follow the symbol order in
[Code style §4](Codestyle.md#4-symbol-order-in-a-source-file). In a
component, the private structs are the component state struct and any
records the render works with, and the public functions are the component
function(s) declared in the header. A component keeps no colors or metrics
of its own: those are in the themes (section 5).

### Naming of file-private symbols

File-private symbols are named as in
[Code style §8](Codestyle.md#8-private-names). In a
component, private structs and enums also carry the component name,
`_<Component><Role>` (`_ButtonState`, `_PopupAxis`, `_PopupPlacement`), so
they cannot collide when several components are compiled into one library.
The state struct is
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

The comment of each field says what the default is when the field is unset.

## 4. The function body, step by step

### 4.1 Open the scope

```cpp
PROFILE
COMPONENT
```

`PROFILE` is the tracing macro used on every function in the tree.
`COMPONENT` opens the reconciliation scope keyed by `props.key` and closes it
when the function returns. Use `COMPONENT_NO_KEY` for app-level components
with no props struct (see `tests/Rocket/UI/Reconciler_Test.cpp`).

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
  key keeps each item's state and retained node with it.
- **Components react to their props.** A component that keeps state
  derived from its props brings that state up to date when the props
  change, so that it can be rendered in place of another one with
  different props and still be right. A key is not used to make it start
  over.

  ```cpp
  /* no: a key to throw the old highlight away */
  StoryMenu({ .key = "story", .onPlay = onPlay });

  /* yes: UseChoices brings its highlight back in range itself when the count changes */
  if (choices.selected >= count) {
      choices.selected = 0;
  }
  ```

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
call position, so never call them inside a branch or loop, and never return
early before the last of them. `UseContext`,
`UseContextIf` and `UseTheme` are plain lookups with no slot of their own
and may be called anywhere during the render.

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
A default that is part of the look (a color, a size, a radius) comes from
the component's theme (section 5), never from a literal in the source.
Small mappings are nested conditional expressions, laid out as in
[Code style §10](Codestyle.md#10-layout).

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
    if (state.nodeOnEvent != nullptr) {
        state.nodeOnEvent(event);
    }

    if (event.is<MouseClickNodeEvent>()) {
        if (disabled == false) {
            if (state.onClick != nullptr) {
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

if (style.opacity.has_value()) {
    node.opacity = node.opacity.value_or(style.opacity.value());
}

if (disabled) {
    node.mouseEvents = node.mouseEvents.value_or(false);
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
size and text color are inherited, so set them on the wrapper when they
apply to everything inside, and on the text node only when they are
specific to it.

## 5. Theme

Everything a component looks like comes from the theme: colors, sizes,
radii, font sizes, shadows, and how each changes with kind, intent, size
and state.
A component keeps no look of its own, not even a default.

### 5.1 Shape

- `<Component>Theme`, in the component's header, holds everything the
  component takes from the theme: a list of rules, as in a CSS stylesheet.
  Its types are nested in it, so a component adds no theme types of its
  own to the `Rocket` namespace, and components share none:

  - `State`: the states a rule can match (`Normal`, `Hover`, `Active`,
    `Disabled` for Button);
  - `Selector`: what a rule matches, one optional field per prop or state
    that changes the look; an unset field matches any value;
  - `Style`: what a rule sets, one optional field per value the component
    reads; an unset field is left to other rules;
  - `Rule`: a `selector`, a `style` and `important`;
  - `std::vector<Rule> rules`.

- Every rule whose selector matches applies its style, field by field.
  Where two rules set the same field, an important rule beats one that is
  not, then the rule whose selector sets more fields wins, then the later
  one. To change one value of a theme, append a rule; to make sure it
  wins, make it important.
- A component that can be drawn with a material has the material's
  fields in its `Style` (`materialKind`, `materialIntent`,
  `materialBrightness`, `materialRoundness`), each cascading on its own:
  a hover rule can change only the brightness. A style
  resolved with a `materialKind` is drawn with the theme's material,
  otherwise with the plain colors (`background`, `borderColor`,
  `borderWidth`, `borderRadius`). A left-out edge (`noLeftBorder`...)
  squares the corners on its side either way.
- The component resolves its rules once per state in its Effect gate
  (section 6), with `_GetCascade` and `_Cascade` from
  `src/Rocket/UI/Theme_Private.hpp`, and picks the resolved style of the
  current state on every render.
- `Theme` (`src/Rocket/UI/Theme.hpp`) holds the material the material
  styles are drawn with and one member per component, named after it
  in camelCase:

  ```cpp
  struct Theme {
      Material const* material;
      ButtonTheme button;
      IconTheme icon;
  };

  Theme const& UseTheme();
  ```

- The themes compiled into the library are in `src/Rocket/UI/Theme/<Name>/`.
  A folder holds one theme, or several variants of one look:

  ```cpp
  Theme const& Get<Name>Theme();           // Theme.hpp|cpp: one theme
  Theme const& Get<Name><Variant>Theme();  // Theme.hpp|cpp: several (GetDefaultDarkTheme, GetDefaultLightTheme)
  Material const& Get<Name>Material();     // Material.hpp|cpp, in a folder whose themes have a material
  ```

  Each builds its value in a function-local static on first use, so it
  never depends on the order in which statics of other files are
  initialized. A theme fills in every member of every component theme;
  adding a component means adding its values to every theme. Themes in
  different folders share no data, with one exception: any theme may build
  on the default palette (`GetDefaultPalette()` in `Theme/Default`), since
  the Default themes are always part of the library. The themes of one
  folder may also share the functions that build them.
- `Theme/Default` holds `GetDefaultDarkTheme()` and `GetDefaultLightTheme()`:
  every component drawn with plain colors, and no material
  (`material` is null). Both are built on the folder's palette,
  `ColorPalette const& GetDefaultPalette()` in `Palette.hpp|cpp`. A
  `ColorPalette` (`src/Rocket/UI/ColorPalette.hpp`) holds the colors of
  Tailwind CSS 4 in shades 50 to 950 every 50 (`red50`, `red100`, `red150`
  ... `red950`), where the shades Tailwind does not define are the
  midpoints of their neighbors in OKLCH. `Palette.py` generates both files
  from Tailwind's `theme.css`.
- The `Pixel` themes draw with a material: `Theme/PixelDark` is the first of
  them (`PixelLight` and others follow the same pattern). PixelDark takes
  its colors from the default palette, apart from a few of its own. A theme's images
  are in its `Image/` folder: the PixelDark material is drawn in
  `Image/Image.aseprite` and exported by `Image/Export.py` into
  `Image/Image*.cpp` (see [Material](#55-material)).
- `UseTheme` returns the theme provided by the nearest `Theme` context
  (section 8), or `GetDefaultDarkTheme()` when there is none, so a
  component always has a theme, inside a `Document` or not.

### 5.2 Providing a theme

A theme is provided like any context, const or not:
`Context(GetDefaultLightTheme(), children)`, or `SetContext(theme)` in the
component that owns it. The provider keeps a `Theme` it built alive,
usually in its state.

**A provided theme is never changed.** To switch themes (light and dark),
keep both and provide the other one; to change a value, build a new theme.
Components cache what they derive from their theme and recompute it when
the theme's address changes (section 6); a theme changed in place would
leave those caches stale.

```cpp
struct _AppState {
    Theme square = _CreateSquareTheme();
    bool isSquare = false;
};

static Theme _CreateSquareTheme() {
    auto theme = GetPixelDarkTheme();
    theme.button.rules.push_back({ .style = { .materialRoundness = MaterialRoundness::None }, .important = true });
    return theme;
}

Context((state.isSquare ? state.square : GetPixelDarkTheme()), [&]{
    ...
});
```

### 5.3 Reading it

```cpp
auto const& theme = UseTheme();
```

once, at the top of the render, next to the hooks; the component reads its
own member (`theme.button`) and, to draw a material, `theme.material`. The order of precedence is: a field set in `nodeProps`,
then a prop of the component that sets the same thing (`Splitter`'s
`color`), then the theme.

### 5.4 Writing a theme

- Colors are compile-time literals, one per line, named after the palette
  entry they come from:
  `auto constexpr _blue500 = Vec4{ 59.0f / 255.0f, 130.0f / 255.0f,
  246.0f / 255.0f, 1.0f };`. A `constexpr` variable at namespace scope is
  already file-private, so it needs no `static`.
  Never `ColorFromHex` in a static: it runs at start-up and is not
  `constexpr`. Derived
  variants are their own statics built from the base one, never a second
  hard-coded color and never a helper call inside a table initializer
  (see [Code style §4](Codestyle.md#4-symbol-order-in-a-source-file)).
- A theme lists the rules of each component from the general to the
  specific: the base rule with no selector, then states and sizes, then
  kinds and intents, then their combinations. A theme that repeats a
  pattern for each intent builds those rules in a function, as
  `Theme/Default` does.
- Metrics (padding, min height, gap, radius, font size) are in points.

### 5.5 Material

A `Material` (`src/Rocket/UI/Material.hpp`) is a sprite sheet drawn at 1x,
2x and 3x, made of the three images; every sheet follows the layout of the
PixelDark one, which draws each variant in three brightnesses
(`MaterialBrightness::Normal`, `Lighter`, `Darker`). `getVariant` cuts the
brush of a panel in one brightness from the sheet drawn closest to the
scale, `getArrowVariant` that of an arrow. `getCircleVariant` draws a
circle of any width in the colors of the panel of the same kind, and
`getCapsuleVariant` a capsule of any size, each built once per size in
pixels of the sheet. Which brightness a component
draws is up to its theme, not tied to its state: a panel may rest on the
darker one. A material is not
part of the theme system: a theme only points at one, and a material can be
used without a theme. `GetPixelDarkMaterial()` is the material of the
PixelDark theme.

## 6. Effect gating

`Effect<Types...>` is a change detector: its `operator()` returns true on the
first call and whenever any argument differs from the previous call.
`UseEffect(deps...)` is the same thing stored in a state slot.

Use it to split a component's work into a cold path and a hot path:

```cpp
struct _ButtonState {
    std::array<_ButtonLook, 4> looks;
    ...
    Effect<
        Theme const*,
        std::optional<ButtonKind>,
        std::optional<ButtonIntent>,
        std::optional<ButtonSize>,
        std::optional<bool>
    > shouldUpdate;
};

if (state.shouldUpdate(&theme, props.kind, props.intent, props.size, props.narrow)) {
    for (auto index = 0llu; index < _states.size(); index++) {
        state.looks[index] = ...;  // the rules resolved for that state
    }
}
}

...
```

The gated block does the table lookups, scale-dependent math and image
selection; the hover, pressed and disabled cascade after it runs every
render.

Guidelines:

- Gate: map lookups, anything multiplied by `document.getScale()`, image or
  slice selection, string formatting. Key the Effect on the address of the
  theme and on the raw `std::optional` props, not on the resolved values,
  so the dependency list is the theme plus a straight copy of the props.
- Do not gate: hover, pressed, focus, `disabled`, `active`, and the merge
  into `NodeProps`. These are a handful of branches and they
  change on every mouse move anyway.
- Resolve the theme's rules in the gate and keep the result in state. It
  stays right as long as the theme is provided, because a provided theme
  never changes (section 5.2), and the Effect resolves the rules again
  when another theme is provided.
- Include `document.getScale()` in the dependency list of anything that
  depends on it.

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
is more than one window. A component that can render outside a `Document`
uses `UseContextIf` and checks for `nullptr` (section 8).

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
- Files dragged from another application and dropped on a window arrive as
  `DropFilesNodeEvent` (the paths and the point in document coordinates) on
  the topmost node under the drop point, or on the document when there is
  none, and bubble like mouse events. A drop zone handles it in its own
  node's `onEvent`; the window publishes it first as `DropFilesWindowEvent`.
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

`UseTheme()` is available everywhere: with no `Theme` provided, it returns
the default theme (section 5.1).

A context is found by its type whether it was provided const or not, so a
const value is read with `UseContext<Type const>()`.

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
- **Handlers outlive renders.** Never capture `props`, or any other local
  of the render (a helper lambda, a loop variable by reference), in a
  lambda stored on a node, passed to a child as a callback, or given to a
  subscription; copy what you need into the state struct each render, or
  into the lambda by value, and capture `state` and `reconciler` by
  reference through `[&]`. The failure is a crash on a
  later click, not at render time, and it can stay hidden while the dead
  stack memory happens to still hold the old values.

## 10. Code style checklist

- The general rules in [Code style](Codestyle.md): no aliases, no `!`, no
  comments in sources and private headers, symbol order, headers with
  implementation, exceptions, lambda captures, private names, types and
  literals, layout, global C names.
- One component per file; the function name equals the file name.
- No look in the component's source: every color, size and table the
  render uses is read from `<Component>Theme`, and the values are written
  in the themes.

## 11. Adding a new component, end to end

1. Create `src/Rocket/UI/X.hpp` and `X.cpp` with the license header.
2. Define `XKind` / `XIntent` / `XSize` enums as needed, then `XTheme`
   with its nested `State`, `Selector`, `Style` and `Rule` (section 5),
   then `XProps` in the field order of section 3. No `ref` field.
3. Declare `void X(XProps const& props, std::function<void()> const& children = []{});`
   (drop `children` if the component cannot contain arbitrary content).
4. In the `.cpp`, lay the file out in order: private enums, private structs
   (`_XState`), static variables, static functions, then the component
   function.
5. Write the body in the fixed order: `PROFILE`, `COMPONENT`, hooks
   (`UseState<_XState>()`, `UseRef`) and `UseTheme()`, store callbacks,
   Effect-gated resolution, per-render cascade, `node` merge with
   `value_or`, handler, `Node(node, children)`.
6. Add `src/Rocket/UI/X.cpp` to `ROCKET_SOURCE` in the root
   `CMakeLists.txt` and `#include <Rocket/UI/X.hpp>` to
   `src/Rocket/Rocket.hpp`. Add the member `XTheme x;` to `Theme` and fill
   it in every theme in `src/Rocket/UI/Theme`.
7. Add `tests/Rocket/UI/X_Test.cpp`, modeled on `Popup_Test.cpp`: render
   the component under a manually provided Document, assert the computed
   node fields for each prop, assert that a value changed in a provided
   theme reaches the node, and drive hover, press and click through
   `window.onEvent`.
8. Build with `./make.sh --test` (`.\make.ps1 --test` on Windows).
