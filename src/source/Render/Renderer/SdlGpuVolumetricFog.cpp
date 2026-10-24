#include "stdafx.h"

#include "Render/Renderer/SdlGpuVolumetricFog.h"

#include "Core/Utilities/Log/MuLogger.h"

namespace mu
{
namespace
{
constexpr SDL_GPUTextureFormat VolumeFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
} // namespace

bool SdlGpuVolumetricFog::Create(SDL_GPUDevice* device, std::span<const Uint8> blob, SDL_GPUShaderFormat format)
{
    m_device = device;
    if (blob.empty() ||
        !SDL_GPUTextureSupportsFormat(device, VolumeFormat, SDL_GPU_TEXTURETYPE_3D,
                                      SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE))
    {
        mu::log::Get("render")->warn("SDL_gpu -- volumetric fog unavailable (no blob or no RGBA16F 3D storage)");
        return false;
    }
    SDL_GPUComputePipelineCreateInfo info{};
    info.code = blob.data();
    info.code_size = blob.size();
    // spirv-cross renames "main" to "main0" in MSL because Metal reserves "main".
    info.entrypoint = format == SDL_GPU_SHADERFORMAT_MSL ? "main0" : "main";
    info.format = format;
    info.num_readwrite_storage_textures = 1;
    info.num_uniform_buffers = 1;
    info.threadcount_x = ThreadGroupSize;
    info.threadcount_y = ThreadGroupSize;
    info.threadcount_z = ThreadGroupSize;
    m_pipeline = SDL_CreateGPUComputePipeline(device, &info);
    if (!m_pipeline)
    {
        mu::log::Get("render")->warn("SDL_gpu -- volumetric fog pipeline failed: {}", SDL_GetError());
        return false;
    }
    return true;
}

bool SdlGpuVolumetricFog::IsAvailable() const
{
    return m_pipeline != nullptr;
}

void SdlGpuVolumetricFog::Release()
{
    ReleaseVolume();
    if (m_pipeline)
    {
        SDL_ReleaseGPUComputePipeline(m_device, m_pipeline);
        m_pipeline = nullptr;
    }
}

void SdlGpuVolumetricFog::ReleaseVolume()
{
    if (m_volume)
    {
        SDL_ReleaseGPUTexture(m_device, m_volume);
        m_volume = nullptr;
    }
}

SDL_GPUTexture* SdlGpuVolumetricFog::Render(SDL_GPUCommandBuffer* commandBuffer)
{
    if (!IsAvailable())
    {
        return nullptr;
    }
    if (!m_volume && (m_volume = CreateVolume()) == nullptr)
    {
        return nullptr;
    }
    const SDL_GPUStorageTextureReadWriteBinding target{m_volume, 0u, 0u, true};
    SDL_GPUComputePass* const pass = SDL_BeginGPUComputePass(commandBuffer, &target, 1, nullptr, 0);
    if (!pass)
    {
        return nullptr;
    }
    SDL_BindGPUComputePipeline(pass, m_pipeline);
    const SdlGpuFogVolumeUniform uniform{{Width, Height, Depth, 0u}};
    SDL_PushGPUComputeUniformData(commandBuffer, 0, &uniform, sizeof(uniform));
    const auto groups = [](Uint32 cells) { return (cells + ThreadGroupSize - 1) / ThreadGroupSize; };
    SDL_DispatchGPUCompute(pass, groups(Width), groups(Height), groups(Depth));
    SDL_EndGPUComputePass(pass);
    return m_volume;
}

SDL_GPUTexture* SdlGpuVolumetricFog::CreateVolume() const
{
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_3D;
    info.format = VolumeFormat;
    info.width = Width;
    info.height = Height;
    info.layer_count_or_depth = Depth;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
    SDL_GPUTexture* const texture = SDL_CreateGPUTexture(m_device, &info);
    if (!texture)
    {
        mu::log::Get("render")->error("SDL_gpu -- fog volume creation failed ({}x{}x{}): {}", Width, Height, Depth,
                                      SDL_GetError());
    }
    return texture;
}

} // namespace mu
