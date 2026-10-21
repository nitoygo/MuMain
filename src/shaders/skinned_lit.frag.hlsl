// skinned_lit.frag.hlsl — Enhanced per-pixel lighting for GPU-skinned BMD meshes.
// Pairs with skinned_textured.vert.hlsl in Enhanced mode (palette.w == 2), which leaves the
// vertex colour unlit and passes the normal, the view vector and the per-object light direction.
//   diffuse  = Classic curve max(dot(n, L) * scale + ambient, floor) * object light, per pixel
//   rim      = strength * (1 - |dot(n, v)|)^power * object light, added after the texture
//   dynamic  = sum of the frame's point lights (Enhanced dynamic lights): colour * linear falloff
//              over the horizontal distance * wrapped diffuse, added to the diffuse light
//   specular = Blinn-Phong, opt-in per item model, scaled by the scene and object light
//   fog      = Enhanced exponential fog (see basic_textured_fog.frag.hlsl)
// FogUniforms mirrors FogUniform, SceneLighting mirrors mu::SceneLightingUniform and DynamicLights
// mirrors mu::DynamicLightsUniform (std140).
Texture2D tex : register(t0, space2);
SamplerState s : register(s0, space2);
// The sun shadow map's depth (Enhanced sun shadows; a far 1x1 map without them).
Texture2D sunShadowMap : register(t1, space2);
SamplerComparisonState sunShadowSampler : register(s1, space2);
cbuffer FogUniforms : register(b0, space3)
{
    uint fogEnabled;
    uint alphaDiscardEnabled;
    float alphaThreshold;
    uint alphaToCoverage; // 1: the pipeline turns alpha into multisample coverage
    float fogStart;
    float fogEnd;
    float4 fogColor;
};
cbuffer SceneLighting : register(b1, space3)
{
    float4 sceneLightDirection;
    float4 sceneLightColor;
    float4 diffuseCurve; // scale, ambient, floor, highlight knee
    float4 rimParameters;
    float4 specularParameters;
    float4 sceneFogColor;
    float4 sceneFogParameters;
    float4 cameraPosition;
    float4x4 sunViewProjection; // world to the sun shadow map's clip space
    float4 sunShadowParameters; // darkness (0: no shadow map), softness in texels, 1 / map size, texel in world units
    float4 lightingParameters;  // x: point lights' physical falloff (0 linear, 1 inverse-square)
};
static const uint MaxDynamicLights = 32; // mu::kMaxDynamicLights
cbuffer DynamicLights : register(b2, space3)
{
    uint dynamicLightCount;   // lights used
    float dynamicLightWrap;   // 0: Lambert, 0.5: half-Lambert (mu::WrappedDiffuse)
    uint2 dynamicLightPadding;
    float4 dynamicLightPositionAndRadius[MaxDynamicLights];
    float4 dynamicLightColor[MaxDynamicLights];
};

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

// Same curve as basic_textured_fog.frag.hlsl and mu::EnhancedFogAmount() (SceneLighting.cpp).
float EnhancedFogAmount(float eyeDepth)
{
    const float scaledDistance = max(eyeDepth - sceneFogParameters.x, 0.0) * sceneFogParameters.z;
    return saturate(1.0 - exp(-scaledDistance * scaledDistance));
}

float3 SafeNormalize(float3 value)
{
    const float lengthSquared = dot(value, value);
    return lengthSquared > 1e-12 ? value * rsqrt(lengthSquared) : float3(0.0, 0.0, 0.0);
}

// How much sun reaches `worldPosition` (Enhanced sun shadows): 1 lit, 0 in the shadow of what stands
// between it and the sun, softened over sunShadowParameters.y texels. 1 without a shadow map or
// outside it. `worldNormal` pushes the lookup off the surface so that it does not shadow itself.
float SunVisibility(float3 worldPosition, float3 worldNormal)
{
    if (sunShadowParameters.x <= 0.0)
        return 1.0;
    const float3 lifted = worldPosition + worldNormal * (1.5 * sunShadowParameters.w);
    const float4 clip = mul(sunViewProjection, float4(lifted, 1.0));
    const float3 ndc = clip.xyz / clip.w;
    const float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0) || ndc.z >= 1.0)
        return 1.0;
    const float2 step = sunShadowParameters.zz * sunShadowParameters.y;
    float visible = 0.0;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
            visible += sunShadowMap.SampleCmpLevelZero(sunShadowSampler, uv + float2(x, y) * step, ndc.z);
    }
    return visible / 9.0;
}

// How much of a point light reaches a point `toLight` away from it: the terrain light's linear fade
// over the horizontal distance, blended by lightingParameters.x towards inverse-square over the full
// distance, which ends smoothly at the radius (mu::PointLightFalloff).
float PointLightFalloff(float3 toLight, float radius)
{
    const float horizontal = length(toLight.xy);
    if (horizontal >= radius)
        return 0.0;
    const float linearFade = (radius - horizontal) / radius;
    if (lightingParameters.x <= 0.0)
        return linearFade;
    const float distanceSquared = dot(toLight, toLight);
    const float core = 0.4 * radius;
    const float ratio = distanceSquared / (radius * radius);
    const float window = saturate(1.0 - ratio * ratio);
    const float inverseSquare = core * core / (distanceSquared + core * core) * window * window;
    return lerp(linearFade, inverseSquare, lightingParameters.x);
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
    const float sunLight = inWorld ? 1.0 - sunShadowParameters.x *
                                               (1.0 - SunVisibility(input.worldPositionAndDynamicLights.xyz, worldNormal))
                                   : 1.0;
    const float luminosity = max(lightDot * diffuseCurve.x * sunLight + diffuseCurve.y, diffuseCurve.z);
    const float3 dynamicLight =
        inWorld ? DynamicLighting(input.worldPositionAndDynamicLights.xyz, worldNormal) : float3(0.0, 0.0, 0.0);
    float3 diffuse = saturate(objectLight * luminosity + dynamicLight);
    if (RollsOffHighlight(diffuseCurve.w))
        diffuse = RollOffHighlight(objectLight * luminosity + dynamicLight, diffuseCurve.w);

    // Two-sided meshes show back faces; |dot| keeps their interiors from rim-lighting.
    const float facing = saturate(abs(dot(normal, view)));
    const float3 rim = rimParameters.x * pow(1.0 - facing, rimParameters.y) * objectLight;

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
