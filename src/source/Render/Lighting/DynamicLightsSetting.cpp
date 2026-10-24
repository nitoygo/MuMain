#include "stdafx.h"

#include "Render/Lighting/DynamicLightsSetting.h"

#include <chrono>
#include <cmath>

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
    const std::optional<FrameSunDirections> sun = FrameSun();
    return SceneFlag == MAIN_SCENE && DynamicLightsActive() && sun && sun->strength > 0.0f &&
           Render::Look::Value(Data::Maps::LookValue::SunShadowStrength) > 0.0 &&
           mu::GetRenderer().SunShadowsAvailable();
}

namespace
{
std::optional<float> s_timeOverride;
std::optional<float> s_frameHour;
std::optional<FrameSunDirections> s_frameSun;

std::optional<float> ClockHour()
{
    if (s_timeOverride)
    {
        return s_timeOverride;
    }
    // Maps whose profile keeps the sun at noon (indoors, events) never get a clock, whatever the player's value.
    const double dayMinutes = Render::Look::Value(Data::Maps::LookValue::DayMinutes);
    if (dayMinutes <= 0.0 || Render::Look::InheritedValues()[Data::Maps::LookValue::DayMinutes] <= 0.0)
    {
        return std::nullopt;
    }
    const double seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    const double dayLength = dayMinutes * 60.0;
    return static_cast<float>(std::fmod(seconds, dayLength) / dayLength * 24.0);
}
} // namespace

std::optional<float> FrameHour()
{
    return s_frameHour;
}

void SetTimeOverride(std::optional<float> hour)
{
    s_timeOverride = hour;
}

std::optional<FrameSunDirections> FrameSun()
{
    return s_frameSun;
}

void UpdateFrameSun()
{
    s_frameSun.reset();
    s_frameHour.reset();
    if (!DynamicLightsActive())
    {
        return;
    }
    const float fall = gMapManager.InBattleCastle() ? kSunShadowFallBattleCastle : kSunShadowFall;
    const float angle = Render::Look::Value(Data::Maps::LookValue::GroundShadowAngle);
    FrameSunDirections directions{SunDirection(angle, fall), SunDirection(kBakedSunShadowAngle, fall)};
    s_frameHour = ClockHour();
    if (s_frameHour)
    {
        const SunAtTime now = SunAtHour(*s_frameHour, angle, fall);
        directions.sun = now.toSun;
        directions.daylight = now.daylight;
        directions.strength = now.strength;
        // time.dawnWarmth and time.moonTint scale how far the light leaves white.
        const float tint = Render::Look::Value(now.daylight > 0.0f ? Data::Maps::LookValue::DawnWarmth
                                                                    : Data::Maps::LookValue::MoonTint);
        directions.color = glm::vec3(1.0f) + (now.color - glm::vec3(1.0f)) * tint;
    }
    s_frameSun = directions;
}

glm::vec3 FollowFrameSun(const glm::vec3& baked)
{
    return s_frameSun ? FollowSun(baked, s_frameSun->baked, s_frameSun->sun, 5.0f) : baked;
}

} // namespace Render::Lighting
