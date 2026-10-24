// fog_volume.comp.hlsl — the fog along each column of the volume (one screen cell), front to back: each
// slice adds the light its fog scatters towards the camera, dimmed by the fog in front of it. The air
// takes an ambient share of the frame's light everywhere, the sun's (or moon's) where the sun shadow
// map sees it, more of it looking towards the sun, and the glow of the frame's point lights near them.
// Writes rgb: the light gathered up to the slice's far end, a: the share of what lies behind that shows.
#include "fog_common.hlsli"
#include "clouds.hlsli"

static const uint MaxDynamicLights = 32; // mu::kMaxDynamicLights
// Mirrors mu::DynamicLightsUniform (std140), as in dynamic_lighting.hlsli.
cbuffer DynamicLights : register(b1, space2)
{
    uint dynamicLightCount;
    float dynamicLightWrap;
    uint dynamicWorldLightCount;
    float groundShadowOpacity;
    float4 dynamicLightPositionAndRadius[MaxDynamicLights];
    float4 dynamicLightColor[MaxDynamicLights];
};

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

// Interleaved gradient noise in [0, 1): a stable dither per cell and slice.
float Dither(float2 cell, float slice)
{
    const float2 p = cell + slice * 5.588238;
    return frac(52.9829189 * frac(dot(p, float2(0.06711056, 0.00583715))));
}

// How much sun reaches `position`, 0 to 1, over 3x3 shadow-map texels (scatter.w apart) so shadow
// edges fade between cells instead of stepping; 1 outside the shadow map and without one.
float SunVisibility(float3 position)
{
    if (sun.w <= 0.0)
        return 1.0;
    const float4 clip = mul(sunViewProjection, float4(position, 1.0));
    const float3 ndc = clip.xyz / clip.w;
    const float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0) || ndc.z >= 1.0)
        return 1.0;
    float visible = 0.0;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
            visible += sunShadowMap.SampleCmpLevelZero(sunShadowSampler, uv + float2(x, y) * scatter.w, ndc.z);
    }
    return visible / 9.0;
}

// The point lights' glow at `position`: each light's colour fading smoothly to nothing at its radius.
float3 PointLightGlow(float3 position)
{
    float3 glow = float3(0.0, 0.0, 0.0);
    const uint count = min(dynamicLightCount, MaxDynamicLights);
    [loop]
    for (uint i = 0; i < count; ++i)
    {
        const float4 positionAndRadius = dynamicLightPositionAndRadius[i];
        const float reach = saturate(1.0 - length(positionAndRadius.xyz - position) / positionAndRadius.w);
        glow += dynamicLightColor[i].rgb * (reach * reach);
    }
    return glow;
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
        // The sun is looked up at a dithered point inside the slice, so shadow edges dissolve into a
        // fine grain instead of banding along the slices.
        const float3 sunPoint = camera.xyz + direction * lerp(start, end, Dither(float2(cell.xy), float(z)));
        const float3 lit = color.rgb * (scatter.z + scatter.y * sunPhase * SunVisibility(sunPoint) *
                                                      CloudLight(sunPoint.xy, clouds, cloudOffset.xy)) +
                           medium.w * PointLightGlow(position);
        light += transmittance * lit * (1.0 - kept);
        transmittance *= kept;
        volume[uint3(cell.xy, z)] = float4(light, transmittance);
        start = end;
    }
}
