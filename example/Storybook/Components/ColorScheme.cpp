/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/UI/Reconciler.hpp>
#include <Components/ColorScheme.hpp>

using namespace Rocket;

namespace Storybook {

ColorScheme UseColorScheme() {
    PROFILE

    auto const colorScheme = UseContextIf<ColorScheme>();

    return (colorScheme != nullptr ? *colorScheme : ColorScheme::Light);
}

} /* namespace Storybook */
