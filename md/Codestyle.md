# Code style

Library-wide rules for Rocket code. Rules that only concern UI components
are in [Component](Component.md).

## 1. No aliases

A name is introduced only when it does more than rename. This holds for
variables and for functions.

### Variables

Do not introduce a local variable that only gives another name to something
that is already reachable. Call the accessor where the value is used.

```cpp
/* no */
auto const& keycode = keyDownEvent->getKeycode();
auto const& modifiers = keyDownEvent->getModifiers();

if (modifiers.meta == true && keycode == Keycode::M) {
    ...
}

/* yes */
if (
    keyDownEvent->getModifiers().meta == true &&
    keyDownEvent->getKeycode() == Keycode::M
) {
    ...
}
```

A local is justified only when it does more than rename:

- **A copy is needed**, because the value is going to be modified
  independently of its source.

  ```cpp
  auto node = props.nodeProps;
  node.ref = &ref;
  ```

- **It names an expression**, something computed rather than merely
  fetched.

  ```cpp
  auto const isHover = (ref != nullptr && ref->isHover());
  ```

- **The original value has to be saved**, because the source is about to
  change and the old value is still needed.

  ```cpp
  auto const previousScale = document->getScale();
  document->setScale(previousScale + 0.1f);
  ```

- **The call does work**, more than returning a field, and the result is
  used more than once: it is called once and kept.

  ```cpp
  auto const& glyphs = getGlyphs(); // lays the text out first
  auto& context = _GetContext();
  ```

### Functions

This applies to helpers: static functions, private member functions and
lambdas. A function declared in a header is part of its module's
interface, such as an accessor, and is not a helper.

A helper whose body only calls another function, with the same arguments
or fixed ones, or only returns a field, is an alias. Call the target where
it is used.

```cpp
/* no */
static Level& _GetLevel(Project& project) {
    return project.level;
}

auto const save = [&]{ saveProject(); };

/* yes */
project.level
saveProject()
```

It is justified only when the target cannot be reached directly: a lambda
that binds `this` so a callback can reach a private member function, or a
lambda that adapts arguments to the signature a callback requires.

A helper called from one place is written at that place, unless it names
a step. It is written inline when its body is a single expression or a few
statements that read as plainly as its name would. It stays a function
when it is one step of a longer process and its name says what the step
does, or when written inline it would bury the code around it.

```cpp
/* no: the name says no more than the code */
static bool _IsWheelEvent(::SDL_Event const& event) {
    return (event.type == ::SDL_EVENT_MOUSE_WHEEL);
}

if (_IsWheelEvent(event)) { ... }

/* yes */
if (event.type == ::SDL_EVENT_MOUSE_WHEEL) { ... }

/* yes: each one is a step of a body's move, named by what it does */
body.position.x = _StopAtCeilings(_ceilings, body, x);
_CrossFences(_fences, _platforms, body, y, _levelHeight);
```

## 2. No negation operator

Never use `!`. Compare against the value directly, so the condition reads
the same way whichever outcome it tests.

```cpp
/* no */
if (!disabled) { ... }
if (!state.onClose) { ... }
return !operator==(other);

/* yes */
if (disabled == false) { ... }
if (state.onClose == nullptr) { ... }
return (operator==(other) == false);
```

To flip a flag, negation is allowed:

```cpp
state.open = !state.open;
```

`!=` is a direct comparison and is fine.

`== true` may be omitted, but only for a real `bool`. Anything that is merely
truthy (a pointer, a `std::function`, a `std::optional`) is compared
explicitly.

```cpp
/* fine: real bools */
if (disabled) { ... }
if (disabled == true) { ... }
if (state.runOnce()) { ... }

/* no: truthy, not bool */
if (state.onClose) { ... }
if (documentRef) { ... }
if (props.label) { ... }

/* yes */
if (state.onClose != nullptr) { ... }
if (documentRef != nullptr) { ... }
if (props.label.has_value()) { ... }
```

One exception: a pointer declared in an `if` condition by `event.as<T>()` or
`std::get_if<T>()` is tested bare. The declaration reads as "if it is a T",
the same way a type check would.

```cpp
/* fine */
if (auto mouseDragEvent = event.as<MouseDragNodeEvent>()) { ... }
if (auto const number = std::get_if<double>(&value)) { ... }

/* no: any other value declared in the condition is compared explicitly */
if (auto const frame = _FindFrameAt(view, point)) { ... }

/* yes */
if (auto const frame = _FindFrameAt(view, point); frame != nullptr) { ... }
```

## 3. No comments in sources and private headers

Do not put comments in source files (`.cpp`, `.mm`) or in private headers
(`<Module>_Private.hpp` and the platform headers `<Module>_<Platform>.hpp`,
see [§8](#8-private-names)). Code must be self-explanatory through its names
and structure; if it seems to need a comment, rename or restructure it first.

This covers every kind of comment:

- documentation comments: no `/** ... */` blocks on types, fields or
  functions, no `@param` lists;
- comments that describe what the next lines do;
- section banners and labels inside a function.

In a source file, a comment is written only inside a function body, and only
when it is really needed: something non-obvious that cannot be made obvious
by the code, such as why a value is what it is or a constraint the reader
could not guess. What is private carries no comment at all: a private header,
and in a source file a private enum, struct, field, static variable or
function. What such a comment would say goes into a name or a type instead.

A backend marks a feature its platform does not provide with
`// NOTE: Not supported in <Platform>` where it falls back, so the gaps of
each backend are found with one search.

```cpp
/* no: a field of a private struct */
/** Whether the pointer is over the node. */
bool hover;

/* no */
/* Load every collection the project lists. */
for (auto const& item : AsJSONArray(collections->second)) {
    ...
}

/* yes */
/* The edge is drawn `border * factor` device pixels wide; a node unit is `scale` device pixels. */
auto const border = ((static_cast<float>(it->second.border) * factor) / options.scale);
```

Public headers keep their documentation comments: every public enum, struct,
field and function has a `/** ... */` comment, with `@param` for function
parameters.

The license header at the top of each file and the comment closing a
namespace (`} /* namespace Rocket */`) stay everywhere.

## 4. Symbol order in a source file

This applies to every `.cpp`, not only to components. Declarations appear in
this fixed order, each group complete before the next starts:

1. **Enums** private to the file.
2. **Structs** private to the file.
3. **Static variables**: colors, lookup tables, metric constants.
4. **Static functions**: helpers used further down.
5. **Public functions**: the functions declared in the header.

```cpp
namespace Rocket {

/* 1. private enums */
/* 2. private structs      _MaterialOptionsLess, _MaterialVariant */
/* 3. static variables     _image1x, _image2x, _image3x, _variants */
/* 4. static functions */
/* 5. public functions     GetMaterial() */

} /* namespace Rocket */
```

The order is also a dependency order. Tables (3) may refer to structs (2)
and to earlier variables, and functions (4) may refer to anything above
them, but a table initializer must never call a static function, because
that function would have to be declared before the variables. If a value
needs computing, give it its own static variable built from earlier ones
rather than a helper call.

```cpp
/* no */
static auto const _cellWidth = _ComputeCellWidth();

/* yes */
static auto const _cellWidth = ((_swatchWidth * 3.0f) + (_swatchGap * 2.0f));
```

## 5. Headers with implementation

When a `.hpp` contains implementation (inline functions, templates), it is
laid out in two parts: all declarations first, then all implementations.
The top of the file reads like any other header.

```cpp
#pragma once

#include <filesystem>
#include <Rocket/Base/JSON.hpp>

namespace Rocket {

/**
 * Reads and parses a JSON file.
 *
 * @param path File to read.
 */
JSONValue ReadJSONFile(std::filesystem::path const& path);

/**
 * Serializes a value and writes it to a JSON file.
 *
 * @param path  File to write.
 * @param value Value to serialize.
 */
void WriteJSONFile(std::filesystem::path const& path, JSONValue const& value);

inline JSONValue ReadJSONFile(std::filesystem::path const& path) {
    ...
}

inline void WriteJSONFile(std::filesystem::path const& path, JSONValue const& value) {
    ...
}

} /* namespace Rocket */
```

## 6. Exceptions

An exception is a `std::runtime_error`. Its message says what the code was
doing when it failed, starting with `Failed to`.

An exception is caught as `std::exception const& e`, never with
`catch (...)`. A level that catches one and cannot deal with it throws it on
with `std::throw_with_nested`, in a new `std::runtime_error` whose message is
what this level was doing, followed by `: ` and the message of the exception
it caught.

```cpp
/* no */
} catch (...) {
    std::throw_with_nested(
        std::runtime_error("Failed to load the project")
    );
}

/* yes */
} catch (std::exception const& e) {
    std::throw_with_nested(
        std::runtime_error(
            std::format("Failed to load the project: {}", e.what())
        )
    );
}
```

The message of the outermost exception is therefore complete on its own and
reads from the outside in:
`Failed to save the project: Failed to write file: Levels/Street.json`.
Whoever reports it shows `what()` and does not unwind the nested exceptions.

## 7. Lambda captures

What a lambda captures by reference is not listed: the capture block opens
with `&`. Only what is captured by value is named, after it. `this` counts
as captured by reference.

```cpp
/* no */
.onClick = [&state, &reconciler, index]{
    ...
}

/* yes */
.onClick = [&, index]{
    ...
}
```

A lambda that captures nothing by reference names its copies alone:
`[saveSettings]`.

Capturing by reference does not keep a local alive. A lambda that is stored
and called later captures by reference only what outlives it, such as
`state`, and copies the locals of the render in by value. See "Handlers
outlive renders" in [Component §9](Component.md#9-reconciler-gotchas).

## 8. Private names

A name that is not part of the public interface starts with an underscore;
a public name never does.

One underscore marks a private name: a symbol private to its file, a private
member of a class (`_impl`, `_show()`), or a function a module shares between
its own files (`_GamepadDown()`).

| Kind | Form | Examples |
|---|---|---|
| Private struct / enum | `_PascalCase` | `_MaterialVariant`, `_PopupState` |
| Static variable, a constant too | `_camelCase` | `_blue500`, `_variants`, `_blurTapMax` |
| Static function | `_PascalCase` | `_GetMapped`, `_FindFrameAt` |
| Local variable | `camelCase` (no prefix) | `state`, `nodeRef`, `isHover` |

Two underscores mark a platform-specific name. The shared code forwards to
a function that each platform backend implements: `Window::setVisible()`
calls `Window::__setVisible()`, which `Window_MacOS.mm`, `Window_SDL3.cpp`
and `Window_Win32.cpp` each define. A type or function that the files of one
platform share carries two underscores as well (`__NSWindow`,
`__InitWin32()`).

What a module's files share is declared in a `<Module>_Private.hpp` next to
the module (see `src/Rocket/UI/Node_Private.hpp`); what the files of one
platform share is declared in a `<Module>_<Platform>.hpp` (`Window_SDL3.hpp`,
`App_Win32.hpp`).

## 9. Types and literals

`auto` first: `auto const x = ...;`, `auto& state = UseState<_PopupState>();`.
Spell the type only when it cannot be deduced.

Floating-point literals always carry their suffix: `0.0f`, `6.0f`, `1.5f`,
never `6`.

## 10. Layout

Includes are ordered: system headers, then `Rocket/...` headers
alphabetically by path, then the file's own header last.

Every `if` has braces, even a one-liner, and an `if` that follows
assignments is preceded by a blank line.

A small mapping is a nested conditional expression rather than an `if`
ladder. It is parenthesized and laid out one case per line, the final
fallback indented to line up with the values:

```cpp
auto const justify = (
    alignment == ButtonAlignment::Start ? NodeJustify::Start :
    alignment == ButtonAlignment::End ? NodeJustify::End :
                                        NodeJustify::Center
);
```

## 11. Global C names

A name a C library or a system API declares in the global namespace is
written with a leading `::`: functions, types, enumerators and global
variables alike, from SDL, Win32, FreeType, OpenGL, libcurl and the like.
The `::` marks a call or a type as leaving Rocket's own code, and keeps a
Rocket name from being picked by mistake.

```cpp
/* no */
SDL_Event event;
while (SDL_PollEvent(&event)) { ... }
auto const hwnd = static_cast<HWND>(handle);

/* yes */
auto event = ::SDL_Event{};
while (::SDL_PollEvent(&event)) { ... }
auto const hwnd = static_cast<::HWND>(handle);
```

A macro cannot take `::` and is written as it is: `SDL_INIT_VIDEO`,
`WAIT_OBJECT_0`, `GL_TEXTURE_2D`. Names of the C standard library are
reached through `std::` instead (`std::memcpy`, `std::uint32_t`).
Objective-C is written as it is: its classes, its messages, and the types
in its method declarations (`NSString`, `[NSApp run]`,
`- (void)setMarkedText:(id)string selectedRange:(NSRange)range`).
