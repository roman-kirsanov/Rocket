/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <tuple>
#include <vector>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/UI/Material.hpp>

namespace Rocket {

enum class _MaterialRoundPart {
    Outside,
    Outline,
    Bevel,
    Fill
};

struct _MaterialArrowSlices {
    Vec4 normal;
    Vec4 lighter;
    Vec4 darker;
};

struct _MaterialVariantSlices {
    Vec4 normal;
    Vec4 lighter;
    Vec4 darker;
    Vec4 nPatch;
    int border;
    bool normalEmpty = false;
};

using _MaterialComposites = std::map<
    std::tuple<
        std::array<_MaterialVariantSlices const*, 4>, float
    >,
    std::unique_ptr<Image>
>;

struct _MaterialRoundPixel {
    _MaterialRoundPart part = _MaterialRoundPart::Outside;
    float blend = 0.0f;
    float strength = 1.0f;
    float opacity = 1.0f;
};

using _MaterialRounds = std::map<
    std::tuple<MaterialKind, MaterialIntent, int, int>,
    std::unique_ptr<Image>
>;

struct _Material {
    std::array<Image const*, 3> images;
    _MaterialComposites composites;
    _MaterialRounds rounds;
    std::vector<std::uint8_t> pixels;
};

static auto constexpr _materialCellSize = 16.0f;
static auto constexpr _materialStateCount = 3llu;
static auto constexpr _materialArrowOverlap = 2.0f;
static auto constexpr _materialPixelSize = 2.0f;
static auto constexpr _materialRoundBlend = 0.5f;
static auto constexpr _materialRoundSamples = 8;
static auto constexpr _materialRoundOutlineTop = Vec2{ 8.0f, 0.0f };
static auto constexpr _materialRoundOutlineSide = Vec2{ 0.0f, 8.0f };
static auto constexpr _materialRoundBevelTop = Vec2{ 8.0f, 2.0f };
static auto constexpr _materialRoundBevelBottom = Vec2{ 8.0f, 12.0f };
static auto constexpr _materialRoundFill = Vec2{ 8.0f, 8.0f };

static auto const _materialArrows = std::map<std::tuple<MaterialArrowDirection, MaterialIntent>, _MaterialArrowSlices>{
    { { MaterialArrowDirection::Up, MaterialIntent::Default }, { { 0.0f, 568.0f, 16.0f, 8.0f }, { 16.0f, 568.0f, 16.0f, 8.0f }, { 32.0f, 568.0f, 16.0f, 8.0f } } },
    { { MaterialArrowDirection::Down, MaterialIntent::Default }, { { 0.0f, 576.0f, 16.0f, 8.0f }, { 16.0f, 576.0f, 16.0f, 8.0f }, { 32.0f, 576.0f, 16.0f, 8.0f } } },
    { { MaterialArrowDirection::Right, MaterialIntent::Default }, { { 0.0f, 592.0f, 8.0f, 16.0f }, { 16.0f, 592.0f, 8.0f, 16.0f }, { 32.0f, 592.0f, 8.0f, 16.0f } } },
    { { MaterialArrowDirection::Left, MaterialIntent::Default }, { { 40.0f, 608.0f, 8.0f, 16.0f }, { 24.0f, 608.0f, 8.0f, 16.0f }, { 8.0f, 608.0f, 8.0f, 16.0f } } }
};

static auto const _materialVariants = std::map<std::tuple<MaterialKind, MaterialIntent, MaterialRoundness>, _MaterialVariantSlices>{
    { { MaterialKind::Default, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 0.0f, 16.0f, 16.0f }, { 16.0f, 0.0f, 16.0f, 16.0f }, { 32.0f, 0.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 0.0f, 16.0f, 16.0f }, { 64.0f, 0.0f, 16.0f, 16.0f }, { 80.0f, 0.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 0.0f, 16.0f, 16.0f }, { 112.0f, 0.0f, 16.0f, 16.0f }, { 128.0f, 0.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Bordered, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 16.0f, 16.0f, 16.0f }, { 16.0f, 16.0f, 16.0f, 16.0f }, { 32.0f, 16.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 16.0f, 16.0f, 16.0f }, { 64.0f, 16.0f, 16.0f, 16.0f }, { 80.0f, 16.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 16.0f, 16.0f, 16.0f }, { 112.0f, 16.0f, 16.0f, 16.0f }, { 128.0f, 16.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Lowered, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 32.0f, 16.0f, 16.0f }, { 16.0f, 32.0f, 16.0f, 16.0f }, { 32.0f, 32.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 32.0f, 16.0f, 16.0f }, { 64.0f, 32.0f, 16.0f, 16.0f }, { 80.0f, 32.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 32.0f, 16.0f, 16.0f }, { 112.0f, 32.0f, 16.0f, 16.0f }, { 128.0f, 32.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 48.0f, 16.0f, 16.0f }, { 16.0f, 48.0f, 16.0f, 16.0f }, { 32.0f, 48.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 48.0f, 16.0f, 16.0f }, { 64.0f, 48.0f, 16.0f, 16.0f }, { 80.0f, 48.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 48.0f, 16.0f, 16.0f }, { 112.0f, 48.0f, 16.0f, 16.0f }, { 128.0f, 48.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Outlined, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 64.0f, 16.0f, 16.0f }, { 16.0f, 64.0f, 16.0f, 16.0f }, { 32.0f, 64.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 64.0f, 16.0f, 16.0f }, { 64.0f, 64.0f, 16.0f, 16.0f }, { 80.0f, 64.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 64.0f, 16.0f, 16.0f }, { 112.0f, 64.0f, 16.0f, 16.0f }, { 128.0f, 64.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Flat, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 80.0f, 16.0f, 16.0f }, { 16.0f, 80.0f, 16.0f, 16.0f }, { 32.0f, 80.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 80.0f, 16.0f, 16.0f }, { 64.0f, 80.0f, 16.0f, 16.0f }, { 80.0f, 80.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 80.0f, 16.0f, 16.0f }, { 112.0f, 80.0f, 16.0f, 16.0f }, { 128.0f, 80.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Transparent, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 80.0f, 16.0f, 16.0f }, { 16.0f, 80.0f, 16.0f, 16.0f }, { 32.0f, 80.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 80.0f, 16.0f, 16.0f }, { 64.0f, 80.0f, 16.0f, 16.0f }, { 80.0f, 80.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 80.0f, 16.0f, 16.0f }, { 112.0f, 80.0f, 16.0f, 16.0f }, { 128.0f, 80.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 96.0f, 16.0f, 16.0f }, { 16.0f, 96.0f, 16.0f, 16.0f }, { 32.0f, 96.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 96.0f, 16.0f, 16.0f }, { 64.0f, 96.0f, 16.0f, 16.0f }, { 80.0f, 96.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 96.0f, 16.0f, 16.0f }, { 112.0f, 96.0f, 16.0f, 16.0f }, { 128.0f, 96.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Default, MaterialRoundness::None }, { { 0.0f, 64.0f, 16.0f, 16.0f }, { 16.0f, 96.0f, 16.0f, 16.0f }, { 32.0f, 96.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Default, MaterialRoundness::Small }, { { 48.0f, 64.0f, 16.0f, 16.0f }, { 64.0f, 96.0f, 16.0f, 16.0f }, { 80.0f, 96.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Default, MaterialRoundness::Large }, { { 96.0f, 64.0f, 16.0f, 16.0f }, { 112.0f, 96.0f, 16.0f, 16.0f }, { 128.0f, 96.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Default, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 112.0f, 16.0f, 16.0f }, { 16.0f, 112.0f, 16.0f, 16.0f }, { 32.0f, 112.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 112.0f, 16.0f, 16.0f }, { 64.0f, 112.0f, 16.0f, 16.0f }, { 80.0f, 112.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 112.0f, 16.0f, 16.0f }, { 112.0f, 112.0f, 16.0f, 16.0f }, { 128.0f, 112.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Bordered, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 128.0f, 16.0f, 16.0f }, { 16.0f, 128.0f, 16.0f, 16.0f }, { 32.0f, 128.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 128.0f, 16.0f, 16.0f }, { 64.0f, 128.0f, 16.0f, 16.0f }, { 80.0f, 128.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 128.0f, 16.0f, 16.0f }, { 112.0f, 128.0f, 16.0f, 16.0f }, { 128.0f, 128.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Lowered, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 144.0f, 16.0f, 16.0f }, { 16.0f, 144.0f, 16.0f, 16.0f }, { 32.0f, 144.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 144.0f, 16.0f, 16.0f }, { 64.0f, 144.0f, 16.0f, 16.0f }, { 80.0f, 144.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 144.0f, 16.0f, 16.0f }, { 112.0f, 144.0f, 16.0f, 16.0f }, { 128.0f, 144.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 160.0f, 16.0f, 16.0f }, { 16.0f, 160.0f, 16.0f, 16.0f }, { 32.0f, 160.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 160.0f, 16.0f, 16.0f }, { 64.0f, 160.0f, 16.0f, 16.0f }, { 80.0f, 160.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 160.0f, 16.0f, 16.0f }, { 112.0f, 160.0f, 16.0f, 16.0f }, { 128.0f, 160.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Outlined, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 176.0f, 16.0f, 16.0f }, { 16.0f, 176.0f, 16.0f, 16.0f }, { 32.0f, 176.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 176.0f, 16.0f, 16.0f }, { 64.0f, 176.0f, 16.0f, 16.0f }, { 80.0f, 176.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 176.0f, 16.0f, 16.0f }, { 112.0f, 176.0f, 16.0f, 16.0f }, { 128.0f, 176.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Flat, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 192.0f, 16.0f, 16.0f }, { 16.0f, 192.0f, 16.0f, 16.0f }, { 32.0f, 192.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 192.0f, 16.0f, 16.0f }, { 64.0f, 192.0f, 16.0f, 16.0f }, { 80.0f, 192.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 192.0f, 16.0f, 16.0f }, { 112.0f, 192.0f, 16.0f, 16.0f }, { 128.0f, 192.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Transparent, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 192.0f, 16.0f, 16.0f }, { 16.0f, 192.0f, 16.0f, 16.0f }, { 32.0f, 192.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 192.0f, 16.0f, 16.0f }, { 64.0f, 192.0f, 16.0f, 16.0f }, { 80.0f, 192.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 192.0f, 16.0f, 16.0f }, { 112.0f, 192.0f, 16.0f, 16.0f }, { 128.0f, 192.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 208.0f, 16.0f, 16.0f }, { 16.0f, 208.0f, 16.0f, 16.0f }, { 32.0f, 208.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 208.0f, 16.0f, 16.0f }, { 64.0f, 208.0f, 16.0f, 16.0f }, { 80.0f, 208.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 208.0f, 16.0f, 16.0f }, { 112.0f, 208.0f, 16.0f, 16.0f }, { 128.0f, 208.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Primary, MaterialRoundness::None }, { { 0.0f, 176.0f, 16.0f, 16.0f }, { 16.0f, 208.0f, 16.0f, 16.0f }, { 32.0f, 208.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Primary, MaterialRoundness::Small }, { { 48.0f, 176.0f, 16.0f, 16.0f }, { 64.0f, 208.0f, 16.0f, 16.0f }, { 80.0f, 208.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Primary, MaterialRoundness::Large }, { { 96.0f, 176.0f, 16.0f, 16.0f }, { 112.0f, 208.0f, 16.0f, 16.0f }, { 128.0f, 208.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Default, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 224.0f, 16.0f, 16.0f }, { 16.0f, 224.0f, 16.0f, 16.0f }, { 32.0f, 224.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 224.0f, 16.0f, 16.0f }, { 64.0f, 224.0f, 16.0f, 16.0f }, { 80.0f, 224.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 224.0f, 16.0f, 16.0f }, { 112.0f, 224.0f, 16.0f, 16.0f }, { 128.0f, 224.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Bordered, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 240.0f, 16.0f, 16.0f }, { 16.0f, 240.0f, 16.0f, 16.0f }, { 32.0f, 240.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 240.0f, 16.0f, 16.0f }, { 64.0f, 240.0f, 16.0f, 16.0f }, { 80.0f, 240.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 240.0f, 16.0f, 16.0f }, { 112.0f, 240.0f, 16.0f, 16.0f }, { 128.0f, 240.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Lowered, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 256.0f, 16.0f, 16.0f }, { 16.0f, 256.0f, 16.0f, 16.0f }, { 32.0f, 256.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 256.0f, 16.0f, 16.0f }, { 64.0f, 256.0f, 16.0f, 16.0f }, { 80.0f, 256.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 256.0f, 16.0f, 16.0f }, { 112.0f, 256.0f, 16.0f, 16.0f }, { 128.0f, 256.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 272.0f, 16.0f, 16.0f }, { 16.0f, 272.0f, 16.0f, 16.0f }, { 32.0f, 272.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 272.0f, 16.0f, 16.0f }, { 64.0f, 272.0f, 16.0f, 16.0f }, { 80.0f, 272.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 272.0f, 16.0f, 16.0f }, { 112.0f, 272.0f, 16.0f, 16.0f }, { 128.0f, 272.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Outlined, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 288.0f, 16.0f, 16.0f }, { 16.0f, 288.0f, 16.0f, 16.0f }, { 32.0f, 288.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 288.0f, 16.0f, 16.0f }, { 64.0f, 288.0f, 16.0f, 16.0f }, { 80.0f, 288.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 288.0f, 16.0f, 16.0f }, { 112.0f, 288.0f, 16.0f, 16.0f }, { 128.0f, 288.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Flat, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 304.0f, 16.0f, 16.0f }, { 16.0f, 304.0f, 16.0f, 16.0f }, { 32.0f, 304.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 304.0f, 16.0f, 16.0f }, { 64.0f, 304.0f, 16.0f, 16.0f }, { 80.0f, 304.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 304.0f, 16.0f, 16.0f }, { 112.0f, 304.0f, 16.0f, 16.0f }, { 128.0f, 304.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Transparent, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 304.0f, 16.0f, 16.0f }, { 16.0f, 304.0f, 16.0f, 16.0f }, { 32.0f, 304.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 304.0f, 16.0f, 16.0f }, { 64.0f, 304.0f, 16.0f, 16.0f }, { 80.0f, 304.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 304.0f, 16.0f, 16.0f }, { 112.0f, 304.0f, 16.0f, 16.0f }, { 128.0f, 304.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 320.0f, 16.0f, 16.0f }, { 16.0f, 320.0f, 16.0f, 16.0f }, { 32.0f, 320.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 320.0f, 16.0f, 16.0f }, { 64.0f, 320.0f, 16.0f, 16.0f }, { 80.0f, 320.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 320.0f, 16.0f, 16.0f }, { 112.0f, 320.0f, 16.0f, 16.0f }, { 128.0f, 320.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Success, MaterialRoundness::None }, { { 0.0f, 288.0f, 16.0f, 16.0f }, { 16.0f, 320.0f, 16.0f, 16.0f }, { 32.0f, 320.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Success, MaterialRoundness::Small }, { { 48.0f, 288.0f, 16.0f, 16.0f }, { 64.0f, 320.0f, 16.0f, 16.0f }, { 80.0f, 320.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Success, MaterialRoundness::Large }, { { 96.0f, 288.0f, 16.0f, 16.0f }, { 112.0f, 320.0f, 16.0f, 16.0f }, { 128.0f, 320.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Default, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 336.0f, 16.0f, 16.0f }, { 16.0f, 336.0f, 16.0f, 16.0f }, { 32.0f, 336.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 336.0f, 16.0f, 16.0f }, { 64.0f, 336.0f, 16.0f, 16.0f }, { 80.0f, 336.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 336.0f, 16.0f, 16.0f }, { 112.0f, 336.0f, 16.0f, 16.0f }, { 128.0f, 336.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Bordered, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 352.0f, 16.0f, 16.0f }, { 16.0f, 352.0f, 16.0f, 16.0f }, { 32.0f, 352.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 352.0f, 16.0f, 16.0f }, { 64.0f, 352.0f, 16.0f, 16.0f }, { 80.0f, 352.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 352.0f, 16.0f, 16.0f }, { 112.0f, 352.0f, 16.0f, 16.0f }, { 128.0f, 352.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Lowered, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 368.0f, 16.0f, 16.0f }, { 16.0f, 368.0f, 16.0f, 16.0f }, { 32.0f, 368.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 368.0f, 16.0f, 16.0f }, { 64.0f, 368.0f, 16.0f, 16.0f }, { 80.0f, 368.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 368.0f, 16.0f, 16.0f }, { 112.0f, 368.0f, 16.0f, 16.0f }, { 128.0f, 368.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 384.0f, 16.0f, 16.0f }, { 16.0f, 384.0f, 16.0f, 16.0f }, { 32.0f, 384.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 384.0f, 16.0f, 16.0f }, { 64.0f, 384.0f, 16.0f, 16.0f }, { 80.0f, 384.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 384.0f, 16.0f, 16.0f }, { 112.0f, 384.0f, 16.0f, 16.0f }, { 128.0f, 384.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Outlined, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 400.0f, 16.0f, 16.0f }, { 16.0f, 400.0f, 16.0f, 16.0f }, { 32.0f, 400.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 400.0f, 16.0f, 16.0f }, { 64.0f, 400.0f, 16.0f, 16.0f }, { 80.0f, 400.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 400.0f, 16.0f, 16.0f }, { 112.0f, 400.0f, 16.0f, 16.0f }, { 128.0f, 400.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Flat, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 416.0f, 16.0f, 16.0f }, { 16.0f, 416.0f, 16.0f, 16.0f }, { 32.0f, 416.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 416.0f, 16.0f, 16.0f }, { 64.0f, 416.0f, 16.0f, 16.0f }, { 80.0f, 416.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 416.0f, 16.0f, 16.0f }, { 112.0f, 416.0f, 16.0f, 16.0f }, { 128.0f, 416.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Transparent, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 416.0f, 16.0f, 16.0f }, { 16.0f, 416.0f, 16.0f, 16.0f }, { 32.0f, 416.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 416.0f, 16.0f, 16.0f }, { 64.0f, 416.0f, 16.0f, 16.0f }, { 80.0f, 416.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 416.0f, 16.0f, 16.0f }, { 112.0f, 416.0f, 16.0f, 16.0f }, { 128.0f, 416.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 432.0f, 16.0f, 16.0f }, { 16.0f, 432.0f, 16.0f, 16.0f }, { 32.0f, 432.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 432.0f, 16.0f, 16.0f }, { 64.0f, 432.0f, 16.0f, 16.0f }, { 80.0f, 432.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 432.0f, 16.0f, 16.0f }, { 112.0f, 432.0f, 16.0f, 16.0f }, { 128.0f, 432.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Warning, MaterialRoundness::None }, { { 0.0f, 400.0f, 16.0f, 16.0f }, { 16.0f, 432.0f, 16.0f, 16.0f }, { 32.0f, 432.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Warning, MaterialRoundness::Small }, { { 48.0f, 400.0f, 16.0f, 16.0f }, { 64.0f, 432.0f, 16.0f, 16.0f }, { 80.0f, 432.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Warning, MaterialRoundness::Large }, { { 96.0f, 400.0f, 16.0f, 16.0f }, { 112.0f, 432.0f, 16.0f, 16.0f }, { 128.0f, 432.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Default, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 448.0f, 16.0f, 16.0f }, { 16.0f, 448.0f, 16.0f, 16.0f }, { 32.0f, 448.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 448.0f, 16.0f, 16.0f }, { 64.0f, 448.0f, 16.0f, 16.0f }, { 80.0f, 448.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Default, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 448.0f, 16.0f, 16.0f }, { 112.0f, 448.0f, 16.0f, 16.0f }, { 128.0f, 448.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Bordered, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 464.0f, 16.0f, 16.0f }, { 16.0f, 464.0f, 16.0f, 16.0f }, { 32.0f, 464.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 464.0f, 16.0f, 16.0f }, { 64.0f, 464.0f, 16.0f, 16.0f }, { 80.0f, 464.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Bordered, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 464.0f, 16.0f, 16.0f }, { 112.0f, 464.0f, 16.0f, 16.0f }, { 128.0f, 464.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Lowered, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 480.0f, 16.0f, 16.0f }, { 16.0f, 480.0f, 16.0f, 16.0f }, { 32.0f, 480.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 480.0f, 16.0f, 16.0f }, { 64.0f, 480.0f, 16.0f, 16.0f }, { 80.0f, 480.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Lowered, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 480.0f, 16.0f, 16.0f }, { 112.0f, 480.0f, 16.0f, 16.0f }, { 128.0f, 480.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 496.0f, 16.0f, 16.0f }, { 16.0f, 496.0f, 16.0f, 16.0f }, { 32.0f, 496.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 496.0f, 16.0f, 16.0f }, { 64.0f, 496.0f, 16.0f, 16.0f }, { 80.0f, 496.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6 } },
    { { MaterialKind::Raised, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 496.0f, 16.0f, 16.0f }, { 112.0f, 496.0f, 16.0f, 16.0f }, { 128.0f, 496.0f, 16.0f, 16.0f }, { 6.0f, 6.0f, 6.0f, 6.0f }, 6 } },
    { { MaterialKind::Outlined, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 512.0f, 16.0f, 16.0f }, { 16.0f, 512.0f, 16.0f, 16.0f }, { 32.0f, 512.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 512.0f, 16.0f, 16.0f }, { 64.0f, 512.0f, 16.0f, 16.0f }, { 80.0f, 512.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Outlined, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 512.0f, 16.0f, 16.0f }, { 112.0f, 512.0f, 16.0f, 16.0f }, { 128.0f, 512.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Flat, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 528.0f, 16.0f, 16.0f }, { 16.0f, 528.0f, 16.0f, 16.0f }, { 32.0f, 528.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 528.0f, 16.0f, 16.0f }, { 64.0f, 528.0f, 16.0f, 16.0f }, { 80.0f, 528.0f, 16.0f, 16.0f }, { 2.0f, 2.0f, 2.0f, 2.0f }, 2 } },
    { { MaterialKind::Flat, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 528.0f, 16.0f, 16.0f }, { 112.0f, 528.0f, 16.0f, 16.0f }, { 128.0f, 528.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::Transparent, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 528.0f, 16.0f, 16.0f }, { 16.0f, 528.0f, 16.0f, 16.0f }, { 32.0f, 528.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 528.0f, 16.0f, 16.0f }, { 64.0f, 528.0f, 16.0f, 16.0f }, { 80.0f, 528.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::Transparent, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 528.0f, 16.0f, 16.0f }, { 112.0f, 528.0f, 16.0f, 16.0f }, { 128.0f, 528.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 6, true } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 544.0f, 16.0f, 16.0f }, { 16.0f, 544.0f, 16.0f, 16.0f }, { 32.0f, 544.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 544.0f, 16.0f, 16.0f }, { 64.0f, 544.0f, 16.0f, 16.0f }, { 80.0f, 544.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::FlatOutlined, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 544.0f, 16.0f, 16.0f }, { 112.0f, 544.0f, 16.0f, 16.0f }, { 128.0f, 544.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Danger, MaterialRoundness::None }, { { 0.0f, 512.0f, 16.0f, 16.0f }, { 16.0f, 544.0f, 16.0f, 16.0f }, { 32.0f, 544.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Danger, MaterialRoundness::Small }, { { 48.0f, 512.0f, 16.0f, 16.0f }, { 64.0f, 544.0f, 16.0f, 16.0f }, { 80.0f, 544.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } },
    { { MaterialKind::TransparentOutlined, MaterialIntent::Danger, MaterialRoundness::Large }, { { 96.0f, 512.0f, 16.0f, 16.0f }, { 112.0f, 544.0f, 16.0f, 16.0f }, { 128.0f, 544.0f, 16.0f, 16.0f }, { 4.0f, 4.0f, 4.0f, 4.0f }, 4 } }
};

static std::tuple<Image const*, float> _GetImage(std::array<Image const*, 3> const& images, float scale) {
    if (scale >= 2.5f) {
        return { images[2], 3.0f };
    } else if (scale >= 1.5f) {
        return { images[1], 2.0f };
    }

    return { images[0], 1.0f };
}

static Image const* _GetComposite(_MaterialComposites& composites, Image const& sheet, float factor, std::array<_MaterialVariantSlices const*, 4> const& corners) {
    auto& composite = composites[{ corners, factor }];

    if (composite != nullptr) {
        return composite.get();
    }

    auto const cell = (_materialCellSize * factor);
    auto const half = (cell / 2.0f);

    composite = std::make_unique<Image>(Vec2{ (cell * static_cast<float>(_materialStateCount)), cell });

    for (auto state = 0llu; state < _materialStateCount; state++) {
        for (auto corner = 0llu; corner < corners.size(); corner++) {
            auto const slice = std::array<Vec4, _materialStateCount>{ corners[corner]->normal, corners[corner]->lighter, corners[corner]->darker }[state] * factor;
            auto const quadrant = Vec2{ (static_cast<float>(corner % 2) * half), (static_cast<float>(corner / 2) * half) };

            composite->copyFrom(
                sheet,
                Vec4{ (slice.x + quadrant.x), (slice.y + quadrant.y), half, half },
                Vec2{ ((static_cast<float>(state) * cell) + quadrant.x), quadrant.y }
            );
        }
    }

    return composite.get();
}

static Image const* _GetRound(_Material& material, MaterialKind kind, MaterialIntent intent, _MaterialVariantSlices const& variant, int width, int height) {
    auto& image = material.rounds[{ kind, intent, width, height }];

    if (image != nullptr) {
        return image.get();
    }

    if (material.pixels.empty()) {
        material.images[0]->getData(material.pixels);
    }

    auto const sheetWidth = static_cast<int>(material.images[0]->getSize().width);
    auto const radius = (static_cast<float>(std::min(width, height)) / 2.0f);
    auto const middle = (static_cast<float>(height) / 2.0f);
    auto const count = static_cast<std::size_t>(width * height);

    auto const getDistanceSquared = [&](float x, float y) {
        auto const nearestX = std::clamp(x, radius, (static_cast<float>(width) - radius));
        auto const nearestY = std::clamp(y, radius, (static_cast<float>(height) - radius));

        return (((x - nearestX) * (x - nearestX)) + ((y - nearestY) * (y - nearestY)));
    };

    auto inside = std::array<std::vector<bool>, 2>{ std::vector<bool>(count), std::vector<bool>(count) };

    for (auto ring = 0llu; ring < inside.size(); ring++) {
        /* Shrunk by a tenth, so a pixel only grazing the circle is left out, as in the cells drawn by hand. */
        auto const limit = (radius - static_cast<float>(ring) - 0.1f);

        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                inside[ring][static_cast<std::size_t>((y * width) + x)] = (limit > 0.0f && getDistanceSquared((static_cast<float>(x) + 0.5f), (static_cast<float>(y) + 0.5f)) <= (limit * limit));
            }
        }
    }

    auto const isInside = [&](int x, int y, std::size_t ring) {
        return (x >= 0 && y >= 0 && x < width && y < height && inside[ring][static_cast<std::size_t>((y * width) + x)]);
    };

    auto const isEdge = [&](int x, int y, std::size_t ring) {
        return (
            isInside(x, y, ring) &&
            (
                (isInside((x - 1), y, ring) == false) ||
                (isInside((x + 1), y, ring) == false) ||
                (isInside(x, (y - 1), ring) == false) ||
                (isInside(x, (y + 1), ring) == false)
            )
        );
    };

    auto const getCoverage = [&](int x, int y, std::size_t ring) {
        auto const limit = (radius - static_cast<float>(ring));
        auto covered = 0;

        for (auto row = 0; row < _materialRoundSamples; row++) {
            for (auto column = 0; column < _materialRoundSamples; column++) {
                auto const sampleX = (static_cast<float>(x) + ((static_cast<float>(column) + 0.5f) / static_cast<float>(_materialRoundSamples)));
                auto const sampleY = (static_cast<float>(y) + ((static_cast<float>(row) + 0.5f) / static_cast<float>(_materialRoundSamples)));

                covered += (getDistanceSquared(sampleX, sampleY) <= (limit * limit) ? 1 : 0);
            }
        }

        return (static_cast<float>(covered) / static_cast<float>(_materialRoundSamples * _materialRoundSamples));
    };

    auto shape = std::vector<_MaterialRoundPixel>(count);

    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            if (isInside(x, y, 0) == false) {
                continue;
            }

            auto const offset = (static_cast<float>(y) + 0.5f - middle);
            auto const isShaded = (kind == MaterialKind::Raised && offset > 0.0f);
            auto const isOuterEdge = isEdge(x, y, 0);
            auto const isInnerEdge = isEdge(x, y, 1);
            auto const isOutline = (kind == MaterialKind::Lowered ? (isInside(x, y, 1) == false) : isOuterEdge);
            auto const isBevel = (isInnerEdge || (isShaded && (isInside(x, y, 1) == false)));

            shape[static_cast<std::size_t>((y * width) + x)] = _MaterialRoundPixel{
                .part = (
                    isOutline ? _MaterialRoundPart::Outline :
                    isBevel ? _MaterialRoundPart::Bevel :
                              _MaterialRoundPart::Fill
                ),
                .blend = std::clamp((((offset / (middle * _materialRoundBlend)) + 1.0f) / 2.0f), 0.0f, 1.0f),
                .strength = ((isOuterEdge == false && kind == MaterialKind::Raised && isShaded == false && isInnerEdge) ? getCoverage(x, y, 1) : 1.0f),
                .opacity = (isOuterEdge ? getCoverage(x, y, 0) : 1.0f)
            };
        }
    }

    auto const mix = [](std::array<std::uint8_t, 4> const& from, std::array<std::uint8_t, 4> const& to, float amount) {
        auto color = std::array<std::uint8_t, 4>();

        for (auto channel = 0llu; channel < color.size(); channel++) {
            color[channel] = static_cast<std::uint8_t>(std::lround(static_cast<float>(from[channel]) + ((static_cast<float>(to[channel]) - static_cast<float>(from[channel])) * amount)));
        }

        return color;
    };

    auto const stride = (width * static_cast<int>(_materialStateCount));
    auto data = std::vector<std::uint8_t>((count * _materialStateCount * 4llu), 0);

    for (auto state = 0llu; state < _materialStateCount; state++) {
        auto const cell = std::array<Vec4, _materialStateCount>{ variant.normal, variant.lighter, variant.darker }[state];

        auto const sample = [&](Vec2 const& spot) {
            auto const index = static_cast<std::size_t>((((static_cast<int>(cell.y + spot.y) * sheetWidth) + static_cast<int>(cell.x + spot.x))) * 4);

            return std::array<std::uint8_t, 4>{ material.pixels[index], material.pixels[index + 1], material.pixels[index + 2], material.pixels[index + 3] };
        };

        auto const outlineTop = sample(_materialRoundOutlineTop);
        auto const outlineSide = sample(_materialRoundOutlineSide);
        auto const bevelTop = sample(_materialRoundBevelTop);
        auto const bevelBottom = sample(_materialRoundBevelBottom);
        auto const fill = sample(_materialRoundFill);

        for (auto y = 0; y < height; y++) {
            for (auto x = 0; x < width; x++) {
                auto const& pixel = shape[static_cast<std::size_t>((y * width) + x)];

                if (pixel.part == _MaterialRoundPart::Outside) {
                    continue;
                }

                auto color = mix(fill, (
                    pixel.part == _MaterialRoundPart::Outline ? mix(outlineTop, outlineSide, pixel.blend) :
                    pixel.part == _MaterialRoundPart::Bevel ? mix(bevelTop, bevelBottom, pixel.blend) :
                                                               fill
                ), pixel.strength);

                color[3] = static_cast<std::uint8_t>(std::lround(static_cast<float>(color[3]) * pixel.opacity));

                std::ranges::copy(color, (data.begin() + (((y * stride) + (static_cast<int>(state) * width) + x) * 4)));
            }
        }
    }

    image = std::make_unique<Image>(Vec2{ static_cast<float>(stride), static_cast<float>(height) }, data.data());

    return image.get();
}

static MaterialCapsuleVariant _GetRoundVariant(_Material& material, MaterialKind kind, MaterialIntent intent, MaterialBrightness brightness, float width, float height, float scale) {
    auto const it = _materialVariants.find({ kind, intent, MaterialRoundness::None });

    if (it == _materialVariants.end()) {
        return {};
    }

    auto const factor = std::get<1>(_GetImage(material.images, scale));
    auto const unit = (scale > 0.0f ? (factor / scale) : 1.0f);
    auto const pixelsWide = std::max(1, static_cast<int>(std::round(width / (_materialPixelSize * unit))));
    auto const pixelsHigh = std::max(1, static_cast<int>(std::round(height / (_materialPixelSize * unit))));
    auto const image = _GetRound(material, kind, intent, it->second, pixelsWide, pixelsHigh);
    auto const state = (
        brightness == MaterialBrightness::Lighter ? 1.0f :
        brightness == MaterialBrightness::Darker ? 2.0f :
                                                   0.0f
    );

    return MaterialCapsuleVariant{
        .brush = ImageBrush{
            .image = ((brightness == MaterialBrightness::Normal && it->second.normalEmpty) ? nullptr : image),
            .slice = Vec4{ (state * static_cast<float>(pixelsWide)), 0.0f, static_cast<float>(pixelsWide), static_cast<float>(pixelsHigh) },
            .filterMag = ImageFilter::Nearest,
            .filterMin = ImageFilter::Nearest,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        },
        .border = (static_cast<float>(it->second.border) * unit),
        .size = Vec2{ (static_cast<float>(pixelsWide) * _materialPixelSize * unit), (static_cast<float>(pixelsHigh) * _materialPixelSize * unit) }
    };
}

Material::~Material() {
    PROFILE

    delete _impl;
}

Material::Material(Image const& image1x, Image const& image2x, Image const& image3x)
    : _impl(new _Material{
        .images = { &image1x, &image2x, &image3x }
    })
{
    PROFILE
}

bool MaterialVariantOptions::operator==(MaterialVariantOptions const& other) const {
    PROFILE

    return (
        (kind == other.kind) &&
        (intent == other.intent) &&
        (brightness == other.brightness) &&
        (roundness == other.roundness) &&
        (topLeftRoundness == other.topLeftRoundness) &&
        (topRightRoundness == other.topRightRoundness) &&
        (bottomLeftRoundness == other.bottomLeftRoundness) &&
        (bottomRightRoundness == other.bottomRightRoundness) &&
        (scale == other.scale) &&
        (noLeftBorder == other.noLeftBorder) &&
        (noTopBorder == other.noTopBorder) &&
        (noRightBorder == other.noRightBorder) &&
        (noBottomBorder == other.noBottomBorder)
    );
}

bool MaterialVariantOptions::operator!=(MaterialVariantOptions const& other) const {
    PROFILE

    return (operator==(other) == false);
}

MaterialVariant Material::getVariant(MaterialVariantOptions const& options) const {
    PROFILE

    auto const kind = options.kind.value_or(MaterialKind::Default);
    auto const intent = options.intent.value_or(MaterialIntent::Default);
    auto const roundness = options.roundness.value_or(MaterialRoundness::None);
    auto const scale = options.scale.value_or(1.0f);
    auto const noLeftBorder = options.noLeftBorder.value_or(false);
    auto const noTopBorder = options.noTopBorder.value_or(false);
    auto const noRightBorder = options.noRightBorder.value_or(false);
    auto const noBottomBorder = options.noBottomBorder.value_or(false);
    auto const cornerRoundness = std::array<MaterialRoundness, 4>{
        options.topLeftRoundness.value_or(roundness),
        options.topRightRoundness.value_or(roundness),
        options.bottomLeftRoundness.value_or(roundness),
        options.bottomRightRoundness.value_or(roundness)
    };

    auto const [sheet, factor] = _GetImage(_impl->images, scale);
    auto corners = std::array<_MaterialVariantSlices const*, 4>();

    for (auto corner = 0llu; corner < corners.size(); corner++) {
        auto const it = _materialVariants.find({ kind, intent, cornerRoundness[corner] });

        if (it == _materialVariants.end()) {
            return {};
        }

        corners[corner] = &it->second;
    }

    auto const mixed = (std::ranges::all_of(corners, [&](auto const corner) { return (corner == corners[0]); }) == false);
    auto const image = (mixed ? _GetComposite(_impl->composites, *sheet, factor, corners) : sheet);
    auto const variant = (
        mixed
            ? _MaterialVariantSlices{
                .normal = { 0.0f, 0.0f, _materialCellSize, _materialCellSize },
                .lighter = { _materialCellSize, 0.0f, _materialCellSize, _materialCellSize },
                .darker = { (_materialCellSize * 2.0f), 0.0f, _materialCellSize, _materialCellSize },
                .nPatch = {
                    std::max(corners[0]->nPatch.left, corners[2]->nPatch.left),
                    std::max(corners[0]->nPatch.top, corners[1]->nPatch.top),
                    std::max(corners[1]->nPatch.right, corners[3]->nPatch.right),
                    std::max(corners[2]->nPatch.bottom, corners[3]->nPatch.bottom)
                },
                .border = std::max({ corners[0]->border, corners[1]->border, corners[2]->border, corners[3]->border }),
                .normalEmpty = corners[0]->normalEmpty
            }
            : *corners[0]
    );

    auto const brightness = options.brightness.value_or(MaterialBrightness::Normal);
    auto nPatch = variant.nPatch;
    auto slice = (
        brightness == MaterialBrightness::Lighter ? variant.lighter :
        brightness == MaterialBrightness::Darker ? variant.darker :
                                                   variant.normal
    );

    if (noLeftBorder) {
        slice.x += variant.nPatch.left;
        slice.width -= variant.nPatch.left;
        nPatch.left = 0.0f;
    }

    if (noTopBorder) {
        slice.y += variant.nPatch.top;
        slice.height -= variant.nPatch.top;
        nPatch.top = 0.0f;
    }

    if (noRightBorder) {
        slice.width -= variant.nPatch.right;
        nPatch.right = 0.0f;
    }

    if (noBottomBorder) {
        slice.height -= variant.nPatch.bottom;
        nPatch.bottom = 0.0f;
    }

    /* The edge is drawn `border * factor` device pixels wide; a node unit is `scale` device pixels. */
    auto const border = (
        scale > 0.0f
            ? ((static_cast<float>(variant.border) * factor) / scale)
            : static_cast<float>(variant.border)
    );

    return MaterialVariant{
        .brush = ImageBrush{
            .image = ((brightness == MaterialBrightness::Normal && variant.normalEmpty) ? nullptr : image),
            .slice = (slice * factor),
            .nPatch = (nPatch * factor),
            .filterMag = ImageFilter::Nearest,
            .filterMin = ImageFilter::Nearest,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        },
        .border = border
    };
}

MaterialArrowVariant Material::getArrowVariant(MaterialArrowVariantOptions const& options) const {
    PROFILE

    auto it = _materialArrows.find({ options.direction, options.intent });

    if (it == _materialArrows.end()) {
        it = _materialArrows.find({ options.direction, MaterialIntent::Default });
    }

    auto const [sheet, factor] = _GetImage(_impl->images, options.scale);
    auto const unit = (options.scale > 0.0f ? (factor / options.scale) : 1.0f);

    auto const brightness = options.brightness.value_or(MaterialBrightness::Normal);
    auto const slice = (
        brightness == MaterialBrightness::Lighter ? it->second.lighter :
        brightness == MaterialBrightness::Darker ? it->second.darker :
                                       it->second.normal
    );

    return MaterialArrowVariant{
        .brush = ImageBrush{
            .image = sheet,
            .slice = (slice * factor),
            .filterMag = ImageFilter::Nearest,
            .filterMin = ImageFilter::Nearest,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        },
        .size = Vec2{
            (it->second.normal.width * unit),
            (it->second.normal.height * unit)
        },
        .overlap = (_materialArrowOverlap * unit)
    };
}

MaterialCircleVariant Material::getCircleVariant(MaterialCircleVariantOptions const& options) const {
    PROFILE

    auto const variant = _GetRoundVariant(
        *_impl,
        options.kind.value_or(MaterialKind::Default),
        options.intent.value_or(MaterialIntent::Default),
        options.brightness.value_or(MaterialBrightness::Normal),
        options.width,
        options.width,
        options.scale.value_or(1.0f)
    );

    return MaterialCircleVariant{
        .brush = variant.brush,
        .border = variant.border,
        .size = variant.size
    };
}

MaterialCapsuleVariant Material::getCapsuleVariant(MaterialCapsuleVariantOptions const& options) const {
    PROFILE

    return _GetRoundVariant(
        *_impl,
        options.kind.value_or(MaterialKind::Default),
        options.intent.value_or(MaterialIntent::Default),
        options.brightness.value_or(MaterialBrightness::Normal),
        options.width,
        options.height,
        options.scale.value_or(1.0f)
    );
}

} /* namespace Rocket */
