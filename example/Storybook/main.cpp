/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <filesystem>
#include <string>
#include <vector>
#include <Rocket/Rocket.hpp>
#include <App.hpp>

using namespace Rocket;

static auto const _monoFontFamily = std::string("Mono");
static auto const _monoFontPaths = std::vector<std::string>{
    "/System/Library/Fonts/Menlo.ttc",
    "C:\\Windows\\Fonts\\consola.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
};

static void _AddMonoFont() {
    for (auto const& path : _monoFontPaths) {
        if (std::filesystem::exists(path)) {
            Font::Add(_monoFontFamily, FontWeight::Normal, FontStyle::Normal, path, 0);
            return;
        }
    }
}

int main(int argc, char const* argv[]) {
    auto reconciler = Reconciler();

    _AddMonoFont();

    reconciler.setUpdateMode(ReconcilerUpdateMode::OnUpdate);
    reconciler.setUpdateFn([]{
        Storybook::App({
            .onClose = []{ App::Exit(); }
        });
    });

    auto subscription = Sub(App::OnEvent, [&](AppEvent const& event) {
        if (event.is<ReadyAppEvent>() || event.is<UpdateAppEvent>()) {
            reconciler.update();
        } else if (event.is<CloseAppEvent>()) {
            App::Exit();
        }
    });

    App::Run(argc, argv);

    return 0;
}
