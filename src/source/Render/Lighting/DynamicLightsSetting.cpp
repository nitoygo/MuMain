#include "stdafx.h"

#include "Render/Lighting/DynamicLightsSetting.h"

#include <chrono>
#include <cmath>
#include <cstdint>

#include "Core/Utilities/Log/MuLogger.h"
#include "Render/Lighting/EnhancedLightingSetting.h"
#include "Render/Look/LookProfileSetting.h"
#include "Render/Lighting/SunShadow.h"
#include "Render/Renderer/MuRenderer.h"
#include "World/MapInfra/MapManager.h"
#include "Scenes/SceneCore.h"

namespace Render::Lighting
{
namespace
{
bool s_dynamicLightsEnabled = false;
} // namespace

void SetDynamicLightsEnabled(bool enabled)
{
    s_dynamicLightsEnabled = enabled;
    if (enabled && !EnhancedLightingActive())
    {
        mu::log::Get("render")->info("Enhanced dynamic lights on (applied once Enhanced lighting is on)");
        return;
    }
    mu::log::Get("render")->info("Enhanced dynamic lights {}", enabled ? "on" : "off");
}

bool DynamicLightsActive()
{
    return s_dynamicLightsEnabled && EnhancedLightingActive();
}

bool SunShadowsActive()
{
    return SceneFlag == MAIN_SCENE && DynamicLightsActive() &&
           Render::Look::Value(Data::Maps::LookValue::SunShadowStrength) > 0.0 &&
           mu::GetRenderer().SunShadowsAvailable() && FrameSun()->strength > 0.0f;
}

namespace
{
std::optional<float> s_timeOverride;
} // namespace

std::optional<float> FrameHour()
{
    if (s_timeOverride)
    {
        return s_timeOverride;
    }
    // Maps whose profile keeps the sun at noon (indoors, events) never get a clock, whatever the player's value.
    static int s_world = -1;
    static std::uint32_t s_revision = 0;
    static bool s_mapHasClock = false;
    if (gMapManager.WorldActive != s_world || Render::Look::Revision() != s_revision)
    {
        s_world = gMapManager.WorldActive;
        s_revision = Render::Look::Revision();
        s_mapHasClock = Render::Look::InheritedValues()[Data::Maps::LookValue::DayMinutes] > 0.0;
    }
    const double dayMinutes = Render::Look::Value(Data::Maps::LookValue::DayMinutes);
    if (!s_mapHasClock || dayMinutes <= 0.0)
    {
        return std::nullopt;
    }
    const double seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    const double dayLength = dayMinutes * 60.0;
    return static_cast<float>(std::fmod(seconds, dayLength) / dayLength * 24.0);
}

void SetTimeOverride(std::optional<float> hour)
{
    s_timeOverride = hour;
}

std::optional<FrameSunDirections> FrameSun()
{
    if (!DynamicLightsActive())
    {
        return std::nullopt;
    }
    const float fall = gMapManager.InBattleCastle() ? kSunShadowFallBattleCastle : kSunShadowFall;
    const float angle = Render::Look::Value(Data::Maps::LookValue::GroundShadowAngle);
    FrameSunDirections directions{SunDirection(angle, fall), SunDirection(kBakedSunShadowAngle, fall)};
    if (const std::optional<float> hour = FrameHour())
    {
        const SunAtTime now = SunAtHour(*hour, angle, fall);
        directions.sun = now.toSun;
        directions.daylight = now.daylight;
        directions.strength = now.strength;
        // time.dawnWarmth and time.moonTint scale how far the light leaves white.
        const float tint = Render::Look::Value(now.daylight > 0.0f ? Data::Maps::LookValue::DawnWarmth
                                                                    : Data::Maps::LookValue::MoonTint);
        directions.color = glm::vec3(1.0f) + (now.color - glm::vec3(1.0f)) * tint;
    }
    return directions;
}

glm::vec3 FollowFrameSun(const glm::vec3& baked)
{
    const std::optional<FrameSunDirections> sun = FrameSun();
    if (!sun)
    {
        return baked;
    }
    // Every model asks with the same vector each frame: the last answer is reused.
    static glm::vec3 s_baked{0.0f}, s_sun{0.0f}, s_followed{0.0f};
    if (baked != s_baked || sun->sun != s_sun)
    {
        s_baked = baked;
        s_sun = sun->sun;
        s_followed = FollowSun(baked, sun->baked, sun->sun, 5.0f);
    }
    return s_followed;
}

} // namespace Render::Lighting
