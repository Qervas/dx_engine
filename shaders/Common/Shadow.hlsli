// Shadow Calculation Functions
#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

// PCF shadow sampling
float CalculateShadowPCF(
    Texture2D shadowMap,
    SamplerComparisonState shadowSampler,
    float3 worldPos,
    float4x4 lightViewProj,
    float2 shadowMapSize)
{
    float4 lightSpacePos = mul(float4(worldPos, 1.0), lightViewProj);
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Transform to UV space
    float2 shadowUV;
    shadowUV.x = projCoords.x * 0.5 + 0.5;
    shadowUV.y = -projCoords.y * 0.5 + 0.5;

    float currentDepth = projCoords.z;

    // Check bounds
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0 ||
        currentDepth > 1.0)
    {
        return 1.0;
    }

    // PCF filtering
    float shadow = 0.0;
    float2 texelSize = 1.0 / shadowMapSize;

    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV + offset, currentDepth);
        }
    }

    return shadow / 9.0;
}

// Simple shadow sampling (no PCF)
float CalculateShadowSimple(
    Texture2D shadowMap,
    SamplerComparisonState shadowSampler,
    float3 worldPos,
    float4x4 lightViewProj)
{
    float4 lightSpacePos = mul(float4(worldPos, 1.0), lightViewProj);
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    float2 shadowUV;
    shadowUV.x = projCoords.x * 0.5 + 0.5;
    shadowUV.y = -projCoords.y * 0.5 + 0.5;

    float currentDepth = projCoords.z;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0 ||
        currentDepth > 1.0)
    {
        return 1.0;
    }

    return shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, currentDepth);
}

#endif // SHADOW_HLSLI
