/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <Rocket/Node/Document.hpp>
#include <Rocket/Paint/Brush.hpp>
#include <Rocket/Paint/Svg.hpp>
#include <Rocket/UI/Reconciler.hpp>
#include <Rocket/UI/Theme.hpp>
#include <Rocket/UI/Theme_Private.hpp>
#include <Rocket/UI/Icon.hpp>

namespace Rocket {

using _IconCacheKey = std::tuple<std::string, Vec4, Vec2>;

struct _IconCacheKeyHash {
    std::size_t operator()(_IconCacheKey const& key) const {
        auto const combine = [](std::size_t& seed, float value) {
            seed ^= std::hash<float>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        auto const& [ svg, color, size ] = key;
        auto seed = std::hash<std::string>{}(svg);

        combine(seed, color.red);
        combine(seed, color.green);
        combine(seed, color.blue);
        combine(seed, color.alpha);
        combine(seed, size.width);
        combine(seed, size.height);

        return seed;
    }
};

struct _IconState {
    Image const* image;
    ImageFilter filter;
    Vec2 size;
    Effect<
        Theme const*,
        char const*,
        std::size_t,
        Image const*,
        ImageFilter,
        std::optional<IconIntent>,
        std::optional<IconSize>,
        std::optional<bool>,
        std::optional<Vec4>,
        std::optional<Vec2>,
        float
    > shouldUpdate;
};

static auto _cache = std::unordered_map<_IconCacheKey, Image, _IconCacheKeyHash>();

static IconTheme::Style _ResolveStyle(IconTheme const& theme, IconIntent intent, IconSize size, bool disabled) {
    auto const rules = _GetCascade<IconTheme::Rule>(
        theme.rules,
        [&](IconTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() == false || rule.selector.intent == intent) &&
                (rule.selector.size.has_value() == false || rule.selector.size == size) &&
                (rule.selector.disabled.has_value() == false || rule.selector.disabled == disabled)
            );
        },
        [](IconTheme::Rule const& rule) {
            return (
                (rule.selector.intent.has_value() ? 1 : 0) +
                (rule.selector.size.has_value() ? 1 : 0) +
                (rule.selector.disabled.has_value() ? 1 : 0)
            );
        }
    );

    auto style = IconTheme::Style();

    for (auto const rule : rules) {
        _Cascade(style.color, rule->style.color);
        _Cascade(style.size, rule->style.size);
        _Cascade(style.alpha, rule->style.alpha);
    }

    return style;
}

void Icon(IconProps const& props) {
    PROFILE
    COMPONENT

    auto& state = UseState<_IconState>();
    auto& document = UseContext<class Document>();
    auto const& theme = UseTheme();

    auto const svgData = (props.data.has_value() ? props.data->as<SvgIconData>() : nullptr);
    auto const imageData = (props.data.has_value() ? props.data->as<ImageIconData>() : nullptr);

    auto const svg = (svgData != nullptr ? svgData->data : std::string_view());
    auto const image = (imageData != nullptr ? &imageData->image : nullptr);
    auto const filter = (imageData != nullptr ? imageData->filter : ImageFilter::Nearest);

    if (state.shouldUpdate(&theme, svg.data(), svg.size(), image, filter, props.intent, props.size, props.disabled, props.colorValue, props.sizeValue, document.getScale())) {
        auto const style = _ResolveStyle(theme.icon, props.intent.value_or(IconIntent::Default), props.size.value_or(IconSize::Medium), props.disabled.value_or(false));
        auto const size = props.sizeValue.value_or(style.size.value_or(Vec2{ 0.0f, 0.0f }));
        auto color = props.colorValue.value_or(style.color.value_or(Vec4{ 0.0f, 0.0f, 0.0f, 1.0f }));

        auto imageSize = (size * document.getScale());
        imageSize.width = std::roundf(imageSize.width);
        imageSize.height = std::roundf(imageSize.height);

        if (style.alpha.has_value()) {
            color.alpha = style.alpha.value();
        }

        if (image != nullptr) {
            state.image = image;
        } else if (svgData != nullptr) {
            auto const key = _IconCacheKey(std::string(svg), color, imageSize);
            auto const [ it, inserted ] = _cache.emplace(key, imageSize);

            if (inserted) {
                auto buffer = std::vector<std::uint8_t>();

                ConvertSVGToBitmap(std::get<0>(key), imageSize, color, buffer);

                it->second.setData(imageSize, buffer.data(), ImageDataFormat::RGBA);
            }

            state.image = &it->second;
        } else {
            state.image = nullptr;
        }

        state.filter = filter;
        state.size = size;
    }

    auto node = props.nodeProps;
    node.width = node.width.value_or(state.size.width);
    node.height = node.height.value_or(state.size.height);
    node.background = node.background.value_or(ImageBrush{
        .image = state.image,
        .filterMag = state.filter,
        .filterMin = state.filter,
        .positionX = ImagePosition::Stretch,
        .positionY = ImagePosition::Stretch
    });

    Node(node);
}

} /* namespace Rocket */
