// fog_common.hlsli — the volumetric fog's uniforms, shared by fog_volume.comp and fog_apply.comp.
// FogUniforms mirrors mu::SdlGpuFogUniform. The volume's slices lie at view distances growing
// exponentially from `camera.w` (the first slice's start) to `color.w` (the last one's end).
#ifndef MU_FOG_COMMON_HLSLI
#define MU_FOG_COMMON_HLSLI
cbuffer FogUniforms : register(b0, space2)
{
    float4x4 inverseViewProjection; // clip space to world
    float4 camera;                  // xyz: the camera, w: nearest slice distance
    float4 color;                   // rgb: the fog's light (the map's fog colour), w: farthest slice distance
    float4 medium;                  // x: extinction per world unit at the base, y: 1 / fog.height, z: base height
    uint4 size;                     // xyz: the volume's cells, w: unused
    uint4 target;                   // xy: the fog layer's pixels
};

// The world position seen at `uv` (y down) at clip depth `depth` (0 near, 1 far).
float3 WorldAt(float2 uv, float depth)
{
    const float4 world = mul(inverseViewProjection, float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0));
    return world.xyz / world.w;
}

// log(far / near): how fast the slices grow.
float SliceGrowth()
{
    return log(color.w / camera.w);
}
#endif
