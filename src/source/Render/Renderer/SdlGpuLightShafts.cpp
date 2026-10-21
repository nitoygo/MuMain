#include "SdlGpuLightShafts.h"

#include "Core/Utilities/Log/MuLogger.h"

#include <algorithm>

namespace mu
{
namespace
{
// Three vertices cover the target with one triangle (post_fullscreen.vert.hlsl).
constexpr Uint32 FullscreenVertexCount = 3u;
constexpr SDL_GPUTextureFormat ShaftFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;

SDL_GPUTexture* CreateTarget(SDL_GPUDevice* device, SdlGpuTargetSize size, SDL_GPUTextureFormat format)
{
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    info.width = size.width;
    info.height = size.height;
    info.layer_count_or_depth = 1u;
    info.num_levels = 1u;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    return SDL_CreateGPUTexture(device, &info);
}

void ReleaseTexture(SDL_GPUDevice* device, SDL_GPUTexture*& texture)
{
    if (texture)
    {
        SDL_ReleaseGPUTexture(device, texture);
        texture = nullptr;
    }
}
} // namespace

bool SdlGpuLightShafts::CreatePipelines(SDL_GPUDevice* device, SDL_GPUShader* fullscreenVertex, SDL_GPUShader* shafts)
{
    if (m_device && m_device != device)
    {
        // Handles of an earlier device, destroyed with it: forget them.
        *this = SdlGpuLightShafts{};
    }
    DestroyPipelines();
    if (!device || !fullscreenVertex || !shafts)
    {
        return false;
    }
    m_device = device;

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = ShaftFormat;
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = fullscreenVertex;
    info.fragment_shader = shafts;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.target_info.color_target_descriptions = &colorTarget;
    info.target_info.num_color_targets = 1;
    m_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!m_linearSampler)
    {
        SDL_GPUSamplerCreateInfo sampler{};
        sampler.min_filter = SDL_GPU_FILTER_LINEAR;
        sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
        sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        m_linearSampler = SDL_CreateGPUSampler(device, &sampler);
    }
    if (!IsAvailable())
    {
        log::Get("render")->warn("SDL_gpu -- light shafts unavailable: {}", SDL_GetError());
        DestroyPipelines();
        return false;
    }
    return true;
}

void SdlGpuLightShafts::DestroyPipelines()
{
    if (m_pipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(m_device, m_pipeline);
        m_pipeline = nullptr;
    }
}

bool SdlGpuLightShafts::IsAvailable() const
{
    return m_pipeline && m_linearSampler;
}

SDL_GPUTexture* SdlGpuLightShafts::OccluderTarget(SdlGpuTargetSize size)
{
    if (!m_device)
    {
        return nullptr;
    }
    if (m_occluders && m_size.width == size.width && m_size.height == size.height)
    {
        return m_occluders;
    }
    ReleaseTargets();
    const SdlGpuTargetSize half{std::max(size.width / 2u, 1u), std::max(size.height / 2u, 1u)};
    m_occluders = CreateTarget(m_device, size, OccluderFormat);
    m_shafts = CreateTarget(m_device, half, ShaftFormat);
    if (!m_occluders || !m_shafts)
    {
        log::Get("render")->error("SDL_gpu -- light shaft targets failed ({}x{}): {}", size.width, size.height,
                                  SDL_GetError());
        ReleaseTargets();
        return nullptr;
    }
    m_size = size;
    return m_occluders;
}

SDL_GPUTexture* SdlGpuLightShafts::Render(SDL_GPUCommandBuffer* commandBuffer,
                                          std::span<const Render::Lighting::ScreenShaft> shafts, float glowWidth,
                                          float decay)
{
    if (!IsAvailable() || !m_occluders || !m_shafts || shafts.empty())
    {
        return nullptr;
    }

    SdlGpuLightShaftsUniform uniform{};
    const std::size_t count = std::min(shafts.size(), kMaxLightShafts);
    for (std::size_t i = 0; i < count; ++i)
    {
        uniform.segments[i][0] = shafts[i].start[0];
        uniform.segments[i][1] = shafts[i].start[1];
        uniform.segments[i][2] = shafts[i].end[0];
        uniform.segments[i][3] = shafts[i].end[1];
        std::copy(shafts[i].color, shafts[i].color + 3, uniform.colors[i]);
    }
    uniform.shaftCount = static_cast<Uint32>(count);
    uniform.aspect = static_cast<float>(m_size.width) / static_cast<float>(std::max(m_size.height, 1u));
    uniform.glowWidth = glowWidth;
    uniform.decay = decay;

    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = m_shafts;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.cycle = true;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);
    if (!pass)
    {
        return nullptr;
    }
    const SdlGpuTargetSize half{std::max(m_size.width / 2u, 1u), std::max(m_size.height / 2u, 1u)};
    const SDL_GPUViewport viewport{0.0f, 0.0f, static_cast<float>(half.width), static_cast<float>(half.height),
                                   0.0f, 1.0f};
    const SDL_Rect scissor{0, 0, static_cast<int>(half.width), static_cast<int>(half.height)};
    SDL_SetGPUViewport(pass, &viewport);
    SDL_SetGPUScissor(pass, &scissor);
    SDL_BindGPUGraphicsPipeline(pass, m_pipeline);
    const SDL_GPUTextureSamplerBinding input{m_occluders, m_linearSampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &input, 1);
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &uniform, sizeof(uniform));
    SDL_DrawGPUPrimitives(pass, FullscreenVertexCount, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return m_shafts;
}

void SdlGpuLightShafts::Release()
{
    DestroyPipelines();
    ReleaseTargets();
    if (m_linearSampler)
    {
        SDL_ReleaseGPUSampler(m_device, m_linearSampler);
        m_linearSampler = nullptr;
    }
    m_device = nullptr;
}

void SdlGpuLightShafts::ReleaseTargets()
{
    ReleaseTexture(m_device, m_occluders);
    ReleaseTexture(m_device, m_shafts);
    m_size = {};
}

} // namespace mu
