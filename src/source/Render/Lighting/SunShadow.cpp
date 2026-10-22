#include "Render/Lighting/SunShadow.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <utility>
#include <array>
#include <cmath>
#include <initializer_list>

namespace Render::Lighting
{
namespace
{
// The box's size and depth move in these steps, so a walking camera keeps the same box (and the
// same texels) until it has gone far enough.
constexpr float kSizeStep = 256.0f;
constexpr float kDepthStep = 512.0f;

// World to light space without translation: light-space x and y stay fixed in the world, so
// snapping them to texels keeps the shadows still while the camera moves.
glm::mat4 LightRotation(const glm::vec3& toSun)
{
    const glm::vec3 up = std::abs(toSun.z) > 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
    return glm::lookAtRH(glm::vec3(0.0f), -toSun, up);
}
} // namespace

glm::vec3 SunDirection(float fallAngleDegrees, float fallPerHeight)
{
    const float angle = glm::radians(fallAngleDegrees);
    // Shadows fall away from the sun: it stands on the other side, fallPerHeight across per unit up.
    return glm::normalize(glm::vec3(-std::cos(angle) * fallPerHeight, -std::sin(angle) * fallPerHeight, 1.0f));
}

glm::vec3 FollowSun(const glm::vec3& baked, const glm::vec3& bakedSun, const glm::vec3& sun, float minElevationDegrees)
{
    const float length = glm::length(baked);
    if (sun == bakedSun || length == 0.0f)
    {
        return baked;
    }
    const auto azimuth = [](const glm::vec3& v) { return std::atan2(v.y, v.x); };
    const auto elevation = [](const glm::vec3& v) { return std::atan2(v.z, std::hypot(v.x, v.y)); };
    const float maxElevation = glm::radians(89.0f);
    const float minElevation = std::min(glm::radians(minElevationDegrees), elevation(baked));
    const float turned = azimuth(baked) + azimuth(sun) - azimuth(bakedSun);
    const float raised =
        std::clamp(elevation(baked) + elevation(sun) - elevation(bakedSun), minElevation, maxElevation);
    return length *
           glm::vec3(std::cos(raised) * std::cos(turned), std::cos(raised) * std::sin(turned), std::sin(raised));
}

SunAtTime SunAtHour(float hour, float noonAngleDegrees, float noonFall)
{
    const glm::vec3 noon = SunDirection(noonAngleDegrees, noonFall);
    if (hour == 12.0f)
    {
        return {noon, 1.0f, 1.0f, glm::vec3(1.0f)};
    }
    const float noonElevation = std::atan2(noon.z, std::hypot(noon.x, noon.y));
    const float pi = glm::pi<float>();
    // Where on its arc a light rising at `rise` stands (0 rising, 1 setting), and the light there.
    const auto onArc = [&](float along)
    {
        const float elevation = noonElevation * std::sin(pi * std::clamp(along, 0.0f, 1.0f));
        const float azimuth = std::atan2(noon.y, noon.x) + (std::clamp(along, 0.0f, 1.0f) - 0.5f) * pi;
        const float shown = std::max(elevation, glm::radians(1.0f));
        return std::pair{
            glm::vec3(std::cos(shown) * std::cos(azimuth), std::cos(shown) * std::sin(azimuth), std::sin(shown)),
            std::max(std::sin(elevation), 0.0f) / std::sin(noonElevation)};
    };
    const float day = (hour - kSunriseHour) / (kSunsetHour - kSunriseHour);
    if (day > 0.0f && day < 1.0f)
    {
        const auto [toSun, strength] = onArc(day);
        const float elevation = noonElevation * std::sin(pi * day);
        // Warm through the morning and afternoon, orange at the horizon.
        const float warmth = std::pow(1.0f - std::min(strength, 1.0f), 0.6f);
        return {toSun, glm::smoothstep(0.0f, glm::radians(kTwilightElevationDegrees), elevation), strength,
                glm::mix(glm::vec3(1.0f), kSunHorizonColor, warmth)};
    }
    // The moon: the sun's path, half a day later.
    const float night = std::fmod(hour - kSunsetHour + 24.0f, 24.0f) / (24.0f - (kSunsetHour - kSunriseHour));
    const auto [toMoon, strength] = onArc(night);
    return {toMoon, 0.0f, kMoonStrength * strength, kMoonColor};
}

SunShadowView BuildSunShadowView(const glm::vec3& toSun, const glm::mat4& sceneViewProjection, float groundLow,
                                 float groundHigh, std::uint32_t resolution)
{
    const glm::dmat4 sceneFromClip = glm::inverse(glm::dmat4(sceneViewProjection));
    const auto unproject = [&](double x, double y, double z)
    {
        const glm::dvec4 point = sceneFromClip * glm::dvec4(x, y, z, 1.0);
        return glm::dvec3(point) / point.w;
    };

    // Where the screen's corner rays cross the lowest and highest ground.
    const glm::mat4 rotation = LightRotation(toSun);
    glm::vec3 low(INFINITY);
    glm::vec3 high(-INFINITY);
    for (const double x : {-1.0, 1.0})
    {
        for (const double y : {-1.0, 1.0})
        {
            const glm::dvec3 nearPoint = unproject(x, y, 0.0);
            const glm::dvec3 farPoint = unproject(x, y, 1.0);
            const double rise = farPoint.z - nearPoint.z;
            for (const float height : {groundLow, groundHigh})
            {
                const double t = std::abs(rise) > 1e-6 ? std::clamp((height - nearPoint.z) / rise, 0.0, 1.0) : 1.0;
                const glm::vec3 lightPoint =
                    glm::vec3(rotation * glm::vec4(glm::vec3(nearPoint + (farPoint - nearPoint) * t), 1.0f));
                low = glm::min(low, lightPoint);
                high = glm::max(high, lightPoint);
            }
        }
    }

    SunShadowView view;
    SunShadowBox& box = view.box;
    const float extent = (std::max)(high.x - low.x, high.y - low.y);
    box.size = (std::floor(extent / kSizeStep) + 2.0f) * kSizeStep;
    const float texel = box.size / static_cast<float>((std::max)(resolution, 1u));
    box.origin = glm::floor(glm::vec2(low) / texel) * texel;
    // The light looks down its -z: the nearest point to the sun has the largest z.
    box.nearDistance = std::floor((-high.z - kSunShadowCasterHeight) / kDepthStep) * kDepthStep;
    box.farDistance = std::ceil((-low.z + kDepthStep) / kDepthStep) * kDepthStep;

    const glm::mat4 projection = glm::orthoRH_ZO(box.origin.x, box.origin.x + box.size, box.origin.y,
                                                 box.origin.y + box.size, box.nearDistance, box.farDistance);
    view.viewProjection = 2.0f * projection * rotation;
    view.sunFromScene = glm::mat4(glm::dmat4(view.viewProjection) * sceneFromClip);
    return view;
}

} // namespace Render::Lighting
