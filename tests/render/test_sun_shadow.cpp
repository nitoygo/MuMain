#include "Render/Lighting/SunShadow.h"

#include <doctest.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <initializer_list>

namespace
{
using Render::Lighting::BuildSunShadowView;
using Render::Lighting::SunDirection;

// A camera like the game's: high above the ground, looking down at the hero at `target`.
glm::mat4 SceneViewProjection(const glm::vec3& target)
{
    const glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(35.0f), 16.0f / 9.0f, 20.0f, 5000.0f);
    const glm::mat4 view = glm::lookAtRH(target + glm::vec3(0.0f, -900.0f, 1300.0f), target, glm::vec3(0, 0, 1));
    return projection * view;
}

glm::vec3 Ndc(const glm::mat4& viewProjection, const glm::vec3& world)
{
    const glm::vec4 clip = viewProjection * glm::vec4(world, 1.0f);
    return glm::vec3(clip) / clip.w;
}
} // namespace

TEST_CASE("the sun stands opposite where the shadows fall, as high as their length says [render][sun]")
{
    // Shadows towards -x (Classic), half as long as the caster is tall.
    const glm::vec3 toSun = SunDirection(180.0f, 0.5f);
    CHECK(glm::length(toSun) == doctest::Approx(1.0f));
    CHECK(toSun.x > 0.0f);
    CHECK(toSun.y == doctest::Approx(0.0f).epsilon(1e-6));
    CHECK(toSun.x / toSun.z == doctest::Approx(0.5f));

    const glm::vec3 east = SunDirection(90.0f, 0.5f);
    CHECK(east.y < 0.0f);
    CHECK(east.x == doctest::Approx(0.0f).epsilon(1e-6));
}

TEST_CASE("the shadow map covers the ground the camera sees [render][sun]")
{
    const glm::vec3 hero(12800.0f, 12800.0f, 200.0f);
    const glm::mat4 scene = SceneViewProjection(hero);
    const auto view = BuildSunShadowView(SunDirection(180.0f, 0.5f), scene, 0.0f, 600.0f, 2048u);

    // Every ground point at the screen's corners and centre lands inside the map's depth and area.
    const glm::mat4 sceneFromClip = glm::inverse(scene);
    for (const float x : {-1.0f, 0.0f, 1.0f})
    {
        for (const float y : {-1.0f, 0.0f, 1.0f})
        {
            const glm::vec4 nearPoint = sceneFromClip * glm::vec4(x, y, 0.0f, 1.0f);
            const glm::vec4 farPoint = sceneFromClip * glm::vec4(x, y, 1.0f, 1.0f);
            const glm::vec3 a = glm::vec3(nearPoint) / nearPoint.w;
            const glm::vec3 b = glm::vec3(farPoint) / farPoint.w;
            const glm::vec3 ground = a + (b - a) * ((hero.z - a.z) / (b.z - a.z));
            const glm::vec3 shadow = Ndc(view.viewProjection, ground);
            CHECK(std::abs(shadow.x) < 1.0f);
            CHECK(std::abs(shadow.y) < 1.0f);
            CHECK(shadow.z > 0.0f);
            CHECK(shadow.z < 1.0f);
        }
    }
    // A caster high above the ground still lies in front of the map's near plane.
    CHECK(Ndc(view.viewProjection, hero + glm::vec3(0.0f, 0.0f, 2000.0f)).z > 0.0f);
    // Clip w is 2, so the vertex shaders' near-plane clip (w - 1) keeps every vertex.
    CHECK((view.viewProjection * glm::vec4(hero, 1.0f)).w == doctest::Approx(2.0f));
}

TEST_CASE("a recorded scene matrix turns into the sun's [render][sun]")
{
    const glm::mat4 scene = SceneViewProjection({12800.0f, 12800.0f, 200.0f});
    const auto view = BuildSunShadowView(SunDirection(135.0f, 0.5f), scene, 0.0f, 600.0f, 2048u);
    // A model's draw records scene * model; the sun pass draws it with sunFromScene * that.
    const glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(12900.0f, 12750.0f, 180.0f)), 0.7f,
                                        glm::vec3(0.0f, 0.0f, 1.0f));
    for (const glm::vec3 vertex : {glm::vec3(0.0f), glm::vec3(40.0f, -30.0f, 150.0f), glm::vec3(-60.0f, 10.0f, 5.0f)})
    {
        const glm::vec3 expected = Ndc(view.viewProjection * model, vertex);
        const glm::vec3 converted = Ndc(view.sunFromScene * (scene * model), vertex);
        CHECK(converted.x == doctest::Approx(expected.x).epsilon(1e-4));
        CHECK(converted.y == doctest::Approx(expected.y).epsilon(1e-4));
        CHECK(converted.z == doctest::Approx(expected.z).epsilon(1e-4));
    }
}

TEST_CASE("a camera moving less than a texel keeps the shadow map where it is [render][sun]")
{
    const glm::vec3 toSun = SunDirection(180.0f, 0.5f);
    const glm::vec3 hero(12800.0f, 12800.0f, 200.0f);
    const auto view = BuildSunShadowView(toSun, SceneViewProjection(hero), 0.0f, 600.0f, 2048u);
    const float texel = view.box.size / 2048.0f;
    const auto nudged = BuildSunShadowView(toSun, SceneViewProjection(hero + glm::vec3(texel * 0.01f, 0.0f, 0.0f)),
                                           0.0f, 600.0f, 2048u);
    CHECK(nudged.box == view.box);

    // Further away, the map follows, by whole texels.
    const auto moved =
        BuildSunShadowView(toSun, SceneViewProjection(hero + glm::vec3(300.0f, 200.0f, 0.0f)), 0.0f, 600.0f, 2048u);
    CHECK_FALSE(moved.box == view.box);
    const glm::vec2 steps = moved.box.origin / (moved.box.size / 2048.0f);
    CHECK(steps.x == doctest::Approx(std::round(steps.x)).epsilon(1e-3));
    CHECK(steps.y == doctest::Approx(std::round(steps.y)).epsilon(1e-3));
}

TEST_CASE("a baked light vector follows the sun's turn and rise, and is the same at the baked sun [render][sun]")
{
    using Render::Lighting::FollowSun;
    const glm::vec3 bakedSun = SunDirection(Render::Lighting::kBakedSunShadowAngle, 0.5f);
    const glm::vec3 terrain(0.5f, -0.5f, 0.5f);
    const glm::vec3 model(0.0f, -1.5f, 0.0f);

    // At the baked sun: exactly the old vectors.
    CHECK(FollowSun(terrain, bakedSun, bakedSun) == terrain);
    CHECK(FollowSun(model, bakedSun, bakedSun) == model);

    // The sun a quarter turn round (shadows towards 225 degrees instead of 135): so is the vector, as long.
    const glm::vec3 turned = FollowSun(terrain, bakedSun, SunDirection(225.0f, 0.5f));
    CHECK(glm::length(turned) == doctest::Approx(glm::length(terrain)));
    CHECK(turned.x == doctest::Approx(0.5f));
    CHECK(turned.y == doctest::Approx(0.5f));
    CHECK(turned.z == doctest::Approx(0.5f));

    // A higher sun (shorter shadows) raises a level vector by as much.
    const glm::vec3 higherSun = SunDirection(Render::Lighting::kBakedSunShadowAngle, 0.25f);
    const float rise = std::atan2(higherSun.z, std::hypot(higherSun.x, higherSun.y)) -
                       std::atan2(bakedSun.z, std::hypot(bakedSun.x, bakedSun.y));
    const glm::vec3 raised = FollowSun(model, bakedSun, higherSun);
    CHECK(std::atan2(raised.z, std::hypot(raised.x, raised.y)) == doctest::Approx(rise));
    CHECK(glm::length(raised) == doctest::Approx(1.5f));

    // Never past straight up.
    const glm::vec3 steep = FollowSun(glm::vec3(0.0f, 0.0f, 1.0f) + glm::vec3(0.01f, 0.0f, 0.0f), bakedSun,
                                      SunDirection(Render::Lighting::kBakedSunShadowAngle, 0.01f));
    CHECK(std::atan2(steep.z, std::hypot(steep.x, steep.y)) <= glm::radians(89.0f) + 1e-5f);
}

TEST_CASE("the sun rises, stands at today's sun at noon, sets, and is gone at night [render][sun]")
{
    using Render::Lighting::SunAtHour;
    const glm::vec3 noon = SunDirection(135.0f, 0.5f);
    CHECK(SunAtHour(12.0f, 135.0f, 0.5f).toSun == noon);
    CHECK(SunAtHour(12.0f, 135.0f, 0.5f).daylight == 1.0f);

    const auto elevation = [](const glm::vec3& v) { return std::atan2(v.z, std::hypot(v.x, v.y)); };
    const auto azimuth = [](const glm::vec3& v) { return std::atan2(v.y, v.x); };
    const glm::vec3 rise = SunAtHour(6.01f, 135.0f, 0.5f).toSun;
    const glm::vec3 set = SunAtHour(17.99f, 135.0f, 0.5f).toSun;
    // Just after sunrise and before sunset: at the horizon (shown a degree above it), half a turn apart,
    // a quarter turn either side of noon. At 6 and 18 exactly, the moon has the sky.
    CHECK(elevation(rise) == doctest::Approx(glm::radians(1.0f)));
    CHECK(elevation(set) == doctest::Approx(glm::radians(1.0f)));
    CHECK(std::abs(std::remainder(azimuth(set) - azimuth(rise), 2.0f * glm::pi<float>())) ==
          doctest::Approx(glm::pi<float>()).epsilon(0.01));
    CHECK(std::remainder(azimuth(rise) - azimuth(noon), 2.0f * glm::pi<float>()) ==
          doctest::Approx(-glm::pi<float>() / 2.0f).epsilon(0.01));

    // Lower in the morning than at noon; daylight full by mid-morning, none at night.
    CHECK(elevation(SunAtHour(9.0f, 135.0f, 0.5f).toSun) < elevation(noon));
    CHECK(SunAtHour(9.0f, 135.0f, 0.5f).daylight == 1.0f);
    CHECK(SunAtHour(6.2f, 135.0f, 0.5f).daylight > 0.0f);
    CHECK(SunAtHour(6.2f, 135.0f, 0.5f).daylight < 1.0f);
    CHECK(SunAtHour(23.0f, 135.0f, 0.5f).daylight == 0.0f);
    CHECK(SunAtHour(3.0f, 135.0f, 0.5f).daylight == 0.0f);
}

TEST_CASE("a low sun never lights a baked vector from below its floor [render][sun]")
{
    using Render::Lighting::FollowSun;
    const glm::vec3 bakedSun = SunDirection(Render::Lighting::kBakedSunShadowAngle, 0.5f);
    const glm::vec3 lowSun = Render::Lighting::SunAtHour(6.5f, Render::Lighting::kBakedSunShadowAngle, 0.5f).toSun;
    const glm::vec3 terrain = FollowSun(glm::vec3(0.5f, -0.5f, 0.5f), bakedSun, lowSun, 5.0f);
    CHECK(std::atan2(terrain.z, std::hypot(terrain.x, terrain.y)) == doctest::Approx(glm::radians(5.0f)));
    // A level vector stays level: the floor is never above the baked elevation.
    const glm::vec3 model = FollowSun(glm::vec3(0.0f, -1.5f, 0.0f), bakedSun, lowSun, 5.0f);
    CHECK(model.z == doctest::Approx(0.0f).epsilon(1e-5));
}

TEST_CASE("the sun is strongest at noon and gentle when low; the moon lights the night faintly [render][sun]")
{
    using Render::Lighting::SunAtHour;
    CHECK(SunAtHour(12.0f, 135.0f, 0.5f).strength == 1.0f);
    const float morning = SunAtHour(9.0f, 135.0f, 0.5f).strength;
    CHECK(morning > 0.5f);
    CHECK(morning < 1.0f);
    CHECK(SunAtHour(15.0f, 135.0f, 0.5f).strength == doctest::Approx(morning));
    CHECK(SunAtHour(6.5f, 135.0f, 0.5f).strength < morning);

    // Midnight: the moon at its highest, where the sun stands at noon.
    const Render::Lighting::SunAtTime midnight = SunAtHour(0.0f, 135.0f, 0.5f);
    CHECK(midnight.daylight == 0.0f);
    CHECK(midnight.strength == doctest::Approx(Render::Lighting::kMoonStrength));
    const glm::vec3 noon = SunDirection(135.0f, 0.5f);
    CHECK(midnight.toSun.x == doctest::Approx(noon.x));
    CHECK(midnight.toSun.z == doctest::Approx(noon.z));
    // The moon rises as the sun sets.
    CHECK(SunAtHour(18.5f, 135.0f, 0.5f).strength < midnight.strength);
}

TEST_CASE("the sun is white at noon, warmer the lower it stands; the moon is cool [render][sun]")
{
    using Render::Lighting::SunAtHour;
    CHECK(SunAtHour(12.0f, 135.0f, 0.5f).color == glm::vec3(1.0f));
    const glm::vec3 morning = SunAtHour(9.0f, 135.0f, 0.5f).color;
    const glm::vec3 dawn = SunAtHour(6.5f, 135.0f, 0.5f).color;
    CHECK(morning.b < 1.0f);
    CHECK(dawn.b < morning.b);
    CHECK(dawn.r == doctest::Approx(1.0f));
    CHECK(SunAtHour(15.0f, 135.0f, 0.5f).color.b == doctest::Approx(morning.b));
    CHECK(SunAtHour(0.0f, 135.0f, 0.5f).color == Render::Lighting::kMoonColor);
}
