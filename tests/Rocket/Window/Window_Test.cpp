/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <tuple>
#include <string>
#include <vector>
#include <algorithm>
#include <Rocket/Window/App.hpp>
#include <Rocket/Window/Window.hpp>
#include <gtest/gtest.h>

#ifdef __linux__
#include <SDL3/SDL.h>
#endif

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <cstdint>
#include <cstring>
#include <windows.h>
#include <shlobj.h>
#include <Rocket/Window/Input.hpp>
#endif

using namespace Rocket;

/* Note: windows are never made visible in this test, so no window flashes
   on screen while the suite runs. */

/* Construction registers the window; destruction unregisters it. */
TEST(Window, ConstructionRegistersDestructionUnregisters) {
    auto countBefore = Window::GetWindows().size();

    {
        auto window = Window();
        auto& windows = Window::GetWindows();
        ASSERT_TRUE(windows.size() == countBefore + 1);
        ASSERT_TRUE(std::find(windows.begin(), windows.end(), &window) != windows.end());
    }

    ASSERT_TRUE(Window::GetWindows().size() == countBefore);
}

/* Properties round-trip through their setters and getters. */
TEST(Window, PropertiesRoundTripThroughSettersAndGetters) {
    auto window = Window();

    window.setTitle("Rocket Test Window");
    ASSERT_TRUE(window.getTitle() == "Rocket Test Window");

    window.setSize({ 640.0f, 480.0f });
    ASSERT_TRUE(window.getSize() == Vec2(640.0f, 480.0f));

    window.setMaximizable(true);
    ASSERT_TRUE(window.getMaximizable() == true);

    window.setClosable(false);
    ASSERT_TRUE(window.getClosable() == false);
    window.setClosable(true);
    ASSERT_TRUE(window.getClosable() == true);

    window.setSizable(false);
    ASSERT_TRUE(window.getSizable() == false);

    window.setMaximizable(false);
    ASSERT_TRUE(window.getMaximizable() == false);

    window.setMinimizable(false);
    ASSERT_TRUE(window.getMinimizable() == false);
}

/* The position round-trips through its setter and getter. */
TEST(Window, PositionRoundTripsThroughSetterAndGetter) {
#ifdef __linux__
    // TODO: Run on SDL3 once the video driver SDL chooses can place windows (Wayland cannot).
    GTEST_SKIP() << "depends on the video driver SDL chooses";
#endif

    auto window = Window();

    window.setPosition({ 100.0f, 120.0f });
    ASSERT_TRUE(window.getPosition() == Vec2(100.0f, 120.0f));
}

/* Topmost round-trips through its setter and getter. */
TEST(Window, TopmostRoundTripsThroughSetterAndGetter) {
#ifdef __linux__
    // TODO: Run on SDL3 once the video driver SDL chooses can keep windows on top (Wayland cannot).
    GTEST_SKIP() << "depends on the video driver SDL chooses";
#endif

    auto window = Window();

    window.setTopmost(true);
    ASSERT_TRUE(window.getTopmost() == true);
    window.setTopmost(false);
    ASSERT_TRUE(window.getTopmost() == false);
}

/* Cursor: defaults to Cursor::Default and round-trips through setCursor,
   even on a hidden window. */
TEST(Window, CursorDefaultsAndRoundTrips) {
    auto window = Window();
    ASSERT_TRUE(window.getCursor() == Cursor::Default);

    window.setCursor(Cursor::Pointer);
    ASSERT_TRUE(window.getCursor() == Cursor::Pointer);

    window.setCursor(Cursor::Default);
    ASSERT_TRUE(window.getCursor() == Cursor::Default);
}

/* A new window starts hidden, unfocused, and windowed. */
TEST(Window, NewWindowStartsHiddenUnfocusedAndWindowed) {
    auto window = Window();
    ASSERT_TRUE(window.getVisible() == false);
    ASSERT_TRUE(window.isMaximized() == false);
    ASSERT_TRUE(window.isMinimized() == false);
    ASSERT_TRUE(window.isFullscreen() == false);
    ASSERT_TRUE(window.getHandle() != nullptr);
    ASSERT_TRUE(window.getScale() > 0.0f);
}

/* Multiple windows coexist in the registry. */
TEST(Window, MultipleWindowsCoexistInRegistry) {
    auto countBefore = Window::GetWindows().size();
    auto first = Window();
    auto second = Window();
    ASSERT_TRUE(Window::GetWindows().size() == countBefore + 2);
}

/* Showing and hiding publish exactly one Show/HideWindowEvent each. */
TEST(Window, ShowAndHidePublishOneEventEach) {
    auto window = Window();
    auto showCount = 0;
    auto hideCount = 0;

    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (event.is<ShowWindowEvent>()) {
            showCount += 1;
        } else if (event.is<HideWindowEvent>()) {
            hideCount += 1;
        }
    });

    window.setVisible(true);
    ASSERT_TRUE(window.getVisible() == true);
    ASSERT_TRUE(showCount == 1);
    ASSERT_TRUE(hideCount == 0);

    window.setVisible(false);
    ASSERT_TRUE(window.getVisible() == false);
    ASSERT_TRUE(showCount == 1);
    ASSERT_TRUE(hideCount == 1);
}

/* The input area starts unset and round-trips through setInputArea, including
   clearing it again. */
TEST(Window, InputAreaDefaultsAndRoundTrips) {
    auto window = Window();
    ASSERT_TRUE(window.getInputArea().has_value() == false);

    window.setInputArea(Vec4{ 10.0f, 20.0f, 2.0f, 16.0f });
    ASSERT_TRUE(window.getInputArea().has_value());
    ASSERT_TRUE(*window.getInputArea() == Vec4(10.0f, 20.0f, 2.0f, 16.0f));

    window.setInputArea(std::nullopt);
    ASSERT_TRUE(window.getInputArea().has_value() == false);
}

/* Input and composition events carry their text and, for composition, the
   caret and selected segment; both reach onEvent subscribers. */
TEST(Window, InputAndCompositionEventsCarryTheirPayloads) {
    auto window = Window();

    auto texts = std::vector<std::string>();
    auto compositions = std::vector<std::tuple<std::string, std::int32_t, std::int32_t>>();
    auto sub = Sub<WindowEvent const&>(window.onEvent, [&](WindowEvent const& event) {
        if (auto e = event.as<InputWindowEvent>()) texts.push_back(e->getText());
        if (auto e = event.as<CompositionWindowEvent>()) compositions.push_back({ e->getText(), e->getCursor(), e->getSelectionLength() });
    });

    window.onEvent.publish(CompositionWindowEvent(window, "にほんご", 2, 2));
    window.onEvent.publish(CompositionWindowEvent(window, "", 0, 0));
    window.onEvent.publish(InputWindowEvent(window, "日本語"));

    ASSERT_TRUE(compositions.size() == 2);
    ASSERT_TRUE(compositions.at(0) == std::make_tuple(std::string("にほんご"), 2, 2));
    ASSERT_TRUE(std::get<0>(compositions.at(1)).empty());
    ASSERT_TRUE(texts == std::vector<std::string>({ "日本語" }));
}

#ifdef __linux__

/* Pushes an SDL event for the window, as the platform would deliver it. */
static void _PushEvent(Window& window, SDL_Event event) {
    event.common.timestamp = SDL_GetTicksNS();
    SDL_PushEvent(&event);
}

static void _PushMotion(Window& window, float x, float y, SDL_MouseButtonFlags state = 0) {
    auto event = SDL_Event{};
    event.motion.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.state = state;
    event.motion.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(window.getHandle()));
    event.motion.x = x;
    event.motion.y = y;
    _PushEvent(window, event);
}

static void _PushWheel(Window& window, float x, float y) {
    auto event = SDL_Event{};
    event.wheel.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(window.getHandle()));
    event.wheel.x = x;
    event.wheel.y = y;
    _PushEvent(window, event);
}

static void _PushButton(Window& window, SDL_EventType type, float x, float y) {
    auto event = SDL_Event{};
    event.button.type = type;
    event.button.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(window.getHandle()));
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = (type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    event.button.clicks = 1;
    event.button.x = x;
    event.button.y = y;
    _PushEvent(window, event);
}

/* Runs the app loop for a moment, so it handles the pushed events. */
static void _RunBriefly() {
    App::SetTimeout([] { App::Exit(); }, 50.0);
    App::Run(0, nullptr);
}

/* Moves that arrive together reach the window as one, at the last position;
   a press between them keeps its place in the order. */
TEST(Window, MovesArrivingTogetherCoalesce) {
    auto window = Window();
    auto events = std::vector<std::string>();
    auto positions = std::vector<Vec2>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto move = event.as<MouseMoveWindowEvent>()) {
            events.push_back("move");
            positions.push_back(move->getPosition());
        } else if (event.is<MouseDownWindowEvent>()) {
            events.push_back("down");
        } else if (event.is<MouseUpWindowEvent>()) {
            events.push_back("up");
        }
    });

    for (auto i = 1; i <= 50; i++) {
        _PushMotion(window, static_cast<float>(i), 10.0f);
    }
    _PushButton(window, SDL_EVENT_MOUSE_BUTTON_DOWN, 50.0f, 10.0f);
    for (auto i = 51; i <= 100; i++) {
        _PushMotion(window, static_cast<float>(i), 10.0f, SDL_BUTTON_LMASK);
    }
    _PushButton(window, SDL_EVENT_MOUSE_BUTTON_UP, 100.0f, 10.0f);

    _RunBriefly();

    ASSERT_TRUE(events == std::vector<std::string>({ "move", "down", "move", "up" }));
    ASSERT_TRUE(positions.front() == Vec2(50.0f, 10.0f));
    ASSERT_TRUE(positions.back() == Vec2(100.0f, 10.0f));
}

/* Wheel turns that arrive together reach the window as one, their sum. */
TEST(Window, WheelTurnsArrivingTogetherSum) {
    auto window = Window();
    auto wheels = std::vector<Vec2>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto wheel = event.as<MouseWheelWindowEvent>()) {
            wheels.push_back(wheel->getPosition());
        }
    });

    _PushWheel(window, 0.0f, 2.0f);
    _RunBriefly();

    for (auto i = 0; i < 4; i++) {
        _PushWheel(window, 0.0f, 0.5f);
    }
    _RunBriefly();

    ASSERT_TRUE(wheels.size() == 2);
    ASSERT_TRUE(wheels.at(1) == wheels.at(0)); /* four half notches turn as far as two notches */
}

#endif

#ifdef _WIN32

/* Posts a message to the window, as Windows would deliver it. */
static void _PostMessage(Window& window, std::uint32_t message, std::uintptr_t wParam, std::intptr_t lParam) {
    ::PostMessageW(static_cast<HWND>(window.getHandle()), message, wParam, lParam);
}

/* The client-area coordinates of a point given in points. */
static LPARAM _Point(Window& window, float x, float y) {
    auto const scale = window.getScale();
    return MAKELPARAM(static_cast<int>(x * scale), static_cast<int>(y * scale));
}

/* Runs the app loop for a moment, so it handles the posted messages. */
static void _RunBriefly() {
    App::SetTimeout([] { App::Exit(); }, 50.0);
    App::Run(0, nullptr);
}

/* Moves that arrive together reach the window as one, at the last position;
   a press between them keeps its place in the order. */
TEST(Window, MovesArrivingTogetherCoalesce) {
    auto window = Window();
    auto events = std::vector<std::string>();
    auto positions = std::vector<Vec2>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto move = event.as<MouseMoveWindowEvent>()) {
            events.push_back("move");
            positions.push_back(move->getPosition());
        } else if (event.is<MouseDownWindowEvent>()) {
            events.push_back("down");
        } else if (event.is<MouseUpWindowEvent>()) {
            events.push_back("up");
        }
    });

    for (auto i = 1; i <= 50; i++) {
        _PostMessage(window, WM_MOUSEMOVE, 0, _Point(window, static_cast<float>(i), 10.0f));
    }
    _PostMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, _Point(window, 50.0f, 10.0f));
    for (auto i = 51; i <= 100; i++) {
        _PostMessage(window, WM_MOUSEMOVE, MK_LBUTTON, _Point(window, static_cast<float>(i), 10.0f));
    }
    _PostMessage(window, WM_LBUTTONUP, 0, _Point(window, 100.0f, 10.0f));

    _RunBriefly();

    ASSERT_TRUE(events == std::vector<std::string>({ "move", "down", "move", "up" }));
    ASSERT_TRUE(positions.front() == Vec2(50.0f, 10.0f));
    ASSERT_TRUE(positions.back() == Vec2(100.0f, 10.0f));
}

/* Wheel turns that arrive together reach the window as one, their sum. */
TEST(Window, WheelTurnsArrivingTogetherSum) {
    auto window = Window();
    auto wheels = std::vector<Vec2>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto wheel = event.as<MouseWheelWindowEvent>()) {
            wheels.push_back(wheel->getPosition());
        }
    });

    _PostMessage(window, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA * 2), 0);
    _RunBriefly();

    for (auto i = 0; i < 4; i++) {
        _PostMessage(window, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA / 2), 0);
    }
    _RunBriefly();

    ASSERT_TRUE(wheels.size() == 2);
    ASSERT_TRUE(wheels.at(1) == wheels.at(0)); /* four half notches turn as far as two notches */
}

/* Sends a message to the window procedure, as Windows would deliver it, and returns its result. */
static LRESULT _Send(Window& window, UINT message, WPARAM wParam, LPARAM lParam) {
    return ::SendMessageW(static_cast<HWND>(window.getHandle()), message, wParam, lParam);
}

/* A key message's lParam: the scan code, the extended flag and the repeat flag. */
static LPARAM _KeyLParam(UINT scancode, bool extended, bool repeat = false) {
    return static_cast<LPARAM>(1 | (scancode << 16) | (extended ? (1u << 24) : 0u) | (repeat ? (1u << 30) : 0u));
}

/* Presses of one button at one place count up; a press elsewhere starts over. */
TEST(Window, PressesCountClicks) {
    auto window = Window();
    auto counts = std::vector<int>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto down = event.as<MouseDownWindowEvent>()) {
            counts.push_back(down->getClickCount());
        }
    });

    for (auto i = 0; i < 3; i++) {
        _Send(window, WM_LBUTTONDOWN, MK_LBUTTON, _Point(window, 20.0f, 20.0f));
        _Send(window, WM_LBUTTONUP, 0, _Point(window, 20.0f, 20.0f));
    }

    _Send(window, WM_LBUTTONDOWN, MK_LBUTTON, _Point(window, 200.0f, 200.0f));
    _Send(window, WM_LBUTTONUP, 0, _Point(window, 200.0f, 200.0f));

    ASSERT_TRUE(counts == std::vector<int>({ 1, 2, 3, 1 }));
}

/* A notch of the wheel turns 50 points; the horizontal wheel is positive to the left. */
TEST(Window, WheelTurnsInPoints) {
    auto window = Window();
    auto wheels = std::vector<Vec2>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto wheel = event.as<MouseWheelWindowEvent>()) {
            wheels.push_back(wheel->getPosition());
        }
    });

    _Send(window, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0);
    _Send(window, WM_MOUSEHWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0);

    ASSERT_TRUE(wheels == std::vector<Vec2>({ Vec2(0.0f, 50.0f), Vec2(-50.0f, 0.0f) }));
}

/* Releases a lost capture cannot deliver are made up, so presses stay paired. */
TEST(Window, LostCaptureReleasesButtons) {
    auto window = Window();
    auto events = std::vector<std::string>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (event.is<MouseDownWindowEvent>()) {
            events.push_back("down");
        } else if (event.is<MouseUpWindowEvent>()) {
            events.push_back("up");
        }
    });

    _Send(window, WM_LBUTTONDOWN, MK_LBUTTON, _Point(window, 20.0f, 20.0f));
    _Send(window, WM_CAPTURECHANGED, 0, 0);
    ::ReleaseCapture();
    _Send(window, WM_LBUTTONDOWN, MK_LBUTTON, _Point(window, 20.0f, 20.0f));
    _Send(window, WM_LBUTTONUP, 0, _Point(window, 20.0f, 20.0f));

    ASSERT_TRUE(events == std::vector<std::string>({ "down", "up", "down", "up" }));
}

/* Keys are their physical position: Windows' scan codes, with its extended
   keys and its swapped Pause and NumLock. */
TEST(Window, KeysMapScanCodes) {
    auto window = Window();
    auto keys = std::vector<std::tuple<Scancode, std::string, bool>>();
    auto ups = std::vector<Scancode>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto down = event.as<KeyDownWindowEvent>()) {
            keys.push_back({ down->getScancode(), down->getKeycode(), down->isRepeat() });
        } else if (auto up = event.as<KeyUpWindowEvent>()) {
            ups.push_back(up->getScancode());
        }
    });

    _Send(window, WM_KEYDOWN, VK_UP, _KeyLParam(0x48, true));
    _Send(window, WM_KEYDOWN, VK_UP, _KeyLParam(0x48, true, true));
    _Send(window, WM_KEYDOWN, VK_NUMPAD8, _KeyLParam(0x48, false));
    _Send(window, WM_KEYDOWN, VK_PAUSE, _KeyLParam(0x45, false));
    _Send(window, WM_KEYDOWN, VK_NUMLOCK, _KeyLParam(0x45, true));
    _Send(window, WM_KEYDOWN, VK_RCONTROL, _KeyLParam(0x1D, true));
    _Send(window, WM_SYSKEYDOWN, VK_F10, _KeyLParam(0x44, false));
    _Send(window, WM_KEYUP, VK_SNAPSHOT, _KeyLParam(0x37, true));

    ASSERT_TRUE(keys.size() == 8);
    ASSERT_TRUE(keys.at(0) == std::make_tuple(Scancode::ArrowUp, GetDefaultKeycode(Scancode::ArrowUp), false));
    ASSERT_TRUE(keys.at(1) == std::make_tuple(Scancode::ArrowUp, GetDefaultKeycode(Scancode::ArrowUp), true));
    ASSERT_TRUE(std::get<0>(keys.at(2)) == Scancode::Numpad8);
    ASSERT_TRUE(std::get<0>(keys.at(3)) == Scancode::Pause);
    ASSERT_TRUE(std::get<0>(keys.at(4)) == Scancode::NumLock);
    ASSERT_TRUE(std::get<0>(keys.at(5)) == Scancode::ControlRight);
    ASSERT_TRUE(std::get<0>(keys.at(6)) == Scancode::F10);
    ASSERT_TRUE(std::get<0>(keys.at(7)) == Scancode::PrintScreen); /* PrintScreen sends only its release */
    ASSERT_TRUE(ups == std::vector<Scancode>({ Scancode::PrintScreen }));
}

/* A character key reads as what the layout types on it, without modifiers. */
TEST(Window, CharacterKeysFollowTheLayout) {
    auto const previous = ::GetKeyboardLayout(0);
    auto const english = ::LoadKeyboardLayoutW(L"00000409", 0);

    if (english == nullptr) {
        GTEST_SKIP() << "the US layout is not available";
    }

    ::ActivateKeyboardLayout(english, 0);

    auto window = Window();
    auto keycodes = std::vector<std::string>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto down = event.as<KeyDownWindowEvent>()) {
            keycodes.push_back(down->getKeycode());
        }
    });

    _Send(window, WM_KEYDOWN, 'Q', _KeyLParam(0x10, false));
    _Send(window, WM_KEYDOWN, VK_OEM_1, _KeyLParam(0x27, false));
    auto const label = GetScancodeLabel(Scancode::KeyQ);

    ::ActivateKeyboardLayout(previous, 0);

    ASSERT_TRUE(keycodes == std::vector<std::string>({ "q", ";" }));
    ASSERT_TRUE(label == "Q");
}

/* Typed text reaches the window only while it has an input area, joined from
   UTF-16 halves and without control characters. */
TEST(Window, TextArrivesWithAnInputArea) {
    auto window = Window();
    auto texts = std::vector<std::string>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto input = event.as<InputWindowEvent>()) {
            texts.push_back(input->getText());
        }
    });

    _Send(window, WM_CHAR, 'a', 0);

    window.setInputArea(Vec4{ 10.0f, 10.0f, 1.0f, 16.0f });
    _Send(window, WM_CHAR, 'b', 0);
    _Send(window, WM_CHAR, 0x01, 0);
    _Send(window, WM_CHAR, 0xD83D, 0);
    _Send(window, WM_CHAR, 0xDE00, 0);
    _Send(window, WM_CHAR, 0x044F, 0);

    window.setInputArea(std::nullopt);
    _Send(window, WM_CHAR, 'c', 0);

    ASSERT_TRUE(texts == std::vector<std::string>({ "b", "\xF0\x9F\x98\x80", "\xD1\x8F" }));
}

/* The files of one drop arrive together, where they were dropped. */
TEST(Window, DroppedFilesArriveTogether) {
    auto window = Window();
    auto drops = std::vector<std::pair<std::vector<std::string>, Vec2>>();
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (auto drop = event.as<DropFilesWindowEvent>()) {
            drops.push_back({ drop->getPaths(), drop->getPosition() });
        }
    });

    /* what Explorer hands over: a DROPFILES header, then the paths, each
       ended by a zero, then one more zero */
    auto paths = std::wstring(L"C:\\one.png");
    paths.push_back(L'\0');
    paths += L"C:\\\u0434\u0432\u0430.png";
    paths.push_back(L'\0');
    paths.push_back(L'\0');

    auto const scale = window.getScale();
    auto const memory = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(DROPFILES) + (paths.size() * sizeof(wchar_t)));
    auto const header = static_cast<DROPFILES*>(::GlobalLock(memory));

    header->pFiles = sizeof(DROPFILES);
    header->pt = POINT{ static_cast<LONG>(30.0f * scale), static_cast<LONG>(40.0f * scale) };
    header->fNC = FALSE;
    header->fWide = TRUE;
    std::memcpy(reinterpret_cast<std::uint8_t*>(header) + sizeof(DROPFILES), paths.data(), (paths.size() * sizeof(wchar_t)));
    ::GlobalUnlock(memory);

    _Send(window, WM_DROPFILES, reinterpret_cast<WPARAM>(memory), 0); /* the window frees it */

    ASSERT_TRUE(drops.size() == 1);
    ASSERT_TRUE(drops.at(0).first == std::vector<std::string>({ "C:\\one.png", "C:\\\xD0\xB4\xD0\xB2\xD0\xB0.png" }));
    ASSERT_TRUE(drops.at(0).second == Vec2(30.0f, 40.0f));
}

/* The close control asks to close only while the window is closable; the
   window never closes by itself. */
TEST(Window, CloseFollowsClosable) {
    auto window = Window();
    auto closes = 0;
    auto sub = Sub(window.onEvent, [&](WindowEvent const& event) {
        if (event.is<CloseWindowEvent>()) {
            closes++;
        }
    });

    _Send(window, WM_SYSCOMMAND, SC_CLOSE, 0);
    ASSERT_TRUE(closes == 0);

    window.setClosable(true);
    _Send(window, WM_SYSCOMMAND, SC_CLOSE, 0);
    ASSERT_TRUE(closes == 1);
    ASSERT_TRUE(::IsWindow(static_cast<HWND>(window.getHandle())));
}

/* The flags become the window's frame, and the content keeps its size
   through every change. */
TEST(Window, FrameFollowsFlagsAndKeepsTheSize) {
    auto window = Window();
    auto const hwnd = static_cast<HWND>(window.getHandle());
    auto const style = [&]() { return ::GetWindowLongPtrW(hwnd, GWL_STYLE); };

    window.setSize({ 400.0f, 300.0f });

    window.setSizable(true);
    window.setMinimizable(true);
    window.setMaximizable(false);

    ASSERT_TRUE((style() & WS_THICKFRAME) != 0);
    ASSERT_TRUE((style() & WS_MINIMIZEBOX) != 0);
    ASSERT_TRUE((style() & WS_MAXIMIZEBOX) == 0);
    ASSERT_TRUE(window.getSize() == Vec2(400.0f, 300.0f));

    window.setFullscreen(true);
    ASSERT_TRUE(window.isFullscreen());
    ASSERT_TRUE((style() & WS_POPUP) != 0);

    window.setFullscreen(false);
    ASSERT_TRUE(window.isFullscreen() == false);
    ASSERT_TRUE((style() & WS_THICKFRAME) != 0);
    ASSERT_TRUE(window.getSize() == Vec2(400.0f, 300.0f));
    ASSERT_TRUE(window.getVisible() == false);
}

#endif
