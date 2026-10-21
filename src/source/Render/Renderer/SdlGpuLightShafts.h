// SdlGpuLightShafts.h: light shafts of the Enhanced scene (post_shafts.frag) and the occluder mask
// they read.
#pragma once

#include "Render/Lighting/LightShafts.h"
#include "SdlGpuPixelFormat.h"

#include <SDL3/SDL_gpu.h>

#include <cstddef>
#include <span>

namespace mu
{

// Mirrors LightShafts in post_shafts.frag.hlsl.
struct SdlGpuLightShaftsUniform
{
    float segments[kMaxLightShafts][4]; // xy start, zw end, UV
    float colors[kMaxLightShafts][4];   // rgb
    Uint32 shaftCount;
    float aspect;
    float glowWidth;
    float decay;
    float length; // how far the dark streaks reach from the light, in screen heights
    float padding[3];
};
static_assert(offsetof(SdlGpuLightShaftsUniform, colors) == 64);
static_assert(offsetof(SdlGpuLightShaftsUniform, shaftCount) == 128);
static_assert(sizeof(SdlGpuLightShaftsUniform) == 160);

// The occluder mask (R8, the frame's size, which the models are replayed into) and the shaft pass
// that reads it into an RGBA16F texture at half the frame's size.
class SdlGpuLightShafts
{
public:
    static constexpr SDL_GPUTextureFormat OccluderFormat = SDL_GPU_TEXTUREFORMAT_R8_UNORM;

    [[nodiscard]] bool CreatePipelines(SDL_GPUDevice* device, SDL_GPUShader* fullscreenVertex, SDL_GPUShader* shafts);
    void DestroyPipelines();
    [[nodiscard]] bool IsAvailable() const;

    // The occluder mask for a frame of `size`, created or resized as needed; null when it cannot be.
    [[nodiscard]] SDL_GPUTexture* OccluderTarget(SdlGpuTargetSize size);

    // The shafts of `shafts` (on screen) through the occluder mask, or null when they cannot be
    // rendered.
    [[nodiscard]] SDL_GPUTexture* Render(SDL_GPUCommandBuffer* commandBuffer,
                                         std::span<const Render::Lighting::ScreenShaft> shafts, float glowWidth,
                                         float decay, float length);

    // Everything, at shutdown.
    void Release();

private:
    void ReleaseTargets();

    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUSampler* m_linearSampler = nullptr;
    SdlGpuTargetSize m_size{};
    SDL_GPUTexture* m_occluders = nullptr;
    SDL_GPUTexture* m_shafts = nullptr;
};

} // namespace mu
