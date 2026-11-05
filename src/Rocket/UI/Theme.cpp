/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme/Default/Theme.hpp>
#include <Rocket/UI/Theme.hpp>

namespace Rocket {

Theme const& UseTheme() {
    PROFILE

    auto const theme = UseContextIf<Theme const>();

    return (theme != nullptr ? *theme : GetDefaultDarkTheme());
}

} /* namespace Rocket */
