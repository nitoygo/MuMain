// DynamicLight.h: one dynamic terrain light of the current frame, as a point light.
#pragma once

#include <algorithm>

namespace Render::Lighting
{

// How bright a light is: the sum of its brightening (positive) colour channels. Dynamic lights and
// shaft lights are ranked by it.
[[nodiscard]] inline float LightBrightness(const float (&color)[3])
{
    return (std::max)(color[0], 0.f) + (std::max)(color[1], 0.f) + (std::max)(color[2], 0.f);
}

// Height of a dynamic light above the terrain under it, in world units: about the chest of a
// standing character. The game's light calls carry no height.
inline constexpr float kDynamicLightHeightAboveTerrain = 100.0f;

// A light the game added to the terrain light grid (AddTerrainLight into PrimaryTerrainLight).
// The terrain light of a grid vertex at distance d (in cells) grows by
// color * (range - d) / range; a point light of radius range * cell size gives models the same
// falloff. Colours may be negative: such lights darken.
struct DynamicLight
{
    // World position: the light call's x and y, z is the terrain height plus
    // kDynamicLightHeightAboveTerrain.
    float position[3]{};
    float color[3]{};
    // Range of the light call, in terrain cells.
    int range = 0;
    // range in world units.
    float radius = 0.0f;
    // Whether characters cast a shadow away from it: not for skill lights, whose glow runs along a
    // beam or burst rather than from one point.
    bool castsShadows = true;
};

} // namespace Render::Lighting
