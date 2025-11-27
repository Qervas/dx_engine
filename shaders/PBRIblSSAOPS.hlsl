// PBR Pixel Shader with Image-Based Lighting (IBL) and SSAO
// Uses split-sum approximation for specular IBL
// SSAO modulates ambient lighting for realistic occlusion

static const float PI = 3.14159265359;

// Material properties
cbuffer MaterialCB : register(b1)
{
    float3 albedo;
    float metallic;

    float roughness;
    float ao;
    float useNormalMap;
    float _padding;
};

// Scene lighting
cbuffer SceneLightingCB : register(b2)
{
    float3 cameraPosition;
    uint lightCount;

    struct LightData
    {
        float3 position;
        uint type;

        float3 direction;
        float range;

        float3 color;
        float intensity;

        float spotAngle;
        float3 _padding2;
    } lights[4];
};

// Shadow constants
cbuffer ShadowCB : register(b3)
{
    float4x4 lightViewProj;
};

// IBL constants
cbuffer IBLCB : register(b4)
{
    uint prefilteredMipLevels;
    float3 _iblPadding;
};

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D shadowMap : register(t2);
TextureCube environmentMap : register(t3);
TextureCube irradianceMap : register(t4);
TextureCube prefilteredMap : register(t5);
Texture2D brdfLUT : register(t6);
Texture2D ssaoTexture : register(t7);

SamplerState linearSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POS;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// PBR Functions
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Fresnel with roughness for environment reflections
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 GetNormalFromMap(PSInput input)
{
    float3 tangentNormal = normalTexture.Sample(linearSampler, input.texCoord).xyz;
    tangentNormal = tangentNormal * 2.0 - 1.0;

    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

// Shadow calculation with PCF
float CalculateShadow(float3 worldPos)
{
    float4 lightSpacePos = mul(float4(worldPos, 1.0), lightViewProj);
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    float2 shadowUV;
    shadowUV.x = projCoords.x * 0.5 + 0.5;
    shadowUV.y = -projCoords.y * 0.5 + 0.5;

    float currentDepth = projCoords.z;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0 || currentDepth > 1.0)
    {
        return 1.0;
    }

    float shadow = 0.0;
    float2 texelSize = float2(1.0 / 2048.0, 1.0 / 2048.0);

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

    shadow /= 9.0;
    return shadow;
}

float3 CalculatePointLight(LightData light, float3 N, float3 V, float3 worldPos, float3 F0, float3 albedoColor, float metallicValue, float roughnessValue)
{
    float3 L = normalize(light.position - worldPos);
    float distance = length(light.position - worldPos);

    float attenuation = 1.0 / (distance * distance);
    attenuation *= saturate(1.0 - (distance / light.range));

    float3 radiance = light.color * light.intensity * attenuation;

    float3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallicValue;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedoColor / PI + specular) * radiance * NdotL;
}

float3 CalculateDirectionalLight(LightData light, float3 N, float3 V, float3 F0, float3 albedoColor, float metallicValue, float roughnessValue, float shadow)
{
    float3 L = normalize(-light.direction);
    float3 radiance = light.color * light.intensity;

    float3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallicValue;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedoColor / PI + specular) * radiance * NdotL * shadow;
}

// Image-Based Lighting (IBL) ambient calculation with SSAO
float3 CalculateIBLAmbient(float3 N, float3 V, float3 F0, float3 albedoColor, float metallicValue, float roughnessValue, float aoValue, float ssaoValue)
{
    float NdotV = max(dot(N, V), 0.0);

    // Fresnel with roughness
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughnessValue);

    // Diffuse and specular weights
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallicValue;

    // Diffuse IBL from irradiance map
    float3 irradiance = irradianceMap.Sample(linearSampler, N).rgb;
    float3 diffuse = irradiance * albedoColor;

    // Specular IBL using split-sum approximation
    float3 R = reflect(-V, N);

    // Sample prefiltered environment map at roughness-based mip level
    float mipLevel = roughnessValue * float(prefilteredMipLevels - 1);
    float3 prefilteredColor = prefilteredMap.SampleLevel(linearSampler, R, mipLevel).rgb;

    // Sample BRDF LUT
    float2 brdf = brdfLUT.Sample(linearSampler, float2(NdotV, roughnessValue)).rg;

    // Combine specular
    float3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    // Combined AO: material AO * SSAO
    float combinedAO = aoValue * ssaoValue;

    // Final ambient with combined AO
    float3 ambient = (kD * diffuse + specular) * combinedAO;

    return ambient;
}

float4 main(PSInput input) : SV_TARGET
{
    float3 albedoColor = albedoTexture.Sample(linearSampler, input.texCoord).rgb * albedo;

    float3 N;
    if (useNormalMap > 0.5)
    {
        N = GetNormalFromMap(input);
    }
    else
    {
        N = normalize(input.normal);
    }

    float3 V = normalize(cameraPosition - input.worldPos);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedoColor, metallic);

    // Calculate shadow
    float shadow = CalculateShadow(input.worldPos);

    // Sample SSAO from screen-space texture
    // Convert screen position to UV coordinates
    float2 screenUV = input.position.xy / float2(1280.0, 720.0);  // TODO: Pass screen dimensions
    float ssaoValue = ssaoTexture.Sample(linearSampler, screenUV).r;

    // Direct lighting
    float3 Lo = float3(0.0, 0.0, 0.0);

    for (uint i = 0; i < lightCount; ++i)
    {
        if (lights[i].type == 0) // Directional
        {
            Lo += CalculateDirectionalLight(lights[i], N, V, F0, albedoColor, metallic, roughness, (i == 0) ? shadow : 1.0);
        }
        else if (lights[i].type == 1) // Point
        {
            Lo += CalculatePointLight(lights[i], N, V, input.worldPos, F0, albedoColor, metallic, roughness);
        }
    }

    // IBL ambient lighting with SSAO
    float3 ambient = CalculateIBLAmbient(N, V, F0, albedoColor, metallic, roughness, ao, ssaoValue);

    float3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + float3(1.0, 1.0, 1.0));

    // Gamma correction
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(color, 1.0);
}
