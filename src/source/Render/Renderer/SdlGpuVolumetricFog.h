// SdlGpuVolumetricFog.h: the Enhanced volumetric fog's volume, a grid of cells (froxels) fitted to the
// camera's view, filled by a compute shader and read by the composite.
#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <span>

namespace mu
{

// Mirrors FogVolume in fog_volume.comp.hlsl.
struct SdlGpuFogVolumeUniform
{
    std::uint32_t size[4];
};

class SdlGpuVolumetricFog
{
public:
    static constexpr Uint32 Width = 160;
    static constexpr Uint32 Height = 90;
    static constexpr Uint32 Depth = 64;
    // fog_volume.comp's [numthreads].
    static constexpr Uint32 ThreadGroupSize = 4;

    // The compute pipeline from fog_volume.comp's blob. False when it failed: the fog is then unavailable.
    bool Create(SDL_GPUDevice* device, std::span<const Uint8> blob, SDL_GPUShaderFormat format);
    [[nodiscard]] bool IsAvailable() const;
    // Everything, at shutdown.
    void Release();

    // Fills the volume (created on first use) in a compute pass of its own; the volume, or null when
    // it could not be.
    [[nodiscard]] SDL_GPUTexture* Render(SDL_GPUCommandBuffer* commandBuffer);
    // The volume and its memory, while the fog is off.
    void ReleaseVolume();

private:
    [[nodiscard]] SDL_GPUTexture* CreateVolume() const;

    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUComputePipeline* m_pipeline = nullptr;
    SDL_GPUTexture* m_volume = nullptr;
};

} // namespace mu
