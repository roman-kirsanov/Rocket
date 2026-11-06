/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstddef>
#include <format>
#include <string>
#include <tuple>
#include <vector>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Button.hpp>
#include <Rocket/UI/Drawer.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/Playground.hpp>
#include <Components/PlaygroundControl.hpp>
#include <Components/SegmentedControl.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Options.hpp>
#include <Pages/DrawerPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _DrawerPageState {
    DrawerOrigin origin = DrawerOrigin::Left;
    bool visible = false;
};

static auto const _origins = std::vector<std::tuple<DrawerOrigin, std::string>>{
    { DrawerOrigin::Left, "Left" },
    { DrawerOrigin::Top, "Top" },
    { DrawerOrigin::Right, "Right" },
    { DrawerOrigin::Bottom, "Bottom" }
};
static auto const _padding = 20.0f;
static auto const _gap = 12.0f;
static auto const _size = 280.0f;

static std::string _GetCode(_DrawerPageState const& state) {
    return std::format(
        "Drawer({{\n"
        "    .origin = DrawerOrigin::{},\n"
        "    .visible = state.visible,\n"
        "    .onClose = [&]{{ state.visible = false; }}\n"
        "}}, [&]{{\n"
        "    ...\n"
        "}});",
        GetOptionName(_origins, state.origin)
    );
}

void DrawerPage(DrawerPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_DrawerPageState>();
    auto const stage = UseStageColors();
    auto const horizontal = (state.origin == DrawerOrigin::Left || state.origin == DrawerOrigin::Right);

    Title({ .content = "Drawer" });

    Text({
        .content = "A panel against one edge of the window, over a backdrop above the rest of the document. A press on "
                   "the backdrop asks to close it; the drawer keeps no visibility of its own. It appears at once: it "
                   "does not slide yet. Its look comes from DrawerTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            Button({
                .label = "Open drawer",
                .onClick = [&]{
                    state.visible = true;
                }
            });

            Drawer({
                .origin = state.origin,
                .visible = state.visible,
                .onClose = [&]{
                    state.visible = false;
                },
                .nodeProps = {
                    .width = (horizontal ? std::optional<NodeValue>(_size) : std::nullopt),
                    .height = (horizontal ? std::nullopt : std::optional<NodeValue>(_size)),
                    .padding = _padding,
                    .gap = _gap
                }
            }, [&]{
                Node({ .display = NodeDisplay::Text, .content = "A press on the backdrop closes the drawer." });
                Button({
                    .label = "Close",
                    .onClick = [&]{
                        state.visible = false;
                    }
                });
            });
        },
        .controls = [&]{
            PlaygroundControl({ .label = "origin" }, [&]{
                SegmentedControl({
                    .options = GetOptionNames(_origins),
                    .selected = GetOptionIndex(_origins, state.origin),
                    .onChange = [&](std::size_t index) {
                        state.origin = std::get<0>(_origins[index]);
                    }
                });
            });
        },
        .code = _GetCode(state),
        .previewProps = {
            .background = ColorBrush{ stage.background }
        }
    });
}

} /* namespace Storybook */
