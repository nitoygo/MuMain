// shaft_occluder.frag.hlsl — a model into the light-shaft occluder mask (R8): how much of the pixel
// its texels cover, so post_shafts.frag leaves a dark streak behind it; the mask keeps the most
// (MAX blend). Pairs with skinned_textured.vert.
// FogUniforms mirrors FogUniform, whose fogEnabled slot carries the coverage mode (SunCoverage - 1):
// solid models cover fully where their texels pass the alpha test (hair, capes and leaves let the
// light through their cut-outs); a character's blended parts (wings, capes, auras) cover by their
// alpha, or glowing, their brightness.
Texture2D tex : register(t0, space2);
SamplerState s : register(s0, space2);
cbuffer FogUniforms : register(b0, space3)
{
    uint coverageMode; // 0 solid, 1 by alpha, 2 by brightness
    uint alphaDiscardEnabled;
    float alphaThreshold;
    uint alphaToCoverage;
    float fogStart;
    float fogEnd;
    float4 fogColor;
};
struct FSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 main(FSInput input) : SV_Target
{
    const float4 texel = tex.Sample(s, input.uv) * input.color;
    if (coverageMode == 0)
    {
        if (texel.a < 0.25)
            discard;
        return float4(1.0, 1.0, 1.0, 1.0);
    }
    const float coverage = coverageMode == 1 ? texel.a : max(texel.r, max(texel.g, texel.b)) * texel.a;
    return float4(coverage, coverage, coverage, coverage);
}
