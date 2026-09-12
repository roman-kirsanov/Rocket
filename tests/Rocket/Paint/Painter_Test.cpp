/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <string>
#include <vector>
#include <functional>
#include <Rocket/Paint/Painter.hpp>
#include <Rocket/Paint/Image.hpp>
#include <Rocket/Paint/Color.hpp>
#define ROCKET_SNAPSHOT_MODULE "Paint"
#include "../../_Snapshots/Snapshot.hpp"
#include <gtest/gtest.h>

using namespace Rocket;

static bool runCase(std::string const& name, int width, int height, std::function<void(Painter&, Image&)> const& render) {
    return _RunSnapshotCase(name, width, height, render);
}

static std::vector<std::uint8_t> makeCheckerboardData() { return _MakeCheckerboardData(); }
static std::vector<std::uint8_t> makeNPatchData() { return _MakeNPatchData(); }

static auto const checkerData = makeCheckerboardData();
static auto const nPatchData  = makeNPatchData();
static auto const checker = Image(Vec2(32.0f, 32.0f), checkerData.data());
static auto const nPatch  = Image(Vec2(48.0f, 48.0f), nPatchData.data());

static auto const red   = Vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
static auto const white = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
static auto const black = Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };

/* --- color --- */

TEST(Painter, ColorOpaqueRed) {
    EXPECT_TRUE(runCase("color-opaque-red", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f } }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ColorSemitransparentGreen) {
    EXPECT_TRUE(runCase("color-semitransparent-green", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f } }, ColorBrush{ .color = { 0.0f, 1.0f, 0.0f, 0.5f } });
    }));
}

TEST(Painter, ColorBlendOverlap) {
    EXPECT_TRUE(runCase("color-blend-overlap", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 16.0f, 16.0f, 64.0f, 64.0f } }, ColorBrush{ .color = { 1.0f, 0.0f, 0.0f, 0.5f } });
        painter.paint(QuadShape{ .rect = { 48.0f, 48.0f, 64.0f, 64.0f } }, ColorBrush{ .color = { 0.0f, 0.0f, 1.0f, 0.5f } });
    }));
}

/* --- shape --- */

TEST(Painter, ShapeEllipseCircle) {
    EXPECT_TRUE(runCase("shape-ellipse-circle", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(EllipseShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f } }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeEllipseWide) {
    EXPECT_TRUE(runCase("shape-ellipse-wide", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(EllipseShape{ .rect = { 8.0f, 40.0f, 112.0f, 48.0f } }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeEllipseGradient) {
    EXPECT_TRUE(runCase("shape-ellipse-gradient", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(EllipseShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .startPosition = Vec2{ 0.0f, 0.0f },
            .stopPosition  = Vec2{ 1.0f, 0.0f },
            .startColor    = red,
            .stopColor     = Vec4{ 0.0f, 0.0f, 1.0f, 1.0f }
        });
    }));
}

TEST(Painter, ShapeQuadOutlineUniform) {
    EXPECT_TRUE(runCase("shape-quad-outline-uniform", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadOutlineShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f }, .border = 8.0f }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadOutlinePeredge) {
    EXPECT_TRUE(runCase("shape-quad-outline-peredge", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadOutlineShape{
            .rect         = { 24.0f, 24.0f, 80.0f, 80.0f },
            .leftBorder   = 4.0f,
            .topBorder    = 8.0f,
            .rightBorder  = 16.0f,
            .bottomBorder = 24.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadOutlinePartial) {
    EXPECT_TRUE(runCase("shape-quad-outline-partial", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadOutlineShape{
            .rect       = { 24.0f, 24.0f, 80.0f, 80.0f },
            .leftBorder = 10.0f,
            .topBorder  = 10.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeEllipseOutline) {
    EXPECT_TRUE(runCase("shape-ellipse-outline", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(EllipseOutlineShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f }, .border = 8.0f }, ColorBrush{ .color = red });
    }));
}

/* --- gradient-linear --- */

TEST(Painter, GradientLinearHorizontal) {
    EXPECT_TRUE(runCase("gradient-linear-horizontal", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .startPosition = Vec2{ 0.0f, 0.0f },
            .stopPosition  = Vec2{ 1.0f, 0.0f },
            .startColor    = red,
            .stopColor     = Vec4{ 0.0f, 0.0f, 1.0f, 1.0f }
        });
    }));
}

TEST(Painter, GradientLinearDiagonal) {
    EXPECT_TRUE(runCase("gradient-linear-diagonal", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .startPosition = Vec2{ 0.0f, 0.0f },
            .stopPosition  = Vec2{ 1.0f, 1.0f },
            .startColor    = Vec4{ 1.0f, 1.0f, 0.0f, 1.0f },
            .stopColor     = Vec4{ 0.0f, 1.0f, 1.0f, 1.0f }
        });
    }));
}

TEST(Painter, GradientLinearStops) {
    EXPECT_TRUE(runCase("gradient-linear-stops", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .startPosition = Vec2{ 0.0f, 0.0f },
            .stopPosition  = Vec2{ 1.0f, 0.0f },
            .stops = {
                { 0.0f,  red },
                { 0.25f, Vec4{ 1.0f, 1.0f, 0.0f, 1.0f } },
                { 0.5f,  Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } },
                { 0.75f, Vec4{ 0.0f, 1.0f, 1.0f, 1.0f } },
                { 1.0f,  Vec4{ 0.0f, 0.0f, 1.0f, 1.0f } }
            }
        });
    }));
}

TEST(Painter, GradientLinearVertical) {
    EXPECT_TRUE(runCase("gradient-linear-vertical", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .startPosition = Vec2{ 0.0f, 0.0f },
            .stopPosition  = Vec2{ 0.0f, 1.0f },
            .startColor    = white,
            .stopColor     = Vec4{ 0.5f, 0.0f, 0.5f, 1.0f }
        });
    }));
}

/* --- gradient-radial --- */

TEST(Painter, GradientRadialCentered) {
    EXPECT_TRUE(runCase("gradient-radial-centered", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .radial        = true,
            .startPosition = Vec2{ 0.5f, 0.5f },
            .stopPosition  = Vec2{ 1.0f, 0.5f },
            .startColor    = Vec4{ 1.0f, 1.0f, 0.0f, 1.0f },
            .stopColor     = Vec4{ 0.3f, 0.0f, 0.5f, 1.0f }
        });
    }));
}

TEST(Painter, GradientRadialStops) {
    EXPECT_TRUE(runCase("gradient-radial-stops", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .radial        = true,
            .startPosition = Vec2{ 0.5f, 0.5f },
            .stopPosition  = Vec2{ 1.0f, 0.5f },
            .stops = {
                { 0.0f, white },
                { 0.4f, red },
                { 1.0f, Vec4{ 0.0f, 0.0f, 0.3f, 1.0f } }
            }
        });
    }));
}

TEST(Painter, GradientRadialOffcenter) {
    EXPECT_TRUE(runCase("gradient-radial-offcenter", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, GradientBrush{
            .radial        = true,
            .startPosition = Vec2{ 0.25f, 0.25f },
            .stopPosition  = Vec2{ 1.0f, 1.0f },
            .startColor    = Vec4{ 0.0f, 1.0f, 1.0f, 1.0f },
            .stopColor     = Vec4{ 0.1f, 0.1f, 0.4f, 1.0f }
        });
    }));
}

/* --- image --- */

TEST(Painter, ImagePositionStart) {
    EXPECT_TRUE(runCase("image-position-start", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Start,
            .positionY = ImagePosition::Start
        });
    }));
}

TEST(Painter, ImagePositionCenter) {
    EXPECT_TRUE(runCase("image-position-center", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Center,
            .positionY = ImagePosition::Center
        });
    }));
}

TEST(Painter, ImagePositionEnd) {
    EXPECT_TRUE(runCase("image-position-end", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::End,
            .positionY = ImagePosition::End
        });
    }));
}

TEST(Painter, ImageStretch) {
    EXPECT_TRUE(runCase("image-stretch", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        });
    }));
}

TEST(Painter, ImageRepeat) {
    EXPECT_TRUE(runCase("image-repeat", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Start,
            .positionY = ImagePosition::Start,
            .repeatX   = true,
            .repeatY   = true
        });
    }));
}

TEST(Painter, ImageTint) {
    EXPECT_TRUE(runCase("image-tint", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, ImageBrush{
            .image     = &checker,
            .color     = Vec4{ 0.2f, 0.8f, 0.2f, 1.0f },
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        });
    }));
}

TEST(Painter, ImageFilterNearest) {
    EXPECT_TRUE(runCase("image-filter-nearest", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 0.0f, 0.0f, 128.0f, 128.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch,
            .filterMag = ImageFilter::Nearest
        });
    }));
}

TEST(Painter, ImageFilterLinear) {
    EXPECT_TRUE(runCase("image-filter-linear", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 0.0f, 0.0f, 128.0f, 128.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch,
            .filterMag = ImageFilter::Linear
        });
    }));
}

TEST(Painter, ImageSlice) {
    EXPECT_TRUE(runCase("image-slice", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 8.0f, 112.0f, 112.0f } }, ImageBrush{
            .image     = &checker,
            .slice     = Vec4{ 8.0f, 8.0f, 16.0f, 16.0f },
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        });
    }));
}

TEST(Painter, ImageNpatch) {
    EXPECT_TRUE(runCase("image-npatch", 192, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 8.0f, 24.0f, 176.0f, 80.0f } }, ImageBrush{
            .image  = &nPatch,
            .nPatch = Vec4{ 12.0f, 12.0f, 12.0f, 12.0f }
        });
    }));
}

/* --- options --- */

TEST(Painter, OptionsOpacity) {
    EXPECT_TRUE(runCase("options-opacity", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f } }, ColorBrush{ .color = red }, PaintOptions{
            .opacity = 0.5f
        });
    }));
}

TEST(Painter, OptionsTransformTranslate) {
    EXPECT_TRUE(runCase("options-transform-translate", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f } }, ColorBrush{ .color = red }, PaintOptions{
            .transform = Mat3().toTranslated(Vec2(20.0f, 10.0f))
        });
    }));
}

TEST(Painter, OptionsTransformRotate) {
    EXPECT_TRUE(runCase("options-transform-rotate", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 40.0f, 40.0f, 48.0f, 48.0f } }, ColorBrush{ .color = red }, PaintOptions{
            .transform = Mat3().toTranslated(Vec2(-64.0f, -64.0f)).toRotated(30.0f).toTranslated(Vec2(64.0f, 64.0f))
        });
    }));
}

TEST(Painter, OptionsScissor) {
    EXPECT_TRUE(runCase("options-scissor", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 0.0f, 0.0f, 128.0f, 128.0f } }, ColorBrush{ .color = red }, PaintOptions{
            .scissor = Vec4{ 32.0f, 32.0f, 64.0f, 64.0f }
        });
    }));
}

/* --- filter-blur --- */

TEST(Painter, BlurRadius4) {
    EXPECT_TRUE(runCase("blur-radius4", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 32.0f, 32.0f, 64.0f, 64.0f } }, ColorBrush{ .color = red }, PaintOptions{
            .filter = BlurFilter{ .radius = 4.0f }
        });
    }));
}

TEST(Painter, BlurRadius16) {
    EXPECT_TRUE(runCase("blur-radius16", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 32.0f, 32.0f, 64.0f, 64.0f } }, ColorBrush{ .color = red }, PaintOptions{
            .filter = BlurFilter{ .radius = 16.0f }
        });
    }));
}

TEST(Painter, BlurImage) {
    EXPECT_TRUE(runCase("blur-image", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 16.0f, 16.0f, 96.0f, 96.0f } }, ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        }, PaintOptions{
            .filter = BlurFilter{ .radius = 6.0f }
        });
    }));
}

/* --- filter-shadow --- */

TEST(Painter, ShadowPlain) {
    EXPECT_TRUE(runCase("shadow-plain", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 40.0f, 40.0f, 48.0f, 48.0f } }, ColorBrush{ .color = white }, PaintOptions{
            .filter = ShadowFilter{ .radius = 8.0f, .color = black }
        });
    }));
}

TEST(Painter, ShadowOffset) {
    EXPECT_TRUE(runCase("shadow-offset", 128, 128, [&](Painter& painter, Image&) {
        auto const shape = QuadShape{ .rect = { 32.0f, 32.0f, 48.0f, 48.0f } };
        painter.paint(shape, ColorBrush{ .color = white }, PaintOptions{
            .filter = ShadowFilter{ .radius = 8.0f, .color = black, .offset = Vec2(8.0f, 8.0f) }
        });
        painter.paint(shape, ColorBrush{ .color = white });
    }));
}

TEST(Painter, ShadowSpread) {
    EXPECT_TRUE(runCase("shadow-spread", 128, 128, [&](Painter& painter, Image&) {
        auto const shape = QuadShape{ .rect = { 40.0f, 40.0f, 48.0f, 48.0f } };
        painter.paint(shape, ColorBrush{ .color = white }, PaintOptions{
            .filter = ShadowFilter{ .radius = 0.0f, .color = black, .spread = Vec2(6.0f, 6.0f) }
        });
        painter.paint(shape, ColorBrush{ .color = white });
    }));
}

TEST(Painter, ShadowColored) {
    EXPECT_TRUE(runCase("shadow-colored", 128, 128, [&](Painter& painter, Image&) {
        auto const shape = QuadShape{ .rect = { 32.0f, 32.0f, 48.0f, 48.0f } };
        painter.paint(shape, ColorBrush{ .color = white }, PaintOptions{
            .filter = ShadowFilter{ .radius = 8.0f, .color = red, .offset = Vec2(8.0f, 8.0f) }
        });
        painter.paint(shape, ColorBrush{ .color = white });
    }));
}

TEST(Painter, ShadowHard) {
    EXPECT_TRUE(runCase("shadow-hard", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 40.0f, 40.0f, 48.0f, 48.0f } }, ColorBrush{ .color = white }, PaintOptions{
            .filter = ShadowFilter{ .radius = 0.0f, .color = { 0.0f, 0.8f, 0.2f, 1.0f } }
        });
    }));
}

/* --- combo --- */

TEST(Painter, ComboGradientBlur) {
    EXPECT_TRUE(runCase("combo-gradient-blur", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f } }, GradientBrush{
            .startPosition = Vec2{ 0.0f, 0.0f },
            .stopPosition  = Vec2{ 1.0f, 0.0f },
            .startColor    = red,
            .stopColor     = Vec4{ 0.0f, 0.0f, 1.0f, 1.0f }
        }, PaintOptions{
            .filter = BlurFilter{ .radius = 8.0f }
        });
    }));
}

TEST(Painter, ComboImageShadow) {
    EXPECT_TRUE(runCase("combo-image-shadow", 128, 128, [&](Painter& painter, Image&) {
        auto const shape = QuadShape{ .rect = { 32.0f, 32.0f, 48.0f, 48.0f } };
        auto const brush = ImageBrush{
            .image     = &checker,
            .positionX = ImagePosition::Stretch,
            .positionY = ImagePosition::Stretch
        };
        painter.paint(shape, brush, PaintOptions{
            .filter = ShadowFilter{ .radius = 8.0f, .color = black, .offset = Vec2(8.0f, 8.0f) }
        });
        painter.paint(shape, brush);
    }));
}

TEST(Painter, ComboScissorBlur) {
    EXPECT_TRUE(runCase("combo-scissor-blur", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{ .rect = { 24.0f, 24.0f, 80.0f, 80.0f } }, ColorBrush{ .color = red }, PaintOptions{
            .scissor = Vec4{ 24.0f, 24.0f, 56.0f, 80.0f },
            .filter  = BlurFilter{ .radius = 8.0f }
        });
    }));
}

/* --- sanity assert (not a snapshot): nested paint passes suspend/resume --- */

TEST(Painter, NestedPaintPassesSuspendResume) {
    auto outer = Image(Vec2{ 32.0f, 32.0f });
    auto inner = Image(Vec2{ 32.0f, 32.0f });
    auto painter = Painter();

    painter.beginPaint(ImagePaintTarget{ .image = outer, .clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f } });
    painter.paint(QuadShape{ Vec4{ 0.0f, 0.0f, 16.0f, 32.0f } }, ColorBrush{ Vec4{ 1.0f, 0.0f, 0.0f, 1.0f } });

    painter.beginPaint(ImagePaintTarget{ .image = inner, .clearColor = Vec4{ 0.0f, 1.0f, 0.0f, 1.0f } });
    painter.endPaint();

    painter.paint(QuadShape{ Vec4{ 16.0f, 0.0f, 16.0f, 32.0f } }, ColorBrush{ Vec4{ 0.0f, 0.0f, 1.0f, 1.0f } });
    painter.endPaint();

    auto outerData = std::vector<std::uint8_t>();
    auto innerData = std::vector<std::uint8_t>();
    outer.getData(outerData);
    inner.getData(innerData);

    auto const at = [](std::vector<std::uint8_t> const& d, int x, int y) {
        return &d[(((std::size_t)y * 32) + x) * 4];
    };

    ASSERT_TRUE(at(outerData, 8, 16)[0] == 255);   /* left half red (pre-suspend) */
    ASSERT_TRUE(at(outerData, 24, 16)[2] == 255);  /* right half blue (post-resume) */
    ASSERT_TRUE(at(innerData, 16, 16)[1] == 255);  /* inner green clear */
}

/* --- layer compositing: a nested offscreen pass composited back --- */

TEST(Painter, ComboNestedLayer) {
    EXPECT_TRUE(runCase("combo-nested-layer", 128, 128, [&](Painter& painter, Image&) {
        /* patterned backdrop so the composite's transparency is visible */
        painter.paint(QuadShape{ .rect = { 0.0f, 0.0f, 128.0f, 128.0f } }, ImageBrush{
            .image = &checker
        });

        /* render a small scene into an offscreen layer via a nested pass —
           the same structure the document uses for opacity/shadow layers */
        auto layer = Image(Vec2(64.0f, 64.0f));

        painter.beginPaint(ImagePaintTarget{
            .image = layer,
            .clearColor = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f }
        });
        painter.paint(QuadShape{ .rect = { 0.0f, 0.0f, 64.0f, 64.0f } }, ColorBrush{ .color = red });
        painter.paint(QuadShape{ .rect = { 16.0f, 16.0f, 32.0f, 32.0f } }, ColorBrush{ .color = black });
        painter.endPaint();

        /* composite the layer back at half opacity */
        painter.paint(QuadShape{ .rect = { 32.0f, 32.0f, 64.0f, 64.0f } }, ImageBrush{
            .image     = &layer,
            .positionX = ImagePosition::Start,
            .positionY = ImagePosition::Start,
            .filterMag = ImageFilter::Nearest,
            .filterMin = ImageFilter::Nearest
        }, PaintOptions{ .opacity = 0.5f });
    }));
}

/* --- border radius (snapshots) --- */

TEST(Painter, ShapeQuadRadiusUniform) {
    EXPECT_TRUE(runCase("shape-quad-radius-uniform", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadShape{
            .rect         = { 14.0f, 24.0f, 100.0f, 80.0f },
            .borderRadius = 20.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadRadiusPercorner) {
    EXPECT_TRUE(runCase("shape-quad-radius-percorner", 128, 128, [&](Painter& painter, Image&) {
        /* four visually distinct radii — catches any corner/quadrant mapping swap */
        painter.paint(QuadShape{
            .rect                    = { 24.0f, 24.0f, 80.0f, 80.0f },
            .borderTopLeftRadius     = 0.0f,
            .borderTopRightRadius    = 10.0f,
            .borderBottomLeftRadius  = 40.0f,
            .borderBottomRightRadius = 25.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadOutlineRadiusUniform) {
    EXPECT_TRUE(runCase("shape-quad-outline-radius-uniform", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadOutlineShape{
            .rect         = { 24.0f, 24.0f, 80.0f, 80.0f },
            .borderRadius = 20.0f,
            .border       = 8.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadOutlineRadiusPeredge) {
    EXPECT_TRUE(runCase("shape-quad-outline-radius-peredge", 128, 128, [&](Painter& painter, Image&) {
        painter.paint(QuadOutlineShape{
            .rect                    = { 24.0f, 24.0f, 80.0f, 80.0f },
            .borderTopLeftRadius     = 0.0f,
            .borderTopRightRadius    = 10.0f,
            .borderBottomLeftRadius  = 40.0f,
            .borderBottomRightRadius = 25.0f,
            .leftBorder              = 10.0f,
            .topBorder               = 2.0f,
            .rightBorder             = 6.0f,
            .bottomBorder            = 4.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadRadiusClamped) {
    EXPECT_TRUE(runCase("shape-quad-radius-clamped", 128, 128, [&](Painter& painter, Image&) {
        /* radius far beyond min(w, h) / 2 clamps to 20 — a pill/capsule shape */
        painter.paint(QuadShape{
            .rect         = { 34.0f, 44.0f, 60.0f, 40.0f },
            .borderRadius = 100.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadRadiusOverride) {
    EXPECT_TRUE(runCase("shape-quad-radius-override", 128, 128, [&](Painter& painter, Image&) {
        /* uniform radius with a single per-corner override — resolve precedence */
        painter.paint(QuadShape{
            .rect                    = { 24.0f, 24.0f, 80.0f, 80.0f },
            .borderRadius            = 12.0f,
            .borderBottomRightRadius = 40.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadOutlineInnerFit) {
    EXPECT_TRUE(runCase("shape-quad-outline-inner-fit", 128, 128, [&](Painter& painter, Image&) {
        /* a contrasting inner fill inset by the border widths, with per-corner
           radius from the documented rule innerR = max(outerR - max(adjacent
           borders), 0), must sit flush against the outline's inner edge */
        auto const rect   = Vec4{ 24.0f, 24.0f, 80.0f, 80.0f };
        auto const left   = 10.0f;
        auto const top    = 2.0f;
        auto const right  = 6.0f;
        auto const bottom = 4.0f;

        painter.paint(QuadOutlineShape{
            .rect                    = rect,
            .borderTopLeftRadius     = 24.0f,
            .borderTopRightRadius    = 12.0f,
            .borderBottomLeftRadius  = 8.0f,
            .borderBottomRightRadius = 20.0f,
            .leftBorder              = left,
            .topBorder               = top,
            .rightBorder             = right,
            .bottomBorder            = bottom
        }, ColorBrush{ .color = red });

        painter.paint(QuadShape{
            .rect = {
                (rect.x + left),
                (rect.y + top),
                (rect.width - (left + right)),
                (rect.height - (top + bottom))
            },
            .borderTopLeftRadius     = std::max(24.0f - std::max(left, top), 0.0f),
            .borderTopRightRadius    = std::max(12.0f - std::max(right, top), 0.0f),
            .borderBottomLeftRadius  = std::max(8.0f - std::max(left, bottom), 0.0f),
            .borderBottomRightRadius = std::max(20.0f - std::max(right, bottom), 0.0f)
        }, ColorBrush{ .color = white });
    }));
}

TEST(Painter, ShapeQuadRadiusSingleCornerFull) {
    EXPECT_TRUE(runCase("shape-quad-radius-single-corner-full", 128, 128, [&](Painter& painter, Image&) {
        /* a lone oversized corner is limited by the overlap rule alone:
           f = min(1, 100/999, 80/999) scales 999 to 80 — a quarter arc
           spanning the entire left edge and 80px along the top edge, with
           the other three corners sharp */
        painter.paint(QuadShape{
            .rect                = { 14.0f, 24.0f, 100.0f, 80.0f },
            .borderTopLeftRadius = 999.0f
        }, ColorBrush{ .color = red });
    }));
}

TEST(Painter, ShapeQuadRadiusProportional) {
    EXPECT_TRUE(runCase("shape-quad-radius-proportional", 128, 128, [&](Painter& painter, Image&) {
        /* competing adjacent radii scale proportionally: f = 100/120 turns
           both top corners into 50, their arcs meeting at the top-edge
           midpoint; bottom corners stay sharp */
        painter.paint(QuadShape{
            .rect                 = { 14.0f, 24.0f, 100.0f, 80.0f },
            .borderTopLeftRadius  = 60.0f,
            .borderTopRightRadius = 60.0f
        }, ColorBrush{ .color = red });
    }));
}

/* --- border radius (programmatic asserts, not snapshots) --- */

static std::vector<std::uint8_t> renderPixels(int width, int height, std::function<void(Painter&)> const& render) {
    auto target  = Image(Vec2((float)width, (float)height));
    auto painter = Painter();

    painter.beginPaint(ImagePaintTarget{
        .image = target,
        .clearColor = Vec4{ 0.1f, 0.1f, 0.1f, 1.0f }
    });
    render(painter);
    painter.endPaint();

    auto pixels = std::vector<std::uint8_t>();
    target.getData(pixels);
    return pixels;
}

static int maxChannelDiff(std::vector<std::uint8_t> const& a, std::vector<std::uint8_t> const& b) {
    auto maxDiff = 0;
    for (std::size_t i = 0; i < a.size(); i++) {
        maxDiff = std::max(maxDiff, std::abs((int)a[i] - (int)b[i]));
    }
    return maxDiff;
}

TEST(Painter, RadiusZeroEquivalence) {
    /* no radius fields, uniform zero, and all-zero overrides must hit the same
       sharp-corner path and produce bit-identical output */
    auto const rect = Vec4{ 24.0f, 24.0f, 80.0f, 80.0f };

    auto const plain = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{ .rect = rect }, ColorBrush{ .color = red });
    });
    auto const uniformZero = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{ .rect = rect, .borderRadius = 0.0f }, ColorBrush{ .color = red });
    });
    auto const overridesZero = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{
            .rect                    = rect,
            .borderTopLeftRadius     = 0.0f,
            .borderTopRightRadius    = 0.0f,
            .borderBottomLeftRadius  = 0.0f,
            .borderBottomRightRadius = 0.0f
        }, ColorBrush{ .color = red });
    });

    ASSERT_EQ(plain.size(), uniformZero.size());
    ASSERT_EQ(plain.size(), overridesZero.size());
    EXPECT_EQ(maxChannelDiff(plain, uniformZero), 0);
    EXPECT_EQ(maxChannelDiff(plain, overridesZero), 0);
}

TEST(Painter, RadiusCornerProbes) {
    /* filled 80x80 quad at (24, 24), radius 20, red on the 0.1-grey clear */
    auto const pixels = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{
            .rect         = { 24.0f, 24.0f, 80.0f, 80.0f },
            .borderRadius = 20.0f
        }, ColorBrush{ .color = red });
    });

    auto const at = [&](int x, int y) {
        return &pixels[(((std::size_t)y * 128) + x) * 4];
    };

    /* 2px inside the top-left rect corner: cut away by the radius-20 arc */
    EXPECT_LT((int)at(26, 26)[0], 60);
    /* rect center: fully filled */
    EXPECT_EQ((int)at(64, 64)[0], 255);
    EXPECT_EQ((int)at(64, 64)[1], 0);
    /* straight top-edge midpoint, 3px inside: unaffected by the corners */
    EXPECT_EQ((int)at(64, 27)[0], 255);
    EXPECT_EQ((int)at(64, 27)[1], 0);
}

/* CPU mirror of Painter.metal's _roundBoxDistance: signed distance to a
   rounded box of the given size with per-corner radii (TL, TR, BR, BL),
   in box-local coordinates with Y down. Corner-region selection: the plain
   box distance max-combined with each corner circle whose radius-sized
   corner square contains the point. */
static float roundBoxDistance(float px, float py, float width, float height, float tl, float tr, float br, float bl) {
    auto const ex = std::max(-px, (px - width));
    auto const ey = std::max(-py, (py - height));
    auto const ox = std::max(ex, 0.0f);
    auto const oy = std::max(ey, 0.0f);
    auto circle = [&](float centerX, float centerY, float radius) {
        return (std::sqrt(((px - centerX) * (px - centerX)) + ((py - centerY) * (py - centerY))) - radius);
    };

    auto distance = (std::sqrt((ox * ox) + (oy * oy)) + std::min(std::max(ex, ey), 0.0f));
    if ((px < tl) && (py < tl)) {
        distance = std::max(distance, circle(tl, tl, tl));
    }
    if ((px > (width - tr)) && (py < tr)) {
        distance = std::max(distance, circle((width - tr), tr, tr));
    }
    if ((px > (width - br)) && (py > (height - br))) {
        distance = std::max(distance, circle((width - br), (height - br), br));
    }
    if ((px < bl) && (py > (height - bl))) {
        distance = std::max(distance, circle(bl, (height - bl), bl));
    }
    return distance;
}

/* Seam/fit: a rounded outline plus an inner fill inset by the border widths
   (inner radius = max(outer - max(adjacent borders), 0), the rule documented
   in Painter.metal) must reproduce a single rounded fill.

   Pixels whose AA coverage is fractional on the shared inner boundary are
   double-blended by the over operator (deficit k*(1-k) of the fill-background
   contrast, up to 57/255 at k = 0.5), so a thin band along that boundary is
   exempted; everywhere else the two renders must match within the snapshot
   channel tolerance — a wrong inner-radius rule would leak differences well
   outside the band. */
static void expectSeamFit(char const* label, float left, float top, float right, float bottom) {
    auto const rect   = Vec4{ 24.0f, 24.0f, 80.0f, 80.0f };
    auto const radius = 20.0f;

    auto const whole = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{ .rect = rect, .borderRadius = radius }, ColorBrush{ .color = red });
    });

    auto const pieces = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadOutlineShape{
            .rect         = rect,
            .borderRadius = radius,
            .leftBorder   = left,
            .topBorder    = top,
            .rightBorder  = right,
            .bottomBorder = bottom
        }, ColorBrush{ .color = red });
        painter.paint(QuadShape{
            .rect = {
                (rect.x + left),
                (rect.y + top),
                (rect.width - (left + right)),
                (rect.height - (top + bottom))
            },
            .borderTopLeftRadius     = std::max(radius - std::max(left, top), 0.0f),
            .borderTopRightRadius    = std::max(radius - std::max(right, top), 0.0f),
            .borderBottomLeftRadius  = std::max(radius - std::max(left, bottom), 0.0f),
            .borderBottomRightRadius = std::max(radius - std::max(right, bottom), 0.0f)
        }, ColorBrush{ .color = red });
    });

    ASSERT_EQ(whole.size(), pieces.size());

    auto const innerX = (rect.x + left);
    auto const innerY = (rect.y + top);
    auto const innerW = (rect.width - (left + right));
    auto const innerH = (rect.height - (top + bottom));
    auto const tl = std::max(radius - std::max(left, top), 0.0f);
    auto const tr = std::max(radius - std::max(right, top), 0.0f);
    auto const br = std::max(radius - std::max(right, bottom), 0.0f);
    auto const bl = std::max(radius - std::max(left, bottom), 0.0f);

    auto maxDiff = 0;
    auto maxOffBandDiff = 0;
    auto offBandCount = 0;

    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            auto const i = (((std::size_t)y * 128) + x) * 4;
            auto pixelDiff = 0;
            for (std::size_t c = 0; c < 4; c++) {
                pixelDiff = std::max(pixelDiff, std::abs((int)whole[i + c] - (int)pieces[i + c]));
            }
            maxDiff = std::max(maxDiff, pixelDiff);

            /* ~1px fwidth AA on either side of the seam, plus half-pixel sampling */
            auto const d = roundBoxDistance(((float)x + 0.5f - innerX), ((float)y + 0.5f - innerY), innerW, innerH, tl, tr, br, bl);
            if (std::abs(d) > 1.5f) {
                maxOffBandDiff = std::max(maxOffBandDiff, pixelDiff);
                if (pixelDiff > _SNAPSHOT_CHANNEL_TOLERANCE) {
                    offBandCount++;
                }
            }
        }
    }

    std::printf("  seam-fit %s: max diff %d on the seam band, %d elsewhere\n", label, maxDiff, maxOffBandDiff);

    /* geometry: away from the seam's AA band the renders are identical */
    EXPECT_EQ(offBandCount, 0);
    /* the band itself may only show the bounded double-blend deficit */
    EXPECT_LE(maxDiff, 64);
}

TEST(Painter, RadiusSeamFitUniform) {
    expectSeamFit("uniform", 8.0f, 8.0f, 8.0f, 8.0f);
}

TEST(Painter, RadiusSeamFitPeredge) {
    expectSeamFit("per-edge", 10.0f, 2.0f, 6.0f, 4.0f);
}

TEST(Painter, RadiusLargeCornerNoQuadrantLeak) {
    /* 100x80 quad, TL=999 (scaled to 80 by the overlap rule): the arc crosses
       the box midline at y=40, where the old quadrant-based radius selection
       switched to the sharp BL radius and filled a ledge from x=0 to ~10.7 */
    auto const pixels = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{
            .rect                = { 14.0f, 24.0f, 100.0f, 80.0f },
            .borderTopLeftRadius = 999.0f
        }, ColorBrush{ .color = red });
    });

    auto const at = [&](int localX, int localY) {
        return &pixels[((((std::size_t)localY + 24) * 128) + ((std::size_t)localX + 14)) * 4];
    };

    /* (1, 45): ~6px outside the radius-80 arc — background now, fill under the bug */
    EXPECT_LT((int)at(1, 45)[0], 60);
    /* (15, 45): ~7px inside the arc — fill */
    EXPECT_EQ((int)at(15, 45)[0], 255);
    /* (2, 77): the arc meets the bottom-left corner vertically, so this stays fill */
    EXPECT_EQ((int)at(2, 77)[0], 255);
}

TEST(Painter, RadiusCircleInvariance) {
    /* on a square, any oversized uniform radius scales down to the inscribed
       circle: uniform 999 and uniform 40 on 80x80 must render identically */
    auto const rect = Vec4{ 24.0f, 24.0f, 80.0f, 80.0f };

    auto const oversized = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{ .rect = rect, .borderRadius = 999.0f }, ColorBrush{ .color = red });
    });
    auto const exact = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{ .rect = rect, .borderRadius = 40.0f }, ColorBrush{ .color = red });
    });

    ASSERT_EQ(oversized.size(), exact.size());
    EXPECT_EQ(maxChannelDiff(oversized, exact), 0);
}

TEST(Painter, RadiusProportionalScale) {
    /* the overlap rule scales all radii by one factor: TL=120, TR=80 on a
       100-wide rect gives f = min(1, 100/200, 80/120, 80/80) = 0.5 exactly,
       so the render must be bit-identical to explicit TL=60, TR=40 (which
       the rule leaves untouched: the top sum equals the width exactly) */
    auto const rect = Vec4{ 14.0f, 24.0f, 100.0f, 80.0f };

    auto const oversized = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{
            .rect                 = rect,
            .borderTopLeftRadius  = 120.0f,
            .borderTopRightRadius = 80.0f
        }, ColorBrush{ .color = red });
    });
    auto const prescaled = renderPixels(128, 128, [&](Painter& painter) {
        painter.paint(QuadShape{
            .rect                 = rect,
            .borderTopLeftRadius  = 60.0f,
            .borderTopRightRadius = 40.0f
        }, ColorBrush{ .color = red });
    });

    ASSERT_EQ(oversized.size(), prescaled.size());
    EXPECT_EQ(maxChannelDiff(oversized, prescaled), 0);
}

/* --- nine-patch at a doubled dest rect: corners stay inset-sized, middle stretches --- */

TEST(Painter, ImageNPatch2x) {
    EXPECT_TRUE(runCase("image-npatch-2x", 384, 256, [&](Painter& painter, Image&) {
        /* nPatch insets are shared by source slicing and the dest frame; at
           documentScale 2 the document swaps to a 2x sprite and doubles the
           insets in lockstep, so source and dest bands always match. With the
           same 1x sprite (real bands 12px), the valid 2x-scaled case keeps the
           12px insets while the dest rect doubles: corners stay 12px and only
           the middle stretches more than at 1x */
        painter.paint(QuadShape{ .rect = { 16.0f, 48.0f, 352.0f, 160.0f } }, ImageBrush{
            .image  = &nPatch,
            .nPatch = Vec4{ 12.0f, 12.0f, 12.0f, 12.0f }
        });
    }));
}
