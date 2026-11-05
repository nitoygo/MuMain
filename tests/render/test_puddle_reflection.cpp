#include "stdafx.h"
#include "doctest.h"

#include "Camera/CameraProjection.h"
#include "Render/Effects/PuddleReflection.h"

using namespace Render::PuddleReflection;

TEST_CASE("A puddle shows fully inside and fades to nothing at its rim [puddle]")
{
    CHECK(EdgeAlpha(0.f, 0.f) == doctest::Approx(1.f));
    CHECK(EdgeAlpha(0.5f, 0.f) == doctest::Approx(1.f));
    CHECK(EdgeAlpha(0.85f, 0.f) == doctest::Approx(0.5f));
    CHECK(EdgeAlpha(1.f, 0.f) == doctest::Approx(0.f));
    CHECK(EdgeAlpha(1.f, 1.f) == doctest::Approx(0.f));
}

TEST_CASE("The mirror keeps points on the water and flips height across it [puddle]")
{
    float m[16];
    CameraProjection::MirrorMatrix(165.f, m);
    const auto mirrored = [&m](float z) { return m[10] * z + m[14]; };
    CHECK(mirrored(165.f) == doctest::Approx(165.f));
    CHECK(mirrored(345.f) == doctest::Approx(-15.f));
    CHECK(m[0] == 1.f);
    CHECK(m[5] == 1.f);
}
