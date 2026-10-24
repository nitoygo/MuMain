// fog_apply.comp.hlsl — the fog in front of each pixel: the volume read at the distance of what the
// pixel shows (the scene's depth). rgb: the fog's light, a: the share of the scene that shows through.
// The composite applies it as scene × a + rgb.
#include "fog_common.hlsli"

Texture2D sceneDepth : register(t0, space0);
Texture3D fogVolume : register(t1, space0);
SamplerState depthSampler : register(s0, space0);  // point
SamplerState volumeSampler : register(s1, space0); // linear, clamped
RWTexture2D<float4> fogLayer : register(u0, space1);

[numthreads(8, 8, 1)]
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= target.xy))
        return;
    const float2 uv = (float2(pixel.xy) + 0.5) / float2(target.xy);
    const float depth = sceneDepth.SampleLevel(depthSampler, uv, 0).r;
    const float distance = length(WorldAt(uv, depth) - camera.xyz);
    // Nothing drawn there: the fog all the way to the last slice.
    const float along = depth >= 1.0 ? 1.0 : saturate(log(max(distance, camera.w) / camera.w) / SliceGrowth());
    // Each slice holds the fog up to its far end: the slice ending at `along`.
    const float slice = along - 0.5 / float(size.z);
    fogLayer[pixel.xy] = fogVolume.SampleLevel(volumeSampler, float3(uv, slice), 0);
}
