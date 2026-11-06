/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/UI/Button.hpp>
#include <Rocket/UI/Modal.hpp>
#include <Rocket/UI/Node.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Components/Playground.hpp>
#include <Components/Stage.hpp>
#include <Components/Text.hpp>
#include <Components/Title.hpp>
#include <Pages/ModalPage.hpp>

using namespace Rocket;

namespace Storybook {

struct _ModalPageState {
    bool open = false;
};

static auto const _code = std::string(
    "if (state.open) {\n"
    "    Modal({ .nodeProps = { .padding = 20.0f, .gap = 16.0f } }, [&]{\n"
    "        Node({ .display = NodeDisplay::Text, .content = \"A modal holds whatever it is given.\" });\n"
    "        Button({ .label = \"Close\", .onClick = [&]{ state.open = false; } });\n"
    "    });\n"
    "}"
);
static auto const _padding = 20.0f;
static auto const _gap = 16.0f;

void ModalPage(ModalPageProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_ModalPageState>();
    auto const stage = UseStageColors();

    Title({ .content = "Modal" });

    Text({
        .content = "A backdrop over the whole window, above the rest of the document, with a panel in its center that "
                   "holds its children. The backdrop takes the presses meant for what is under it; closing the modal is "
                   "up to its content. Its look comes from ModalTheme.",
        .dimmed = true
    });

    Playground({
        .preview = [&]{
            Button({
                .label = "Open modal",
                .onClick = [&]{
                    state.open = true;
                }
            });

            if (state.open) {
                Modal({ .nodeProps = { .padding = _padding, .gap = _gap } }, [&]{
                    Node({ .display = NodeDisplay::Text, .content = "A modal holds whatever it is given." });
                    Button({
                        .label = "Close",
                        .onClick = [&]{
                            state.open = false;
                        }
                    });
                });
            }
        },
        .code = _code,
        .previewProps = {
            .background = ColorBrush{ stage.background }
        }
    });
}

} /* namespace Storybook */
