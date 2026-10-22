#pragma once

#include "Render/Lighting/LightMoodScales.h"

#include <string_view>

namespace Render::Lighting::LightMood
{

// Replaces the mood data (Data/Maps/LightMood.json as loaded or edited) and
// rebuilds the current map's static light.
void SetMoodData(Data::Maps::LightMoodData data);

// Switches the mood and sets its strength (0 to 100 percent) from the next
// frame on (the look preset, Render::Look::ApplyLookPreset); nothing is saved.
// Rebuilds the current map's static light only when either changes.
void SetMoodSwitch(bool enabled, int strengthPercent);

// Applies mood data and the setting without reading or writing a file, and
// rebuilds the current map's static light.
void UseMood(Data::Maps::LightMoodData data, bool enabled, int strengthPercent);

// The scales of the active world. Classic (all 1) while the mood is off and
// outside the world maps (login and character selection).
const Scales& Active();

// Blends the active world's mood towards its night mood by `daylight` (1: day, 0: night), with the sun's
// `strength` and `color`. True when any moved by 0.02 or more, or the daylight reached day or night: the
// caller then builds the map's static light again (CreateTerrainLight, or spread over frames).
bool SetSunlight(float daylight, float strength, const float color[3]);

// Logs the scales a map's static light was built with (map load and reload).
void LogMapLight();

// The terrain light sampled under a character, monster or NPC at (x, y), with
// its static part moved from the base to the figure scale.
void CorrectFigureTerrainLight(float x, float y, float light[3]);

// The fixed light Classic gives the player's own character, at the figure scale.
void ScaleOwnHeroLight(float x, float y, const float classicLight[3], float light[3]);

} // namespace Render::Lighting::LightMood
