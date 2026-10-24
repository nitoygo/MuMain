// fog_volume.comp.hlsl — the fog along each column of the volume (one screen cell), front to back: each
// slice adds the light its fog scatters towards the camera, dimmed by the fog in front of it. Writes
// rgb: the light gathered up to the slice's far end, a: the share of what lies behind that shows.
#include "fog_common.hlsli"

RWTexture3D<float4> volume : register(u0, space1);

[numthreads(8, 8, 1)]
void main(uint3 cell : SV_DispatchThreadID)
{
    if (any(cell.xy >= size.xy))
        return;
    const float2 uv = (float2(cell.xy) + 0.5) / float2(size.xy);
    const float3 direction = normalize(WorldAt(uv, 1.0) - camera.xyz);
    const float growth = SliceGrowth();
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
        light += transmittance * color.rgb * (1.0 - kept);
        transmittance *= kept;
        volume[uint3(cell.xy, z)] = float4(light, transmittance);
        start = end;
    }
}
