// LightShafts.h: the frame's light-shaft lights (mu::LightShaftLight) and where they fall on screen.
#pragma once

#include "Render/Lighting/DynamicLight.h"
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
// Weight kept per step of a pixel's march towards a shaft light (post_shafts.frag).
inline constexpr float kShaftDecay = 0.95f;

// How long a map light (a torch, a bonfire) keeps its shaft after it was last added. Some flames are
// added only on the frames their effect updates, which at a high frame rate is not every frame.
inline constexpr double kShaftHoldSeconds = 0.25;

// The frame's shaft lights; the brightest are kept when more are added than fit.
class LightShaftList
{
public:
    static constexpr std::size_t kCapacity = 32;

    // Adds a light, or refreshes a map light already held at the same place.
    void Add(const LightShaftLight& light);
    void Clear();
    // Ends the frame at `now` (seconds): skill lights go, map lights not added again within
    // kShaftHoldSeconds go.
    void EndFrame(double now);
    [[nodiscard]] std::span<const LightShaftLight> Lights() const;

private:
    void Remove(std::size_t index);

    std::array<LightShaftLight, kCapacity> m_lights{};
    std::array<double, kCapacity> m_lastSeen{};
    std::array<bool, kCapacity> m_seen{};
    std::size_t m_count = 0;
};

[[nodiscard]] LightShaftList& FrameLightShafts();

// Adds a shaft light to the frame's list (Enhanced light shafts): a segment from `start` to `end`. Its
// colour takes the running effect's light values (CurrentEffectLight): its intensity, hue and shaft.
void AddShaftLight(LightShaftKind kind, const float* start, const float* end, const float* color);

// A shaft light on screen: its segment in UV (0..1, y down), its colour scaled by its kind's factor.
struct ScreenShaft
{
    float start[2]{};
    float end[2]{};
    float color[3]{};
    std::size_t light = 0; // its light's index in the list it was selected from
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
                                             std::span<ScreenShaft> shafts);

// How long a map light's shaft takes to fade in or out as it joins or leaves the ones shown.
inline constexpr float kShaftFadeSeconds = 0.3f;
// The shafts the map's lights share; the remaining one is kept for a skill.
inline constexpr std::size_t kMapShaftSlots = mu::kMaxLightShafts - 1;

// The map lights shown with a shaft, each fading in and out instead of switching at once.
class MapShaftSlots
{
public:
    struct Slot
    {
        LightShaftLight light;
        float weight = 0.f; // 0 to 1: how much of its shaft is shown
        bool used = false;
    };

    // Moves the shown lights towards `wanted` (best first; the first kMapShaftSlots count) over
    // `seconds`: a shown light still wanted fades in, one no longer wanted fades out and frees its
    // slot, and a wanted light not shown takes a free slot.
    void Update(std::span<const LightShaftLight> wanted, float seconds);
    [[nodiscard]] std::span<const Slot> Slots() const;

private:
    std::array<Slot, kMapShaftSlots> m_slots{};
};

} // namespace Render::Lighting
