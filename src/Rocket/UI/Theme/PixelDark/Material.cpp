/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Rocket/Paint/Image.hpp>
#include <Rocket/UI/Theme/PixelDark/Image/Image.hpp>
#include <Rocket/UI/Theme/PixelDark/Image/Image@2x.hpp>
#include <Rocket/UI/Theme/PixelDark/Image/Image@3x.hpp>
#include <Rocket/UI/Theme/PixelDark/Material.hpp>

namespace Rocket {

Material const& GetPixelDarkMaterial() {
    PROFILE

    static auto const image1x = Image(MATERIAL_PIXEL_DARK_SIZE, MATERIAL_PIXEL_DARK_DATA);
    static auto const image2x = Image(MATERIAL_PIXEL_DARK_2X_SIZE, MATERIAL_PIXEL_DARK_2X_DATA);
    static auto const image3x = Image(MATERIAL_PIXEL_DARK_3X_SIZE, MATERIAL_PIXEL_DARK_3X_DATA);
    static auto const material = Material(image1x, image2x, image3x);

    return material;
}

} /* namespace Rocket */
