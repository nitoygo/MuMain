// post_shafts.frag.hlsl — light shafts at half resolution: the air glows around each shaft light
// (a segment on screen, a point when its ends meet), and each pixel gathers that glow on its way
// towards the light. Where a model (the occluder mask) stands in the way, the glow behind it is
// missing: a darker streak runs from the model away from the light, through the haze. rgb: the glow
// left after the models, which the composite adds; the air only adds light, it never darkens.
// LightShafts mirrors mu::SdlGpuLightShaftsUniform.
Texture2D occluders : register(t0, space2);
SamplerState occluderSampler : register(s0, space2); // linear: the mask is full resolution
cbuffer LightShafts : register(b0, space3)
{
    float4 segments[4];  // xy: start, zw: end, in UV (y down)
    float4 colors[4];    // rgb: colour, a: unused
    uint shaftCount;
    float aspect;        // width / height: distances are measured in screen heights
    float glowWidth;     // the glow's width, in screen heights
    float decay;         // weight kept per step towards the light
};
struct FSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

static const uint StepCount = 40;
// How thick the air is: the share of a light's glow it scatters towards the camera.
static const float AirDensity = 0.25;

// The nearest point to `uv` on the shaft's segment, with distances in screen heights.
float2 NearestPoint(float4 segment, float2 uv)
{
    const float2 along = (segment.zw - segment.xy) * float2(aspect, 1.0);
    const float2 offset = (uv - segment.xy) * float2(aspect, 1.0);
    const float lengthSquared = dot(along, along);
    const float t = lengthSquared > 0.0 ? saturate(dot(offset, along) / lengthSquared) : 0.0;
    return segment.xy + (segment.zw - segment.xy) * t;
}

// The air's glow at `uv` from one shaft, before the models cut it.
float Glow(float4 segment, float2 uv)
{
    const float2 gap = (uv - NearestPoint(segment, uv)) * float2(aspect, 1.0);
    return exp(-dot(gap, gap) / (glowWidth * glowWidth));
}

// Interleaved gradient noise in [0, 1): a different start for each pixel's march, so the steps of
// neighbouring pixels do not line up into stripes.
float MarchJitter(float2 pixel)
{
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}

float4 main(FSInput input) : SV_Target
{
    const float jitter = MarchJitter(input.pos.xy);
    float3 light = 0.0;
    [loop]
    for (uint i = 0; i < shaftCount; ++i)
    {
        // March from the pixel to the light, adding the glow the models leave uncovered.
        const float2 target = NearestPoint(segments[i], input.uv);
        const float2 step = (target - input.uv) / StepCount;
        float2 uv = input.uv + step * jitter;
        float weight = 1.0;
        float gathered = 0.0;
        float total = 0.0;
        [loop]
        for (uint s = 0; s < StepCount; ++s)
        {
            const float glow = Glow(segments[i], uv) * weight;
            gathered += glow * (1.0 - occluders.SampleLevel(occluderSampler, uv, 0).r);
            total += weight;
            weight *= decay;
            uv += step;
        }
        light += colors[i].rgb * (AirDensity * gathered / total);
    }
    return float4(light, 0.0);
}
