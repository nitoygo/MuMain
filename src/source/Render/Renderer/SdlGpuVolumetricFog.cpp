#include "stdafx.h"

#include "Render/Renderer/SdlGpuVolumetricFog.h"

#include "Core/Utilities/Log/MuLogger.h"

namespace mu
{
namespace
{
constexpr SDL_GPUTextureFormat FogFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;

SDL_GPUComputePipeline* CreatePipeline(SDL_GPUDevice* device, std::span<const Uint8> blob, SDL_GPUShaderFormat format,
                                       Uint32 samplers, Uint32 uniformBuffers)
{
    if (blob.empty())
    {
        return nullptr;
    }
    SDL_GPUComputePipelineCreateInfo info{};
    info.code = blob.data();
    info.code_size = blob.size();
    // spirv-cross renames "main" to "main0" in MSL because Metal reserves "main".
    info.entrypoint = format == SDL_GPU_SHADERFORMAT_MSL ? "main0" : "main";
    info.format = format;
    info.num_samplers = samplers;
    info.num_readwrite_storage_textures = 1;
    info.num_uniform_buffers = uniformBuffers;
    info.threadcount_x = SdlGpuVolumetricFog::ThreadGroupSize;
    info.threadcount_y = SdlGpuVolumetricFog::ThreadGroupSize;
    info.threadcount_z = 1;
    return SDL_CreateGPUComputePipeline(device, &info);
}

SDL_GPUSampler* CreateSampler(SDL_GPUDevice* device, SDL_GPUFilter filter)
{
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = filter;
    info.mag_filter = filter;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler(device, &info);
}

SDL_GPUTexture* CreateStorage(SDL_GPUDevice* device, SDL_GPUTextureType type, Uint32 width, Uint32 height, Uint32 depth)
{
    SDL_GPUTextureCreateInfo info{};
    info.type = type;
    info.format = FogFormat;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = depth;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
    SDL_GPUTexture* const texture = SDL_CreateGPUTexture(device, &info);
    if (!texture)
    {
        mu::log::Get("render")->error("SDL_gpu -- fog target creation failed ({}x{}x{}): {}", width, height, depth,
                                      SDL_GetError());
    }
    return texture;
}

Uint32 Groups(Uint32 cells)
{
    return (cells + SdlGpuVolumetricFog::ThreadGroupSize - 1) / SdlGpuVolumetricFog::ThreadGroupSize;
}
} // namespace

bool SdlGpuVolumetricFog::Create(SDL_GPUDevice* device, const SdlGpuFogBlobs& blobs, SDL_GPUShaderFormat format)
{
    m_device = device;
    constexpr SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
    if (!SDL_GPUTextureSupportsFormat(device, FogFormat, SDL_GPU_TEXTURETYPE_3D, usage) ||
        !SDL_GPUTextureSupportsFormat(device, FogFormat, SDL_GPU_TEXTURETYPE_2D, usage))
    {
        mu::log::Get("render")->warn("SDL_gpu -- volumetric fog unavailable: no RGBA16F storage textures");
        return false;
    }
    m_volumePipeline = CreatePipeline(device, blobs.volume, format, 1u, 2u);
    m_applyPipeline = CreatePipeline(device, blobs.apply, format, 2u, 1u);
    m_pointSampler = CreateSampler(device, SDL_GPU_FILTER_NEAREST);
    m_linearSampler = CreateSampler(device, SDL_GPU_FILTER_LINEAR);
    if (!IsAvailable())
    {
        mu::log::Get("render")->warn("SDL_gpu -- volumetric fog unavailable: {}", SDL_GetError());
        Release();
        return false;
    }
    return true;
}

bool SdlGpuVolumetricFog::IsAvailable() const
{
    return m_volumePipeline && m_applyPipeline && m_pointSampler && m_linearSampler;
}

void SdlGpuVolumetricFog::Release()
{
    ReleaseTargets();
    for (SDL_GPUComputePipeline** pipeline : {&m_volumePipeline, &m_applyPipeline})
    {
        if (*pipeline)
        {
            SDL_ReleaseGPUComputePipeline(m_device, *pipeline);
            *pipeline = nullptr;
        }
    }
    for (SDL_GPUSampler** sampler : {&m_pointSampler, &m_linearSampler})
    {
        if (*sampler)
        {
            SDL_ReleaseGPUSampler(m_device, *sampler);
            *sampler = nullptr;
        }
    }
}

void SdlGpuVolumetricFog::ReleaseTargets()
{
    for (SDL_GPUTexture** texture : {&m_volume, &m_layer})
    {
        if (*texture)
        {
            SDL_ReleaseGPUTexture(m_device, *texture);
            *texture = nullptr;
        }
    }
    m_layerSize = {};
}

bool SdlGpuVolumetricFog::EnsureTargets(SdlGpuTargetSize size)
{
    if (!m_volume)
    {
        m_volume = CreateStorage(m_device, SDL_GPU_TEXTURETYPE_3D, Width, Height, Depth);
    }
    if (m_layer && (m_layerSize.width != size.width || m_layerSize.height != size.height))
    {
        SDL_ReleaseGPUTexture(m_device, m_layer);
        m_layer = nullptr;
    }
    if (!m_layer)
    {
        m_layer = CreateStorage(m_device, SDL_GPU_TEXTURETYPE_2D, size.width, size.height, 1u);
        m_layerSize = m_layer ? size : SdlGpuTargetSize{};
    }
    return m_volume && m_layer;
}

SDL_GPUTexture* SdlGpuVolumetricFog::Render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* depth,
                                            SDL_GPUTextureSamplerBinding shadowMap,
                                            std::span<const std::byte> dynamicLights, SdlGpuTargetSize size,
                                            SdlGpuFogUniform uniform)
{
    if (!IsAvailable() || !depth || !shadowMap.texture || !shadowMap.sampler || size.width == 0u || size.height == 0u || !EnsureTargets(size))
    {
        return nullptr;
    }
    uniform.size[0] = Width;
    uniform.size[1] = Height;
    uniform.size[2] = Depth;
    uniform.target[0] = size.width;
    uniform.target[1] = size.height;

    const SDL_GPUStorageTextureReadWriteBinding volumeTarget{m_volume, 0u, 0u, true};
    SDL_GPUComputePass* pass = SDL_BeginGPUComputePass(commandBuffer, &volumeTarget, 1, nullptr, 0);
    if (!pass)
    {
        return nullptr;
    }
    SDL_BindGPUComputePipeline(pass, m_volumePipeline);
    SDL_BindGPUComputeSamplers(pass, 0, &shadowMap, 1);
    SDL_PushGPUComputeUniformData(commandBuffer, 0, &uniform, sizeof(uniform));
    SDL_PushGPUComputeUniformData(commandBuffer, 1, dynamicLights.data(), static_cast<Uint32>(dynamicLights.size()));
    SDL_DispatchGPUCompute(pass, Groups(Width), Groups(Height), 1);
    SDL_EndGPUComputePass(pass);

    const SDL_GPUStorageTextureReadWriteBinding layerTarget{m_layer, 0u, 0u, true};
    pass = SDL_BeginGPUComputePass(commandBuffer, &layerTarget, 1, nullptr, 0);
    if (!pass)
    {
        return nullptr;
    }
    SDL_BindGPUComputePipeline(pass, m_applyPipeline);
    const SDL_GPUTextureSamplerBinding inputs[] = {{depth, m_pointSampler}, {m_volume, m_linearSampler}};
    SDL_BindGPUComputeSamplers(pass, 0, inputs, 2);
    SDL_PushGPUComputeUniformData(commandBuffer, 0, &uniform, sizeof(uniform));
    SDL_DispatchGPUCompute(pass, Groups(size.width), Groups(size.height), 1);
    SDL_EndGPUComputePass(pass);
    return m_layer;
}

} // namespace mu
