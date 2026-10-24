// ModelLight.h: the directional light models are lit with, whatever format they come from.
#pragma once

#include <glm/glm.hpp>

namespace Render::Lighting
{

// World-space vector towards the models' light; its length is the directional part's strength
// (Classic lights a face max(0.8 × dot(n, L) + 0.4, 0.2) with it), so the normals it meets must be
// world-oriented. `highlight`: the brighter light (BMD::HighLight, which the game always sets), otherwise
// the map's baked direction; scaled by the light mood's sun. With Enhanced dynamic lights, models in the
// world follow the frame's sun (FollowFrameSun) and are as strong as it (the moon by night); models shown
// in the UI keep theirs.
[[nodiscard]] glm::vec3 ModelLightVector(bool highlight);

// The least diffuse light a world model keeps from the sky on faces turned from a low sun or moon
// (FrameSunDirections::skyFarLight); 0 for models drawn in the UI and without the clock.
[[nodiscard]] float ModelSkyFloor();

} // namespace Render::Lighting
