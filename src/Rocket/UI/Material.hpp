/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include <optional>
#include <Rocket/Math/Vec2.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Image.hpp>

namespace Rocket {

/** Look of a material: how its fill and edge are drawn. */
enum class MaterialKind {
    Default,
    Bordered,
    Lowered,
    Raised,
    Outlined,
    Flat,
    FlatOutlined,
    Transparent,
    TransparentOutlined,
};

/** Color family of a material, after what it is used for. */
enum class MaterialIntent {
    Default,
    Primary,
    Success,
    Warning,
    Danger
};

/** How rounded the corners of a material are. */
enum class MaterialRoundness {
    None,
    Small,
    Large
};

/** Brightness a material is drawn in: the sheet draws every variant in these three. */
enum class MaterialBrightness {
    Normal,
    Lighter,
    Darker
};

/** Direction a material arrow points to. */
enum class MaterialArrowDirection {
    Up,
    Down,
    Left,
    Right
};

/** Which variant Material::getVariant returns. */
struct MaterialVariantOptions {
    /** Look of the material (default Default). */
    std::optional<MaterialKind> kind;

    /** Color family of the material (default Default). */
    std::optional<MaterialIntent> intent;

    /** Brightness the brush is drawn in (default Normal). */
    std::optional<MaterialBrightness> brightness;

    /** Roundness of every corner that does not set its own (default None). */
    std::optional<MaterialRoundness> roundness;

    /** Roundness of the top-left corner; unset takes `roundness`. */
    std::optional<MaterialRoundness> topLeftRoundness;

    /** Roundness of the top-right corner; unset takes `roundness`. */
    std::optional<MaterialRoundness> topRightRoundness;

    /** Roundness of the bottom-left corner; unset takes `roundness`. */
    std::optional<MaterialRoundness> bottomLeftRoundness;

    /** Roundness of the bottom-right corner; unset takes `roundness`. */
    std::optional<MaterialRoundness> bottomRightRoundness;

    /** Device pixels per node unit; picks the sheet drawn closest to it (default 1). */
    std::optional<float> scale;

    /** Leaves out the left edge, so the material joins a neighbor on that side. */
    std::optional<bool> noLeftBorder = false;

    /** Leaves out the top edge, so the material joins a neighbor on that side. */
    std::optional<bool> noTopBorder = false;

    /** Leaves out the right edge, so the material joins a neighbor on that side. */
    std::optional<bool> noRightBorder = false;

    /** Leaves out the bottom edge, so the material joins a neighbor on that side. */
    std::optional<bool> noBottomBorder = false;

    /**
     * Compares every field.
     *
     * @param other Options to compare with.
     */
    bool operator==(MaterialVariantOptions const& other) const;

    /**
     * Negation of operator==.
     *
     * @param other Options to compare with.
     */
    bool operator!=(MaterialVariantOptions const& other) const;
};

/** The brush that draws a material variant in one brightness, and its edge. */
struct MaterialVariant {
    /** The brush, in the brightness asked for; its image is null when the brightness draws nothing, as the normal brightness of a Transparent material. */
    ImageBrush brush;

    /** Border width in node units, matching the drawn edge at the requested scale; with mixed corners, the widest of their borders. */
    float border;
};

/** Which arrow Material::getArrowVariant returns. */
struct MaterialArrowVariantOptions {
    /** Direction the arrow points to. */
    MaterialArrowDirection direction;

    /** Color family of the arrow; an intent with no arrows of its own takes those of Default. */
    MaterialIntent intent;

    /** Brightness the brush is drawn in (default Normal). */
    std::optional<MaterialBrightness> brightness;

    /** Device pixels per node unit; picks the sheet drawn closest to it. */
    float scale;
};

/** The brush that draws a material arrow in one brightness, and where it goes. */
struct MaterialArrowVariant {
    /** The brush, in the brightness asked for. */
    ImageBrush brush;

    /** Size in node units, so the arrow is drawn pixel for pixel, as a material's edge is. */
    Vec2 size;

    /** How far, in node units, the arrow lies over the edge it points away from: as thick as a material's outline, so the outline breaks where the arrow joins it. */
    float overlap;
};

/** Which circle Material::getCircleVariant returns. */
struct MaterialCircleVariantOptions {
    /** Look of the circle (default Default). */
    std::optional<MaterialKind> kind;

    /** Color family of the circle (default Default). */
    std::optional<MaterialIntent> intent;

    /** Brightness the brush is drawn in (default Normal). */
    std::optional<MaterialBrightness> brightness;

    /** Diameter in node units; rounded to whole pixels of the sheet, see MaterialCircleVariant::size. */
    float width;

    /** Device pixels per node unit; picks the sheet drawn closest to it (default 1). */
    std::optional<float> scale;
};

/** The brush that draws a material circle in one brightness, its edge and its size. */
struct MaterialCircleVariant {
    /** The brush, in the brightness asked for; its image is null when the brightness draws nothing, as the normal brightness of a Transparent material. */
    ImageBrush brush;

    /** Border width in node units, the same as that of the panel of the same kind. */
    float border;

    /** Size in node units the circle is drawn at pixel for pixel: the width asked for, rounded to whole pixels of the sheet. */
    Vec2 size;
};

/** Which capsule Material::getCapsuleVariant returns. */
struct MaterialCapsuleVariantOptions {
    /** Look of the capsule (default Default). */
    std::optional<MaterialKind> kind;

    /** Color family of the capsule (default Default). */
    std::optional<MaterialIntent> intent;

    /** Brightness the brush is drawn in (default Normal). */
    std::optional<MaterialBrightness> brightness;

    /** Width in node units; rounded to whole pixels of the sheet, see MaterialCapsuleVariant::size. */
    float width;

    /** Height in node units; rounded to whole pixels of the sheet. The shorter side is the diameter of the round ends. */
    float height;

    /** Device pixels per node unit; picks the sheet drawn closest to it (default 1). */
    std::optional<float> scale;
};

/** The brush that draws a material capsule in one brightness, its edge and its size. */
struct MaterialCapsuleVariant {
    /** The brush, in the brightness asked for; its image is null when the brightness draws nothing, as the normal brightness of a Transparent material. */
    ImageBrush brush;

    /** Border width in node units, the same as that of the panel of the same kind. */
    float border;

    /** Size in node units the capsule is drawn at pixel for pixel: the width and height asked for, rounded to whole pixels of the sheet. */
    Vec2 size;
};

/**
 * A sprite sheet the brushes of panels and arrows are cut from, drawn at 1x,
 * 2x and 3x. Every sheet follows the layout of the PixelDark one (see
 * GetPixelDarkMaterial): the same grid, the same cell per kind, intent and
 * roundness, and the same edges. Circles and capsules are not on the sheet:
 * they are drawn in the colors of its panels, for any size.
 */
class Material {
public:
    ~Material();

    Material(Material &&) = delete;
    Material(Material const&) = delete;
    Material& operator=(Material &&) = delete;
    Material& operator=(Material const&) = delete;

    /**
     * Makes a material of the three drawings of one sheet; the images must
     * outlive the material and the brushes it returns.
     *
     * @param image1x The sheet drawn at 1x.
     * @param image2x The sheet drawn at 2x.
     * @param image3x The sheet drawn at 3x.
     */
    Material(Image const& image1x, Image const& image2x, Image const& image3x);

    /**
     * Returns the brush of a panel in one brightness, cut from the sheet drawn
     * closest to the scale.
     *
     * The brush is nine-patched, stretched across the node and drawn without
     * smoothing. A combination with no drawn variant returns an empty
     * MaterialVariant.
     *
     * When the corners differ in roundness, the variant is put together from
     * the variants drawn for each corner, one quadrant per corner, into an
     * image that is built once and kept as long as the material.
     *
     * @param options Which variant, in which brightness, at which scale.
     */
    MaterialVariant getVariant(MaterialVariantOptions const& options) const;

    /**
     * Returns the brush of an arrow in one brightness, cut from the sheet drawn
     * closest to the scale.
     *
     * @param options Which arrow, in which brightness, at which scale.
     */
    MaterialArrowVariant getArrowVariant(MaterialArrowVariantOptions const& options) const;

    /**
     * Returns the brush of a circle in one brightness, drawn for the width at
     * the sheet closest to the scale.
     *
     * The circle has the rings of the panel of the same kind and intent, in
     * the colors of that panel, lit from the top as it is. The brush is
     * stretched across the node and drawn without smoothing; draw it at
     * `size` to keep its pixels square. A combination with no drawn panel
     * returns an empty MaterialCircleVariant.
     *
     * Each circle is built once per kind, intent and width in pixels of the
     * sheet, one texel per pixel of the sheet, into an image kept as long as
     * the material and shared by every scale that draws it at that width; a
     * width that keeps changing, as in an animation, builds an image for each.
     *
     * @param options Which circle, in which brightness, at which width and scale.
     */
    MaterialCircleVariant getCircleVariant(MaterialCircleVariantOptions const& options) const;

    /**
     * Returns the brush of a capsule in one brightness: a rectangle whose
     * shorter sides are half circles, drawn as getCircleVariant draws a circle,
     * which is a capsule as wide as it is high. The brush is stretched across
     * the node and drawn without smoothing; draw it at `size`. Each capsule
     * is built once per kind, intent and size in pixels of the sheet, and kept
     * as a circle is.
     *
     * @param options Which capsule, in which brightness, at which size and scale.
     */
    MaterialCapsuleVariant getCapsuleVariant(MaterialCapsuleVariantOptions const& options) const;
private:
    struct _Material* _impl;
};

} /* namespace Rocket */
