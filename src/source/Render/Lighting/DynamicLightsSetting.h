#pragma once

#include <glm/glm.hpp>

#include <optional>

namespace Render::Lighting
{

// Switches Enhanced dynamic lights from the next frame on (the look preset,
// Render::Look::ApplyLookPreset); nothing is saved. They only take effect while Enhanced lighting
// is on.
void SetDynamicLightsEnabled(bool enabled);

// True when lit models get this frame's dynamic lights per pixel: they are on and the renderer
// draws Enhanced lighting.
[[nodiscard]] bool DynamicLightsActive();

// True when the sun's shadows come from the shadow map: in the game, with dynamic lights active,
// sun.shadow above 0 and a renderer that can draw it. Characters then cast no flat sun shadow.
[[nodiscard]] bool SunShadowsActive();

// This frame's sun (towards it): from dynamic.shadowAngle and the map's shadow length, and the sun the
// map's baked light implies (kBakedSunShadowAngle). None without Enhanced dynamic lights (Classic).
// Taken once per frame (UpdateFrameSun), so every part of a frame sees the same sun.
struct FrameSunDirections
{
    glm::vec3 sun{0.0f, 0.0f, 1.0f};
    glm::vec3 baked{0.0f, 0.0f, 1.0f};
    // How much of the day mood shows (time.daylight).
    float daylight = 1.0f;
    // How strong the light from `sun` is, the moon's by night (sun.strength).
    float strength = 1.0f;
    // The painted light's and the models' brightness (sky.brightness).
    float skyLit = 1.0f;
    // The least light the sky leaves on faces turned from the sun or moon (sky.farLight).
    float skyFarLight = 0.0f;
    // The sky's colour on the sun's side and the other (SkyColorsFor: sky.glow, sky.night).
    glm::vec3 skyToward{1.0f};
    glm::vec3 skyAway{1.0f};
    // The light's colour (SunColor: sun.warmth).
    glm::vec3 color{1.0f};
};
[[nodiscard]] std::optional<FrameSunDirections> FrameSun();

// The hour of the day (0 to 24) while the clock runs: the $time override, else UTC time over a day of
// time.dayMinutes, the same on every client. None while time.dayMinutes is 0 (the sun stays at noon) or
// the map's own profile value is 0 (indoors, events), and without a frame sun.
[[nodiscard]] std::optional<float> FrameHour();

// Takes this frame's sun and hour; called first thing each frame (InitTerrainLight).
void UpdateFrameSun();
// $time: holds the clock at `hour`, or lets it run again (none), from the next frame on. Not saved.
void SetTimeOverride(std::optional<float> hour);

// `baked` (a world-space light vector of the baked light) following this frame's sun (FollowSun), never
// from below 5 degrees; unchanged without one.
[[nodiscard]] glm::vec3 FollowFrameSun(const glm::vec3& baked);

} // namespace Render::Lighting
