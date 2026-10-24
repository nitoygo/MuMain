// fog_volume.comp.hlsl — fills the fog volume, one thread per cell (froxel): x, y across the screen,
// z along the view depth. FogVolume mirrors mu::SdlGpuFogVolumeUniform.
RWTexture3D<float4> volume : register(u0, space1);
cbuffer FogVolume : register(b0, space2)
{
    uint4 size; // xyz: cells, w: unused
};

[numthreads(4, 4, 4)]
void main(uint3 cell : SV_DispatchThreadID)
{
    if (any(cell >= size.xyz))
        return;
    volume[cell] = float4((float3(cell) + 0.5) / float3(size.xyz), 1.0);
}
