// SdlGpuVolumetricFog.h: the Enhanced volumetric fog. A grid of cells (froxels) fitted to the camera's
// view gathers the fog's light front to back (fog_volume.comp); each pixel then reads it at the distance
// of what it shows (fog_apply.comp) into a fog layer the composite applies.
#pragma once

#include "SdlGpuPixelFormat.h"

#include <SDL3/SDL_gpu.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace mu
{

// Mirrors FogUniforms in fog_common.hlsli. Matrix column-major, as glm keeps it.
struct SdlGpuFogUniform
{
    float inverseViewProjection[16];
    float sunViewProjection[16];
    float camera[4];  // xyz, w: nearest slice distance
    float color[4];   // rgb: the air's light, w: farthest slice distance
    float medium[4];  // extinction per world unit at the base, 1 / height, base height, point light glow
    float sun[4];     // towards the sun, w: 1 when the shadow map holds its shadows
    float scatter[4]; // anisotropy, sun scatter, ambient share, shadow lookup step (uv)
    float clouds[4];  // as SceneLightingUniform::cloudParameters
    float cloudOffset[4];
    std::uint32_t size[4];
    std::uint32_t target[4];
};
static_assert(offsetof(SdlGpuFogUniform, camera) == 128);
static_assert(offsetof(SdlGpuFogUniform, size) == 240);
static_assert(sizeof(SdlGpuFogUniform) == 272);

struct SdlGpuFogBlobs
{
    std::span<const Uint8> volume; // fog_volume.comp
    std::span<const Uint8> apply;  // fog_apply.comp
};

class SdlGpuVolumetricFog
{
public:
    static constexpr Uint32 Width = 160;
    static constexpr Uint32 Height = 90;
    static constexpr Uint32 Depth = 64;
    // Both shaders' [numthreads] in x and y.
    static constexpr Uint32 ThreadGroupSize = 8;

    // The compute pipelines and samplers. False when any failed: the fog is then unavailable.
    bool Create(SDL_GPUDevice* device, const SdlGpuFogBlobs& blobs, SDL_GPUShaderFormat format);
    [[nodiscard]] bool IsAvailable() const;
    // Everything, at shutdown.
    void Release();

    // The fog layer of `size` for `depth` (single-sample, sampleable) with the sun's `shadowMap` and the
    // frame's point lights (`dynamicLights`, a mu::DynamicLightsUniform), or null when it could not be
    // rendered. Fills `uniform`'s size and target.
    [[nodiscard]] SDL_GPUTexture* Render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* depth,
                                         SDL_GPUTextureSamplerBinding shadowMap, std::span<const std::byte> dynamicLights,
                                         SdlGpuTargetSize size, SdlGpuFogUniform uniform);
    // The volume and the layer, while the fog is off.
    void ReleaseTargets();

private:
    [[nodiscard]] bool EnsureTargets(SdlGpuTargetSize size);

    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUComputePipeline* m_volumePipeline = nullptr;
    SDL_GPUComputePipeline* m_applyPipeline = nullptr;
    SDL_GPUSampler* m_pointSampler = nullptr;
    SDL_GPUSampler* m_linearSampler = nullptr;
    SDL_GPUTexture* m_volume = nullptr;
    SDL_GPUTexture* m_layer = nullptr;
    SdlGpuTargetSize m_layerSize{};
};

} // namespace mu
