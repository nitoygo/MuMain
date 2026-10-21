#pragma once

#include <array>
#include <optional>

namespace Data::Effects
{
// How an effect's point lights shine with Enhanced dynamic lights ("dynamicLight" of an effect in
// EffectTypes.json, docs/effect-data.md); unset values keep the effect's own. The global skill
// intensity and radius (dynamic.effectGain, dynamic.effectRange) apply on top.
struct EffectDynamicLight
{
    float intensity = 1.f; // multiplies the light's colour
    float radius = 1.f;    // multiplies the light's range
    // The light's hue (its colour over its strongest channel); the effect's own strongest channel keeps
    // the brightness, so a flickering light keeps flickering.
    std::optional<std::array<float, 3>> color;
    bool shadows = true; // false: the light casts no character shadow

    bool operator==(const EffectDynamicLight&) const = default;
};

inline constexpr float MinEffectLightIntensity = 0.f;
inline constexpr float MaxEffectLightIntensity = 4.f;
inline constexpr float MinEffectLightRadius = 0.25f;
inline constexpr float MaxEffectLightRadius = 4.f;
} // namespace Data::Effects
