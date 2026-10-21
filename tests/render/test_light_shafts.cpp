#include <doctest.h>

#include "Render/Lighting/LightShafts.h"

#include <array>
#include <vector>

using mu::LightShaftKind;
using mu::LightShaftLight;
using Render::Lighting::ScreenShaft;

namespace
{
// Orthographic view-projection (column-major) mapping world x and y in [-100, 100] to clip -1..1,
// with w = z + 1: points at z <= -1 are behind the camera.
constexpr float kView[16] = {0.01f, 0.f, 0.f, 0.f, 0.f, 0.01f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f};

LightShaftLight Light(float x0, float y0, float x1, float y1, float brightness, LightShaftKind kind)
{
    LightShaftLight light;
    light.start[0] = x0;
    light.start[1] = y0;
    light.end[0] = x1;
    light.end[1] = y1;
    light.color[0] = light.color[1] = light.color[2] = brightness / 3.f;
    light.kind = kind;
    return light;
}
} // namespace

TEST_CASE("a shaft light projects to UV, y down, and off-screen or behind-camera lights are left out [render][shafts]")
{
    ScreenShaft screen;
    REQUIRE(Render::Lighting::ProjectShaft(Light(-100.f, 100.f, 100.f, -100.f, 1.f, LightShaftKind::Skill), kView, 0.f,
                                           screen));
    CHECK(screen.start[0] == doctest::Approx(0.f));
    CHECK(screen.start[1] == doctest::Approx(0.f));
    CHECK(screen.end[0] == doctest::Approx(1.f));
    CHECK(screen.end[1] == doctest::Approx(1.f));

    CHECK_FALSE(Render::Lighting::ProjectShaft(Light(300.f, 0.f, 400.f, 0.f, 1.f, LightShaftKind::Skill), kView, 0.25f,
                                               screen));

    LightShaftLight behind = Light(0.f, 0.f, 0.f, 0.f, 1.f, LightShaftKind::Skill);
    behind.start[2] = behind.end[2] = -2.f;
    CHECK_FALSE(Render::Lighting::ProjectShaft(behind, kView, 0.25f, screen));
}

TEST_CASE("a shaft crossing the camera plane keeps its part in front [render][shafts]")
{
    LightShaftLight light = Light(50.f, 0.f, 50.f, 0.f, 1.f, LightShaftKind::Skill);
    light.start[2] = 0.f; // w 1
    light.end[2] = -3.f;  // w -2: behind
    ScreenShaft screen;
    REQUIRE(Render::Lighting::ProjectShaft(light, kView, 10.f, screen));
    CHECK(screen.start[0] == doctest::Approx(0.75f));
    // The clipped end is the point at the near plane, far off to the side but finite.
    CHECK(screen.end[0] > 1.f);
}

TEST_CASE("the nearest point on a screen shaft accounts for the screen's aspect [render][shafts]")
{
    ScreenShaft shaft;
    shaft.start[0] = 0.2f;
    shaft.start[1] = 0.5f;
    shaft.end[0] = 0.8f;
    shaft.end[1] = 0.5f;
    float nearest[2];
    const float above[2] = {0.5f, 0.3f};
    CHECK(Render::Lighting::ShaftDistanceSquared(shaft, above, 2.f, nearest) == doctest::Approx(0.04f));
    CHECK(nearest[0] == doctest::Approx(0.5f));
    CHECK(nearest[1] == doctest::Approx(0.5f));
    // Past the end: the end itself, the x gap counted twice as wide.
    const float beyond[2] = {0.9f, 0.5f};
    CHECK(Render::Lighting::ShaftDistanceSquared(shaft, beyond, 2.f, nearest) == doctest::Approx(0.04f));
    CHECK(nearest[0] == doctest::Approx(0.8f));
}

TEST_CASE("the brightest shaft lights on screen are kept, scaled by their kind's factor [render][shafts]")
{
    const std::vector<LightShaftLight> lights = {
        Light(0.f, 0.f, 0.f, 0.f, 1.f, LightShaftKind::Map),
        Light(10.f, 0.f, 20.f, 0.f, 3.f, LightShaftKind::Skill),
        Light(500.f, 0.f, 500.f, 0.f, 9.f, LightShaftKind::Skill), // off screen
        Light(-10.f, 0.f, -10.f, 0.f, 2.f, LightShaftKind::Map),
        Light(0.f, 10.f, 0.f, 10.f, 0.5f, LightShaftKind::Skill),
        Light(0.f, -10.f, 0.f, -10.f, 1.5f, LightShaftKind::Map),
    };
    std::array<ScreenShaft, mu::kMaxLightShafts> shafts{};

    REQUIRE(Render::Lighting::SelectScreenShafts(lights, kView, 0.25f, 1.f, 1.f, shafts) == 4);
    CHECK(Render::Lighting::ShaftBrightness(shafts[0].color) == doctest::Approx(3.f));
    CHECK(Render::Lighting::ShaftBrightness(shafts[1].color) == doctest::Approx(2.f));
    CHECK(Render::Lighting::ShaftBrightness(shafts[2].color) == doctest::Approx(1.5f));
    CHECK(Render::Lighting::ShaftBrightness(shafts[3].color) == doctest::Approx(1.f));

    // Map shafts off, skills doubled.
    REQUIRE(Render::Lighting::SelectScreenShafts(lights, kView, 0.25f, 2.f, 0.f, shafts) == 2);
    CHECK(Render::Lighting::ShaftBrightness(shafts[0].color) == doctest::Approx(6.f));
    CHECK(Render::Lighting::ShaftBrightness(shafts[1].color) == doctest::Approx(1.f));
}

TEST_CASE("a full shaft list keeps the brightest lights [render][shafts]")
{
    Render::Lighting::LightShaftList list;
    for (std::size_t i = 0; i < Render::Lighting::LightShaftList::kCapacity; ++i)
    {
        const float x = static_cast<float>(i);
        list.Add(Light(x, 0.f, x, 0.f, 1.f, LightShaftKind::Map));
    }
    list.Add(Light(0.f, 0.f, 0.f, 0.f, 5.f, LightShaftKind::Skill));
    list.Add(Light(0.f, 0.f, 0.f, 0.f, 0.5f, LightShaftKind::Skill));
    REQUIRE(list.Lights().size() == Render::Lighting::LightShaftList::kCapacity);
    float brightest = 0.f;
    for (const LightShaftLight& light : list.Lights())
    {
        brightest = std::max(brightest, Render::Lighting::ShaftBrightness(light.color));
        CHECK(Render::Lighting::ShaftBrightness(light.color) >= 1.f - 1e-6f);
    }
    CHECK(brightest == doctest::Approx(5.f));
    list.Clear();
    CHECK(list.Lights().empty());
}

TEST_CASE("a map light keeps its shaft for a moment after it was last added; a skill light goes [render][shafts]")
{
    Render::Lighting::LightShaftList list;
    const LightShaftLight torch = Light(10.f, 20.f, 10.f, 20.f, 1.f, LightShaftKind::Map);
    list.Add(torch);
    list.Add(Light(0.f, 0.f, 5.f, 0.f, 2.f, LightShaftKind::Skill));
    list.EndFrame(1.0);
    REQUIRE(list.Lights().size() == 1);
    CHECK(list.Lights()[0].kind == LightShaftKind::Map);

    // Added again at the same place: refreshed, not doubled.
    list.Add(torch);
    list.EndFrame(1.1);
    CHECK(list.Lights().size() == 1);

    // Missed on a few frames: still held.
    list.EndFrame(1.1 + Render::Lighting::kShaftHoldSeconds * 0.5);
    CHECK(list.Lights().size() == 1);
    // Gone once not added for longer than the hold.
    list.EndFrame(1.1 + Render::Lighting::kShaftHoldSeconds * 1.5);
    CHECK(list.Lights().empty());
}

TEST_CASE("a map light's shaft fades in, and fades out before another takes its slot [render][shafts]")
{
    using Render::Lighting::kMapShaftSlots;
    using Render::Lighting::kShaftFadeSeconds;
    Render::Lighting::MapShaftSlots slots;
    std::array<LightShaftLight, kMapShaftSlots + 1> torches{};
    for (std::size_t i = 0; i < torches.size(); ++i)
    {
        const float x = static_cast<float>(i) * 100.f;
        torches[i] = Light(x, 0.f, x, 0.f, 1.f, LightShaftKind::Map);
    }
    const auto weightOf = [&](const LightShaftLight& light)
    {
        for (const auto& slot : slots.Slots())
        {
            if (slot.used && slot.light.start[0] == light.start[0])
            {
                return slot.weight;
            }
        }
        return -1.f;
    };

    // The first kMapShaftSlots fade in over kShaftFadeSeconds.
    const std::span<const LightShaftLight> first(torches.data(), kMapShaftSlots);
    slots.Update(first, kShaftFadeSeconds * 0.5f);
    CHECK(weightOf(torches[0]) == doctest::Approx(0.5f));
    slots.Update(first, kShaftFadeSeconds);
    CHECK(weightOf(torches[0]) == doctest::Approx(1.f));

    // The last torch replaces the first: the first fades out while the newcomer waits for its slot.
    const std::span<const LightShaftLight> second(torches.data() + 1, kMapShaftSlots);
    slots.Update(second, kShaftFadeSeconds * 0.5f);
    CHECK(weightOf(torches[0]) == doctest::Approx(0.5f));
    CHECK(weightOf(torches.back()) == -1.f);
    slots.Update(second, kShaftFadeSeconds * 0.5f);
    CHECK(weightOf(torches[0]) == -1.f);
    CHECK(weightOf(torches.back()) == doctest::Approx(0.5f));
    CHECK(weightOf(torches[1]) == doctest::Approx(1.f));
}
