#include "Render/Lighting/LightShafts.h"

#include <algorithm>

namespace Render::Lighting
{

namespace
{
// Clip-space w below which a point is at or behind the camera.
constexpr float kNearW = 1e-3f;

struct Clip
{
    float x;
    float y;
    float w;
};

Clip ToClip(const float (&m)[16], const float (&p)[3])
{
    return {m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12], m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
            m[3] * p[0] + m[7] * p[1] + m[11] * p[2] + m[15]};
}

Clip Lerp(const Clip& a, const Clip& b, float t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.w + (b.w - a.w) * t};
}

void ToUv(const Clip& clip, float (&uv)[2])
{
    uv[0] = clip.x / clip.w * 0.5f + 0.5f;
    uv[1] = 0.5f - clip.y / clip.w * 0.5f;
}
} // namespace

void LightShaftList::Add(const LightShaftLight& light)
{
    if (m_count < m_lights.size())
    {
        m_lights[m_count++] = light;
        return;
    }
    // Full: the new light replaces the dimmest, if it is brighter.
    const auto dimmest =
        std::min_element(m_lights.begin(), m_lights.end(), [](const LightShaftLight& left, const LightShaftLight& right)
                         { return ShaftBrightness(left.color) < ShaftBrightness(right.color); });
    if (ShaftBrightness(dimmest->color) < ShaftBrightness(light.color))
    {
        *dimmest = light;
    }
}

void LightShaftList::Clear()
{
    m_count = 0;
}

std::span<const LightShaftLight> LightShaftList::Lights() const
{
    return {m_lights.data(), m_count};
}

LightShaftList& FrameLightShafts()
{
    static LightShaftList lights;
    return lights;
}

void AddShaftLight(LightShaftKind kind, const float* start, const float* end, const float* color)
{
    LightShaftLight light;
    std::copy(start, start + 3, light.start);
    std::copy(end, end + 3, light.end);
    std::copy(color, color + 3, light.color);
    light.kind = kind;
    FrameLightShafts().Add(light);
}

float ShaftBrightness(const float (&color)[3])
{
    return std::max(color[0], 0.f) + std::max(color[1], 0.f) + std::max(color[2], 0.f);
}

bool ProjectShaft(const LightShaftLight& light, const float (&viewProjection)[16], float margin, ScreenShaft& screen)
{
    Clip start = ToClip(viewProjection, light.start);
    Clip end = ToClip(viewProjection, light.end);
    if (start.w < kNearW && end.w < kNearW)
    {
        return false;
    }
    // A segment crossing the camera plane keeps its part in front.
    if (start.w < kNearW)
    {
        start = Lerp(start, end, (kNearW - start.w) / (end.w - start.w));
    }
    else if (end.w < kNearW)
    {
        end = Lerp(end, start, (kNearW - end.w) / (start.w - end.w));
    }
    ToUv(start, screen.start);
    ToUv(end, screen.end);
    for (int axis = 0; axis < 2; ++axis)
    {
        if (std::max(screen.start[axis], screen.end[axis]) < -margin ||
            std::min(screen.start[axis], screen.end[axis]) > 1.f + margin)
        {
            return false;
        }
    }
    std::copy(light.color, light.color + 3, screen.color);
    return true;
}

float ShaftDistanceSquared(const ScreenShaft& shaft, const float (&uv)[2], float aspect, float (&nearest)[2])
{
    const float dx = (shaft.end[0] - shaft.start[0]) * aspect;
    const float dy = shaft.end[1] - shaft.start[1];
    const float px = (uv[0] - shaft.start[0]) * aspect;
    const float py = uv[1] - shaft.start[1];
    const float lengthSquared = dx * dx + dy * dy;
    const float t = lengthSquared > 0.f ? std::clamp((px * dx + py * dy) / lengthSquared, 0.f, 1.f) : 0.f;
    nearest[0] = shaft.start[0] + (shaft.end[0] - shaft.start[0]) * t;
    nearest[1] = shaft.start[1] + dy * t;
    const float ox = px - dx * t;
    const float oy = py - dy * t;
    return ox * ox + oy * oy;
}

std::size_t SelectScreenShafts(std::span<const LightShaftLight> lights, const float (&viewProjection)[16], float margin,
                               float skillFactor, float mapFactor, std::span<ScreenShaft, mu::kMaxLightShafts> shafts)
{
    std::array<float, mu::kMaxLightShafts> brightness{};
    std::size_t count = 0;
    for (const LightShaftLight& light : lights)
    {
        const float factor = light.kind == LightShaftKind::Map ? mapFactor : skillFactor;
        LightShaftLight scaled = light;
        for (float& channel : scaled.color)
        {
            channel *= factor;
        }
        const float bright = ShaftBrightness(scaled.color);
        ScreenShaft screen;
        if (bright <= 0.f || !ProjectShaft(scaled, viewProjection, margin, screen))
        {
            continue;
        }
        // Keep the brightest: insert in order, the dimmest falling off the end.
        std::size_t slot = count;
        while (slot > 0 && brightness[slot - 1] < bright)
        {
            --slot;
        }
        if (slot >= shafts.size())
        {
            continue;
        }
        const std::size_t last = std::min(count, shafts.size() - 1);
        for (std::size_t i = last; i > slot; --i)
        {
            shafts[i] = shafts[i - 1];
            brightness[i] = brightness[i - 1];
        }
        shafts[slot] = screen;
        brightness[slot] = bright;
        count = std::min(count + 1, shafts.size());
    }
    return count;
}

} // namespace Render::Lighting
