// skinned_lit.frag.hlsl — Enhanced per-pixel lighting for GPU-skinned BMD meshes.
// Pairs with skinned_textured.vert.hlsl in Enhanced mode (palette.w == 2), which leaves the
// vertex colour unlit and passes the normal, the view vector and the per-object light direction.
//   diffuse  = Classic curve max(dot(n, L) * scale + ambient, floor) * object light, per pixel
//   rim      = strength * (1 - |dot(n, v)|)^power * object light, added after the texture
//   dynamic  = sum of the frame's point lights (Enhanced dynamic lights): colour * linear falloff
//              over the horizontal distance * wrapped diffuse, added to the diffuse light
//   specular = Blinn-Phong, opt-in per item model, scaled by the scene and object light
//   fog      = Enhanced exponential fog (see basic_textured_fog.frag.hlsl)
// The shared uniforms and lighting functions: scene_lighting.hlsli, dynamic_lighting.hlsli.
Texture2D tex : register(t0, space2);
SamplerState s : register(s0, space2);
// The sun shadow map's depth (Enhanced sun shadows; a far 1x1 map without them).
Texture2D sunShadowMap : register(t1, space2);
SamplerComparisonState sunShadowSampler : register(s1, space2);

#include "scene_lighting.hlsli"
#include "dynamic_lighting.hlsli"
#include "clouds.hlsli"

// The light mood's highlight shoulder (Render::Lighting::LightMood::RollOffHighlight): a lit
// colour channel above the knee approaches 1 smoothly instead of being cut off there. Only a knee
// between 0 and 1 rolls off; the callers keep the Classic cut-off expression verbatim otherwise,
// so that the driver compiles the Classic path to the same arithmetic as before.
bool RollsOffHighlight(float knee)
{
    return knee > 0.0 && knee < 1.0;
}

float RollOffChannel(float value, float knee)
{
    const float range = 1.0 - knee;
    return value > knee ? knee + range * (1.0 - exp(-max(value - knee, 0.0) / range)) : value;
}

float3 RollOffHighlight(float3 color, float knee)
{
    return saturate(float3(RollOffChannel(color.r, knee), RollOffChannel(color.g, knee),
                           RollOffChannel(color.b, knee)));
}

struct FSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
    float fogFactor : TEXCOORD2;
    float eyeDepth : TEXCOORD3;
    float3 normal : TEXCOORD4;
    float3 viewVector : TEXCOORD5;
    nointerpolation float4 lightDirectionAndSpecular : TEXCOORD6;
    float4 worldPositionAndDynamicLights : TEXCOORD7; // w: 1 when the point lights apply
    float3 worldNormal : TEXCOORD8;
};

float3 SafeNormalize(float3 value)
{
    const float lengthSquared = dot(value, value);
    return lengthSquared > 1e-12 ? value * rsqrt(lengthSquared) : float3(0.0, 0.0, 0.0);
}

// Point lights reach as far as the terrain light they come from (linear falloff over the
// horizontal distance); the wrapped diffuse keeps MU's soft shading and leaves the far side dark.
float3 DynamicLighting(float3 worldPosition, float3 worldNormal)
{
    float3 light = float3(0.0, 0.0, 0.0);
    const uint count = min(dynamicLightCount, MaxDynamicLights);
    [loop]
    for (uint i = 0; i < count; ++i)
    {
        const float4 positionAndRadius = dynamicLightPositionAndRadius[i];
        const float3 toLight = positionAndRadius.xyz - worldPosition;
        const float falloff = PointLightFalloff(toLight, positionAndRadius.w);
        if (falloff <= 0.0)
            continue;
        const float wrapped =
            saturate(dot(worldNormal, SafeNormalize(toLight)) * (1.0 - dynamicLightWrap) + dynamicLightWrap);
        light += dynamicLightColor[i].rgb * (falloff * wrapped);
    }
    return light;
}

float4 main(FSInput input) : SV_Target
{
    const float3 objectLight = input.color.rgb;
    const float3 normal = SafeNormalize(input.normal);
    const float3 view = SafeNormalize(input.viewVector);
    // Neither the per-object direction nor the skinned normal is unit length in Classic. The
    // diffuse term uses both as they are, so that it is the Classic per-vertex value wherever the
    // curve is not clamped (dot() is linear in the interpolated normal) and keeps the palette.
    const float3 light = input.lightDirectionAndSpecular.xyz;
    const float lightDot = dot(input.normal, light);

    // In the scene's world (not a model drawn for the UI), the point lights and the sun's shadows apply.
    const bool inWorld = input.worldPositionAndDynamicLights.w > 0.5;
    const float3 worldNormal = SafeNormalize(input.worldNormal);
    // The sun's shadow takes away the direct sunlight only: the ambient part and the point lights stay.
    // Clouds dim the sunlight as shadows do.
    const float sunVisible =
        inWorld && sunShadowParameters.x > 0.0
            ? SunVisibility(input.worldPositionAndDynamicLights.xyz, worldNormal) *
                  CloudLight(input.worldPositionAndDynamicLights.xy, cloudParameters, cloudOffset.xy)
            : 1.0;
    const float sunLight = 1.0 - sunShadowParameters.x * (1.0 - sunVisible);
    // Shade turns towards the sky's blue.
    const float3 skyShade = lerp(float3(1.0, 1.0, 1.0), skyFill.rgb, skyFill.w * (1.0 - sunVisible));
    // Faces turned from a low sun or moon keep the sky's light.
    const float diffuseFloor = inWorld ? max(diffuseCurve.z, lightingParameters.z) : diffuseCurve.z;
    // Shadow dims the light a face receives, never the darkening of faces turned away.
    const float luminosity =
        max(lightDot * diffuseCurve.x * (lightDot > 0.0 ? sunLight : 1.0) + diffuseCurve.y, diffuseFloor);
    // All but the direct sunlight is the sky's: its colour on the side the face turns to.
    float direct = 0.0;
    float3 sky = float3(1.0, 1.0, 1.0);
    if (inWorld)
    {
        direct = min(max(lightDot * diffuseCurve.x * sunLight, 0.0), luminosity);
        const float2 sunSide = sunTowards.xy / max(length(sunTowards.xy), 1e-4);
        sky = lerp(skyAway.rgb, skyToward.rgb, saturate(0.5 + 0.5 * dot(worldNormal.xy, sunSide)));
    }
    const float3 surface = objectLight * skyShade;
    const float3 dynamicLight =
        inWorld ? DynamicLighting(input.worldPositionAndDynamicLights.xyz, worldNormal) : float3(0.0, 0.0, 0.0);
    float3 lit = surface * (direct + (luminosity - direct) * sky) + dynamicLight;
    if (inWorld && lightingParameters.w > 0.0)
    {
        const float3 linearSum = ToLinearLight(surface * direct) +
                                 ToLinearLight(surface * (luminosity - direct) * sky) + ToLinearLight(dynamicLight);
        lit = lerp(lit, FromLinearLight(linearSum), lightingParameters.w);
    }
    float3 diffuse = saturate(lit);
    if (RollsOffHighlight(diffuseCurve.w))
        diffuse = RollOffHighlight(lit, diffuseCurve.w);

    // Two-sided meshes show back faces; |dot| keeps their interiors from rim-lighting.
    const float facing = saturate(abs(dot(normal, view)));
    float3 rim = rimParameters.x * pow(1.0 - facing, rimParameters.y) * objectLight;
    if (inWorld && sunTowards.w > 0.0)
    {
        // The rim follows the sun: brighter and sun-coloured on the side facing it, gone in its shadow.
        const float towardsSun = saturate(dot(worldNormal, normalize(sunTowards.xyz)) * 0.75 + 0.25) * sunVisible;
        rim = lerp(rim, rim * sunColor.rgb * (2.0 * towardsSun), sunTowards.w);
    }

    const float specularStrength = input.lightDirectionAndSpecular.w * specularParameters.x;
    float3 specular = float3(0.0, 0.0, 0.0);
    if (specularStrength > 0.0 && lightDot > 0.0)
    {
        const float3 halfVector = SafeNormalize(SafeNormalize(light) + view);
        const float highlight = pow(saturate(dot(normal, halfVector)), specularParameters.y);
        specular = specularStrength * highlight * sunLight * sceneLightColor.rgb * objectLight;
    }

    // Rim and specular are light added on top of the surface, so dark textures still get an
    // outline; both are bounded by the object light.
    const float4 texel = tex.Sample(s, input.uv);
    float4 color = float4(texel.rgb * diffuse + rim + specular, texel.a * input.color.a);
    if (alphaToCoverage)
    {
        // Anti-aliased cutout, as in basic_textured.frag: coverage keeps the
        // texel's translucency; only the alpha test's step becomes a ramp.
        const float edge = saturate((color.a - alphaThreshold) / max(fwidth(color.a), 0.0001) + 0.5);
        color.a *= edge;
        if (color.a <= 0.0) discard;
    }
    else if (alphaDiscardEnabled && color.a <= alphaThreshold) discard;
    if (fogEnabled && sceneFogColor.w > 0.0)
        color.rgb = lerp(color.rgb, sceneFogColor.rgb, EnhancedFogAmount(input.eyeDepth));
    return color;
}
