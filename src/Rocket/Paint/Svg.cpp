/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <format>
#include <cassert>
#include <algorithm>
#include <lunasvg.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Svg.hpp>

namespace Rocket {

void ConvertSVGToBitmap(std::string const& svg, Vec2 const& size, Vec4 const& color, std::vector<std::uint8_t>& buffer) {
    PROFILE

    auto width = std::max(1, static_cast<int>(size.width));
    auto height = std::max(1, static_cast<int>(size.height));

    if (auto document = lunasvg::Document::loadFromData(svg.data())) {
        if (color.alpha > 0.0f) {
            document->applyStyleSheet(
                std::format(
                    "svg {{ color: rgba({}, {}, {}, {}); }}",
                    static_cast<int>(color.red * 255),
                    static_cast<int>(color.green * 255),
                    static_cast<int>(color.blue * 255),
                    static_cast<float>(color.alpha)
                )
            );
        }

        auto bitmap = document->renderToBitmap(width, height);
        bitmap.convertToRGBA();

        assert(bitmap.width() == width);
        assert(bitmap.height() == height);

        buffer.clear();
        buffer.insert(
            buffer.begin(),
            bitmap.data(),
            bitmap.data() + (bitmap.stride() * bitmap.height())
        );
    }
}

} /* namespace Rocket */
