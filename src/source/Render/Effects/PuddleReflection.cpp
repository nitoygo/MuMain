#include "stdafx.h"

#include "Render/Effects/PuddleReflection.h"

#include "Camera/CameraManager.h"
#include "Camera/CameraProjection.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Effects/EffectDraw.h"
#include "Render/Look/LookPresetSetting.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Scaling/UITransform.h"
#include "World/MapInfra/MapManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

extern double WorldTime;

namespace Render::PuddleReflection
{
namespace
{
struct Puddle
{
    int world;
    float x, y;
    float radiusX, radiusY;
};

// shortcut: one hand-placed puddle on Lorencia's flat town ground, move these to Look.json if the idea stays.
constexpr Puddle Puddles[] = {{WD_0LORENCIA, 14250.f, 11850.f, 260.f, 220.f}};
// A reflection reaches about a character's height from where it stands.
constexpr float ReflectionReach = 400.f;
constexpr float RaisedAboveGround = 2.f;
constexpr int GridCells = 16;
constexpr float FadingEdge = 0.3f;
// The map's own water, one repeat over this many world units, drifting this far each millisecond.
constexpr float WaterRepeat = 400.f;
constexpr float WaterDrift = 0.00003f;

bool s_on = true;
bool s_capturing = false;
bool s_reflected = false;
std::uint32_t s_target = 0;
int s_targetWidth = 0;
int s_targetHeight = 0;
std::vector<mu::Vertex3D> s_vertices;

// A capture left open would take the rest of the frame's draws with it.
class CaptureScope
{
public:
    CaptureScope(int width, int height, float planeZ)
    {
        s_capturing = true;
        BeginOpenglForTarget(width, height);
        CameraProjection::ApplyMirroredView(g_Camera, width, height, planeZ);
    }
    ~CaptureScope()
    {
        EndOpengl();
        s_capturing = false;
        mu::GetRenderer().EndOffscreenCapture();
    }
    CaptureScope(const CaptureScope&) = delete;
    CaptureScope& operator=(const CaptureScope&) = delete;
};

const Puddle* VisiblePuddle()
{
    if (!s_on || Look::ActiveLookPreset() != LookPreset::Enhanced)
        return nullptr;
    for (const Puddle& puddle : Puddles)
    {
        if (puddle.world == gMapManager.WorldActive &&
            TestFrustrum2D(puddle.x / TERRAIN_SCALE, puddle.y / TERRAIN_SCALE, -8.f))
            return &puddle;
    }
    return nullptr;
}

std::uint32_t PackedColor(float r, float g, float b, float a)
{
    const auto channel = [](float value) { return static_cast<std::uint32_t>(std::clamp(value, 0.f, 1.f) * 255.f); };
    return channel(r) | channel(g) << 8 | channel(b) << 16 | channel(a) << 24;
}

enum class Layer
{
    Water,
    Reflection,
};

void BuildSurface(const Puddle& puddle, const float color[4], Layer layer)
{
    s_vertices.clear();
    const auto viewport = UI::Scaling::WorldViewport(WindowWidth, WindowHeight, g_Camera.TopViewEnable);
    const float height = RequestTerrainHeight(puddle.x, puddle.y) + RaisedAboveGround;
    const float drift = std::fmod(static_cast<float>(WorldTime) * WaterDrift, 1.f);
    const auto add = [&](float gridX, float gridY)
    {
        vec3_t position = {puddle.x + gridX * puddle.radiusX, puddle.y + gridY * puddle.radiusY, height};
        float u = position[0] / WaterRepeat + drift;
        float v = position[1] / WaterRepeat + drift * 0.6f;
        if (layer == Layer::Reflection)
        {
            // The target holds the frame's view upside down, so a point's place on screen finds its reflection.
            float pixelX = 0.f;
            float pixelY = 0.f;
            CameraProjection::WorldToWindowPixel(g_Camera, position, &pixelX, &pixelY);
            u = (pixelX - viewport.x) / std::max(viewport.width, 1);
            v = 1.f - (pixelY - viewport.y) / std::max(viewport.height, 1);
        }
        s_vertices.push_back({position[0], position[1], position[2], 0.f, 0.f, 1.f, u, v,
                              PackedColor(color[0], color[1], color[2], EdgeAlpha(gridX, gridY) * color[3])});
    };
    const float step = 2.f / GridCells;
    for (int j = 0; j < GridCells; ++j)
    {
        for (int i = 0; i < GridCells; ++i)
        {
            const float x0 = -1.f + i * step;
            const float y0 = -1.f + j * step;
            const float x1 = x0 + step;
            const float y1 = y0 + step;
            add(x0, y0);
            add(x1, y0);
            add(x1, y1);
            add(x0, y0);
            add(x1, y1);
            add(x0, y1);
        }
    }
}
} // namespace

bool IsOn()
{
    return s_on;
}

void SetOn(bool on)
{
    s_on = on;
}

bool IsCapturing()
{
    return s_capturing;
}

float EdgeAlpha(float dx, float dy)
{
    const float distance = std::sqrt(dx * dx + dy * dy);
    return std::clamp((1.f - distance) / FadingEdge, 0.f, 1.f);
}

void CaptureReflection()
{
    s_reflected = false;
    const Puddle* puddle = VisiblePuddle();
    // What the world pass doesn't draw (a hidden game master, a siege cloak) doesn't reflect either.
    if (puddle == nullptr || Hero == nullptr || !IsCharacterDrawn(Hero))
        return;
    const OBJECT* hero = &Hero->Object;
    if (std::abs(hero->Position[0] - puddle->x) > puddle->radiusX + ReflectionReach ||
        std::abs(hero->Position[1] - puddle->y) > puddle->radiusY + ReflectionReach)
        return;

    auto& renderer = mu::GetRenderer();
    const auto viewport = UI::Scaling::WorldViewport(WindowWidth, WindowHeight, g_Camera.TopViewEnable);
    if (viewport.width <= 0 || viewport.height <= 0)
        return;
    if (s_target == 0 || s_targetWidth != viewport.width || s_targetHeight != viewport.height)
    {
        if (s_target != 0)
            renderer.ReleaseRenderTarget(s_target);
        s_target = renderer.CreateRenderTarget(viewport.width, viewport.height);
        s_targetWidth = viewport.width;
        s_targetHeight = viewport.height;
        if (s_target == 0)
            return;
    }
    WaitCharactersAnimation();
    if (renderer.BeginOffscreenCapture(s_target, viewport.width, viewport.height) == 0)
        return;
    {
        const CaptureScope capture(viewport.width, viewport.height, RequestTerrainHeight(puddle->x, puddle->y));
        const mu::ScopedBloomExclusion classicCharacters;
        RenderCharacter(Hero, hero);
    }
    s_reflected = true;
}

void DrawPuddles()
{
    const Puddle* puddle = VisiblePuddle();
    if (puddle == nullptr)
        return;

    const mu::ScopedBloomExclusion classicWater;
    auto& drawer = EffectDrawing::ActiveEffectDrawer();

    // Plain alpha blending: it covers the ground, as EnableAlphaBlend's additive glow can't.
    constexpr float Water[4] = {1.f, 1.f, 1.f, 0.85f};
    EnableAlphaBlend3();
    BindTexture(BITMAP_MAPTILE + 5);
    BuildSurface(*puddle, Water, Layer::Water);
    drawer.DrawQuads({.vertices = s_vertices, .triangles = true});

    if (s_reflected && s_target != 0)
    {
        constexpr float Reflection[4] = {0.85f, 0.9f, 1.f, 0.8f};
        EnableAlphaBlend3();
        BuildSurface(*puddle, Reflection, Layer::Reflection);
        drawer.DrawQuads({.vertices = s_vertices, .texture = s_target, .triangles = true});
    }

    DisableAlphaBlend();
}
} // namespace Render::PuddleReflection
