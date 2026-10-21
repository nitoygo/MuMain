// shaft_occluder.frag.hlsl — a model into the light-shaft occluder mask (R8): 1 where its opaque
// texels cover the pixel, so post_shafts.frag leaves a dark streak behind it. Pairs with
// skinned_textured.vert.
Texture2D tex : register(t0, space2);
SamplerState s : register(s0, space2);
struct FSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 main(FSInput input) : SV_Target
{
    // Cut-out texels (hair, capes, leaves) let the light through, as the scene's alpha test does.
    if (tex.Sample(s, input.uv).a * input.color.a < 0.25)
        discard;
    return float4(1.0, 1.0, 1.0, 1.0);
}
