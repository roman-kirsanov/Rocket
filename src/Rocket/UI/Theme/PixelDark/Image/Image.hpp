#pragma once

#include <cstdlib>
#include <cinttypes>
#include <Rocket/Math/Vec2.hpp>

namespace Rocket {

/** Embedded Image image pixels, RGBA8, row-major. */
extern std::uint8_t MATERIAL_PIXEL_DARK_DATA[];
/** Size in pixels of MATERIAL_PIXEL_DARK_DATA. */
extern Vec2 MATERIAL_PIXEL_DARK_SIZE;

} /* namespace Rocket */
