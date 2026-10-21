// LightShafts.h: the frame's light-shaft lights (mu::LightShaftLight) and where they fall on screen.
#pragma once

#include "Render/Renderer/SceneLighting.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Render::Lighting
{

using mu::LightShaftKind;
using mu::LightShaftLight;

// How far off screen (in UV) a shaft light still casts shafts onto it.
inline constexpr float kShaftScreenMargin = 0.25f;
// The width of the air's glow around a shaft light, in screen heights.
inline constexpr float kShaftGlowWidth = 0.15f;
// How far the dark streak behind a model reaches from the light, in screen heights.
inline constexpr float kShaftLength = 0.35f;
// Weight kept per step of a pixel's march towards a shaft light (post_shafts.frag).
inline constexpr float kShaftDecay = 0.95f;

// The frame's shaft lights; the brightest are kept when more are added than fit.
class LightShaftList
{
public:
    static constexpr std::size_t kCapacity = 32;

    void Add(const LightShaftLight& light);
    void Clear();
    [[nodiscard]] std::span<const LightShaftLight> Lights() const;

private:
    std::array<LightShaftLight, kCapacity> m_lights{};
    std::size_t m_count = 0;
};

[[nodiscard]] LightShaftList& FrameLightShafts();

// Adds a shaft light to the frame's list (Enhanced light shafts): a segment from `start` to `end`.
void AddShaftLight(LightShaftKind kind, const float* start, const float* end, const float* color);

// How bright a shaft light is: the sum of its colour.
[[nodiscard]] float ShaftBrightness(const float (&color)[3]);

// A shaft light on screen: its segment in UV (0..1, y down), its colour scaled by its kind's factor.
struct ScreenShaft
{
    float start[2]{};
    float end[2]{};
    float color[3]{};
};

// `light` seen through `viewProjection` (column-major): its segment clipped to the near plane and
// projected to UV, or false when it lies behind the camera or more than `margin` (UV) off screen.
[[nodiscard]] bool ProjectShaft(const LightShaftLight& light, const float (&viewProjection)[16], float margin,
                                ScreenShaft& screen);

// The squared distance from `uv` to the shaft's segment, in UV scaled by `aspect` (width / height) on
// x, and the nearest point of the segment.
[[nodiscard]] float ShaftDistanceSquared(const ScreenShaft& shaft, const float (&uv)[2], float aspect,
                                         float (&nearest)[2]);

// The (at most mu::kMaxLightShafts) brightest of `lights` on screen, their colours scaled by
// `skillFactor` or `mapFactor` (a light scaled to nothing is left out).
[[nodiscard]] std::size_t SelectScreenShafts(std::span<const LightShaftLight> lights, const float (&viewProjection)[16],
                                             float margin, float skillFactor, float mapFactor,
                                             std::span<ScreenShaft, mu::kMaxLightShafts> shafts);

} // namespace Render::Lighting
