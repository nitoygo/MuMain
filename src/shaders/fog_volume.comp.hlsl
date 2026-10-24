// fog_volume.comp.hlsl — the fog along each column of the volume (one screen cell), front to back: each
// slice adds the light its fog scatters towards the camera, dimmed by the fog in front of it. The air
// takes an ambient share of the frame's light everywhere and the sun's (or moon's) where the sun shadow
// map sees it, more of it looking towards the sun. Writes rgb: the light gathered up to the slice's far
// end, a: the share of what lies behind that shows.
#include "fog_common.hlsli"

// The sun shadow map (the far 1x1 one without sun shadows), compared as the lit shaders do.
Texture2D sunShadowMap : register(t0, space0);
SamplerComparisonState sunShadowSampler : register(s0, space0);
RWTexture3D<float4> volume : register(u0, space1);

// How much light a point scatters towards the camera, relative to scattering it evenly
// (Henyey-Greenstein over its isotropic value): above 1 looking towards the light.
float PhaseRatio(float cosine, float g)
{
    const float denominator = 1.0 + g * g - 2.0 * g * cosine;
    return (1.0 - g * g) / (denominator * sqrt(denominator));
}

// 1 where the sun reaches `position`, 0 in a shadow; 1 outside the shadow map and without one.
float SunVisibility(float3 position)
{
    if (sun.w <= 0.0)
        return 1.0;
    const float4 clip = mul(sunViewProjection, float4(position, 1.0));
    const float3 ndc = clip.xyz / clip.w;
    const float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0) || ndc.z >= 1.0)
        return 1.0;
    return sunShadowMap.SampleCmpLevelZero(sunShadowSampler, uv, ndc.z);
}

[numthreads(8, 8, 1)]
void main(uint3 cell : SV_DispatchThreadID)
{
    if (any(cell.xy >= size.xy))
        return;
    const float2 uv = (float2(cell.xy) + 0.5) / float2(size.xy);
    const float3 direction = normalize(WorldAt(uv, 1.0) - camera.xyz);
    const float growth = SliceGrowth();
    // Light scattered from the sun back towards the camera: the angle between them.
    const float sunPhase = PhaseRatio(dot(-direction, normalize(sun.xyz)), scatter.x);
    float3 light = float3(0.0, 0.0, 0.0);
    float transmittance = 1.0;
    float start = 0.0;
    for (uint z = 0; z < size.z; ++z)
    {
        const float end = camera.w * exp(float(z + 1) / float(size.z) * growth);
        const float3 position = camera.xyz + direction * (0.5 * (start + end));
        // Height fog: thickest at the base, thinning by e every fog.height above it.
        const float extinction = medium.x * exp(-max(position.z - medium.z, 0.0) * medium.y);
        const float kept = exp(-extinction * (end - start));
        const float lit = scatter.z + scatter.y * sunPhase * SunVisibility(position);
        light += transmittance * color.rgb * lit * (1.0 - kept);
        transmittance *= kept;
        volume[uint3(cell.xy, z)] = float4(light, transmittance);
        start = end;
    }
}
