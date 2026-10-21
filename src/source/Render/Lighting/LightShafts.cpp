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
    if (light.kind == LightShaftKind::Map)
    {
        for (std::size_t i = 0; i < m_count; ++i)
        {
            const LightShaftLight& held = m_lights[i];
            if (held.kind == LightShaftKind::Map && std::equal(held.start, held.start + 3, light.start) &&
                std::equal(held.end, held.end + 3, light.end))
            {
                m_lights[i] = light;
                m_seen[i] = true;
                return;
            }
        }
    }
    std::size_t slot = m_count;
    if (m_count == m_lights.size())
    {
        // Full: the new light replaces the dimmest, if it is brighter.
        const auto dimmest = std::min_element(m_lights.begin(), m_lights.end(),
                                              [](const LightShaftLight& left, const LightShaftLight& right)
                                              { return ShaftBrightness(left.color) < ShaftBrightness(right.color); });
        if (ShaftBrightness(dimmest->color) >= ShaftBrightness(light.color))
        {
            return;
        }
        slot = static_cast<std::size_t>(dimmest - m_lights.begin());
    }
    else
    {
        ++m_count;
    }
    m_lights[slot] = light;
    m_seen[slot] = true;
}

void LightShaftList::Clear()
{
    m_count = 0;
}

void LightShaftList::EndFrame(double now)
{
    for (std::size_t i = m_count; i-- > 0;)
    {
        if (m_seen[i])
        {
            m_lastSeen[i] = now;
            m_seen[i] = false;
        }
        if (m_lights[i].kind != LightShaftKind::Map || now - m_lastSeen[i] > kShaftHoldSeconds)
        {
            Remove(i);
        }
    }
}

void LightShaftList::Remove(std::size_t index)
{
    --m_count;
    m_lights[index] = m_lights[m_count];
    m_lastSeen[index] = m_lastSeen[m_count];
    m_seen[index] = m_seen[m_count];
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
                               float skillFactor, float mapFactor, std::span<ScreenShaft> shafts)
{
    // Ranked by brightness, then by nearness to the screen's centre: equally bright lights (the map's
    // flames) keep the same order from frame to frame, whatever order they were added in.
    // shortcut: ranks at most kMaxLightShafts; widen the arrays if a caller asks for more.
    std::array<float, mu::kMaxLightShafts> brightness{};
    std::array<float, mu::kMaxLightShafts> offCentre{};
    shafts = shafts.first(std::min(shafts.size(), brightness.size()));
    std::size_t count = 0;
    for (std::size_t index = 0; index < lights.size(); ++index)
    {
        const LightShaftLight& light = lights[index];
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
        screen.light = index;
        const float dx = (screen.start[0] + screen.end[0]) * 0.5f - 0.5f;
        const float dy = (screen.start[1] + screen.end[1]) * 0.5f - 0.5f;
        const float centre = dx * dx + dy * dy;
        // Keep the brightest: insert in order, the dimmest falling off the end.
        std::size_t slot = count;
        while (slot > 0 &&
               (brightness[slot - 1] < bright || (brightness[slot - 1] == bright && offCentre[slot - 1] > centre)))
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
            offCentre[i] = offCentre[i - 1];
        }
        shafts[slot] = screen;
        brightness[slot] = bright;
        offCentre[slot] = centre;
        count = std::min(count + 1, shafts.size());
    }
    return count;
}

namespace
{
bool SamePlace(const LightShaftLight& left, const LightShaftLight& right)
{
    return left.kind == right.kind && std::equal(left.start, left.start + 3, right.start) &&
           std::equal(left.end, left.end + 3, right.end);
}
} // namespace

void MapShaftSlots::Update(std::span<const LightShaftLight> wanted, float seconds)
{
    wanted = wanted.first(std::min(wanted.size(), m_slots.size()));
    const float step = seconds / kShaftFadeSeconds;
    for (Slot& slot : m_slots)
    {
        if (!slot.used)
        {
            continue;
        }
        const auto still = std::find_if(wanted.begin(), wanted.end(),
                                        [&](const LightShaftLight& light) { return SamePlace(light, slot.light); });
        if (still != wanted.end())
        {
            slot.light = *still;
            slot.weight = std::min(slot.weight + step, 1.f);
        }
        else if ((slot.weight -= step) <= 0.f)
        {
            slot = {};
        }
    }
    for (const LightShaftLight& light : wanted)
    {
        const bool shown = std::any_of(m_slots.begin(), m_slots.end(),
                                       [&](const Slot& slot) { return slot.used && SamePlace(slot.light, light); });
        const auto free = std::find_if(m_slots.begin(), m_slots.end(), [](const Slot& slot) { return !slot.used; });
        if (!shown && free != m_slots.end())
        {
            *free = {light, std::min(step, 1.f), true};
        }
    }
}

std::span<const MapShaftSlots::Slot> MapShaftSlots::Slots() const
{
    return m_slots;
}

} // namespace Render::Lighting
