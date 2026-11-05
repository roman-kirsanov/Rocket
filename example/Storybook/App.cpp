/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstddef>
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Color.hpp>
#include <Rocket/UI/Document.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme/PixelDark/Theme.hpp>
#include <Rocket/UI/Window.hpp>
#include <Components/ColorScheme.hpp>
#include <Components/ColorSchemeToggle.hpp>
#include <Components/NavLink.hpp>
#include <Components/Select.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Pages/ButtonPage.hpp>
#include <Pages/CheckboxPage.hpp>
#include <Pages/MaterialPage.hpp>
#include <Pages/RadioboxPage.hpp>
#include <Pages/SwitchPage.hpp>
#include <App.hpp>

using namespace Rocket;

namespace Storybook {

struct _AppColors {
    Vec4 background;
    Vec4 sidebar;
    Vec4 border;
};

struct _AppPage {
    std::string title;
    std::function<void()> render;
};

struct _AppTheme {
    std::string name;
    Theme const& (*get)();
    StageColors stage;
};

struct _AppState {
    std::size_t page = 0;
    std::size_t theme = 0;
    ColorScheme colorScheme = ColorScheme::Light;
    std::function<void()> onClose;
};

auto constexpr _gray0 = Vec4{ 248.0f / 255.0f, 249.0f / 255.0f, 250.0f / 255.0f, 1.0f }; /* #f8f9fa */
auto constexpr _gray2 = Vec4{ 233.0f / 255.0f, 236.0f / 255.0f, 239.0f / 255.0f, 1.0f }; /* #e9ecef */
auto constexpr _dark5 = Vec4{ 59.0f / 255.0f, 59.0f / 255.0f, 59.0f / 255.0f, 1.0f };    /* #3b3b3b */
auto constexpr _dark7 = Vec4{ 36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f };    /* #242424 */
auto constexpr _dark8 = Vec4{ 31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f };    /* #1f1f1f */
auto constexpr _white = COLOR_WHITE;

static auto const _colors = std::map<ColorScheme, _AppColors>{
    { ColorScheme::Light, { _white, _gray0, _gray2 } },
    { ColorScheme::Dark, { _dark7, _dark8, _dark5 } }
};

static auto const _pages = std::vector<_AppPage>{
    { "Button", []{ ButtonPage({}); } },
    { "Checkbox", []{ CheckboxPage({}); } },
    { "Radiobox", []{ RadioboxPage({}); } },
    { "Switch", []{ SwitchPage({}); } },
    { "Material", []{ MaterialPage({}); } }
};
static auto const _themes = std::vector<_AppTheme>{
    { "Default Dark", &GetDefaultDarkTheme, GetDarkStageColors() },
    { "Default Light", &GetDefaultLightTheme, GetLightStageColors() },
    { "PixelDark", &GetPixelDarkTheme, GetDarkStageColors() }
};
static auto const _windowSize = Vec2{ 1280.0f, 820.0f };
static auto const _sidebarWidth = 240.0f;
static auto const _sidebarPadding = 16.0f;
static auto const _sidebarGap = 4.0f;
static auto const _sidebarHeaderPaddingX = 12.0f;
static auto const _sidebarHeaderMargin = 16.0f;
static auto const _themePickerMargin = 16.0f;
static auto const _borderWidth = 1.0f;
static auto const _pagePaddingX = 40.0f;
static auto const _pagePaddingTop = 40.0f;
static auto const _pagePaddingBottom = 80.0f;
static auto const _pageMaxWidth = 1000.0f;
static auto const _pageGap = 24.0f;

void App(AppProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_AppState>();

    state.onClose = props.onClose;

    auto const& colors = _colors.at(state.colorScheme);

    SetContext(state.colorScheme);

    Window({
        .title = "Rocket Storybook",
        .size = _windowSize,
        .closable = true,
        .sizable = true,
        .maximizable = true,
        .minimizable = true,
        .visible = true,
        .onEvent = [&](WindowEvent const& event) {
            if (event.is<CloseWindowEvent>() && state.onClose != nullptr) {
                state.onClose();
            }
        }
    }, [&]{
        Document({
            .nodeProps = {
                .direction = NodeDirection::Horizontal,
                .alignment = NodeAlignment::Stretch,
                .background = ColorBrush{ colors.background }
            }
        }, [&]{
            Node({
                .direction = NodeDirection::Vertical,
                .alignment = NodeAlignment::Stretch,
                .width = _sidebarWidth,
                .padding = _sidebarPadding,
                .gapY = _sidebarGap,
                .borderRightWidth = _borderWidth,
                .background = ColorBrush{ colors.sidebar },
                .border = ColorBrush{ colors.border }
            }, [&]{
                Node({
                    .direction = NodeDirection::Horizontal,
                    .alignment = NodeAlignment::Center,
                    .justify = NodeJustify::SpaceBetween,
                    .paddingLeft = _sidebarHeaderPaddingX,
                    .marginBottom = _sidebarHeaderMargin
                }, [&]{
                    Node({
                        .direction = NodeDirection::Vertical
                    }, [&]{
                        Title({ .content = "Rocket", .order = TitleOrder::H3 });
                        Text({ .content = "Storybook", .size = TextSize::Small, .dimmed = true });
                    });

                    ColorSchemeToggle({
                        .onClick = [&]{
                            state.colorScheme = (state.colorScheme == ColorScheme::Light ? ColorScheme::Dark : ColorScheme::Light);
                        }
                    });
                });

                Node({
                    .direction = NodeDirection::Vertical,
                    .alignment = NodeAlignment::Stretch,
                    .marginBottom = _themePickerMargin,
                    .gapY = _sidebarGap
                }, [&]{
                    Text({
                        .content = "Theme",
                        .size = TextSize::Small,
                        .dimmed = true,
                        .nodeProps = { .paddingLeft = _sidebarHeaderPaddingX }
                    });

                    auto themeNames = std::vector<std::string>();

                    for (auto const& theme : _themes) {
                        themeNames.push_back(theme.name);
                    }

                    Select({
                        .options = themeNames,
                        .selected = state.theme,
                        .onChange = [&](std::size_t index) {
                            state.theme = index;
                        }
                    });
                });

                for (auto index = 0llu; index < _pages.size(); index++) {
                    NavLink({
                        .key = _pages[index].title,
                        .label = _pages[index].title,
                        .active = (state.page == index),
                        .onClick = [&, index]{
                            state.page = index;
                        }
                    });
                }
            });

            Node({
                .overflowY = NodeOverflow::Scroll,
                .direction = NodeDirection::Vertical,
                .alignment = NodeAlignment::Center,
                .paddingLeft = _pagePaddingX,
                .paddingTop = _pagePaddingTop,
                .paddingRight = _pagePaddingX,
                .paddingBottom = _pagePaddingBottom,
                .flex = true
            }, [&]{
                Node({
                    .direction = NodeDirection::Vertical,
                    .alignment = NodeAlignment::Stretch,
                    .width = NodeValue{ PercentValue{ 100.0f } },
                    .maxWidth = _pageMaxWidth,
                    .gapY = _pageGap
                }, [&]{
                    Context(_themes[state.theme].get(), [&]{
                        Context(_themes[state.theme].stage, [&]{
                            _pages[state.page].render();
                        });
                    });
                });
            });
        });
    });
}

} /* namespace Storybook */
