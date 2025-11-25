// PBR Pixel Shader with Normal Mapping

static const float PI = 3.14159265359;

// Material properties
cbuffer MaterialCB : register(b1)
{
    float3 albedo;
    float metallic;

    float roughness;
    float ao;
    float useNormalMap;  // 1.0 if normal map should be used, 0.0 otherwise
    float _padding;
};

// Scene lighting
cbuffer SceneLightingCB : register(b2)
{
    float3 cameraPosition;
    uint lightCount;

    // Light data (up to 4 lights)
    struct LightData
    {
        float3 position;
        uint type;          // 0=Directional, 1=Point, 2=Spot

        float3 direction;
        float range;

        float3 color;
        float intensity;

        float spotAngle;
        float3 _padding2;
    } lights[4];
};

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
SamplerState linearSampler : register(s0);

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

float3 GetNormalFromMap(PSInput input)
{
    // Sample the normal map
    float3 tangentNormal = normalTexture.Sample(linearSampler, input.texCoord).xyz;

    // Transform from [0,1] to [-1,1]
    tangentNormal = tangentNormal * 2.0 - 1.0;

    // Construct TBN matrix
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    float3x3 TBN = float3x3(T, B, N);

    // Transform tangent space normal to world space
    return normalize(mul(tangentNormal, TBN));
}

float3 CalculatePointLight(LightData light, float3 N, float3 V, float3 worldPos, float3 F0, float3 albedoColor, float metallicValue, float roughnessValue)
{
    // Calculate light direction and distance
    float3 L = normalize(light.position - worldPos);
    float distance = length(light.position - worldPos);

    // Attenuation
    float attenuation = 1.0 / (distance * distance);
    attenuation *= saturate(1.0 - (distance / light.range));

    float3 radiance = light.color * light.intensity * attenuation;

    // Cook-Torrance BRDF
    float3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    // Energy conservation
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallicValue;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedoColor / PI + specular) * radiance * NdotL;
}

float3 CalculateDirectionalLight(LightData light, float3 N, float3 V, float3 F0, float3 albedoColor, float metallicValue, float roughnessValue)
{
    float3 L = normalize(-light.direction);
    float3 radiance = light.color * light.intensity;

    // Cook-Torrance BRDF
    float3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    // Energy conservation
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallicValue;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedoColor / PI + specular) * radiance * NdotL;
}

float4 main(PSInput input) : SV_TARGET
{
    // DEBUG: Output just the albedo texture to see if textures are working
    // return float4(albedoTexture.Sample(linearSampler, input.texCoord).rgb, 1.0);

    // DEBUG: Output the normal map to see if it's being read correctly
    // return float4(normalTexture.Sample(linearSampler, input.texCoord).rgb, 1.0);

    // Sample albedo texture
    float3 albedoColor = albedoTexture.Sample(linearSampler, input.texCoord).rgb * albedo;

    // Get normal (either from normal map or vertex normal)
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

    // Calculate reflectance at normal incidence
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedoColor, metallic);

    // Lighting calculation
    float3 Lo = float3(0.0, 0.0, 0.0);

    for (uint i = 0; i < lightCount; ++i)
    {
        if (lights[i].type == 0) // Directional
        {
            Lo += CalculateDirectionalLight(lights[i], N, V, F0, albedoColor, metallic, roughness);
        }
        else if (lights[i].type == 1) // Point
        {
            Lo += CalculatePointLight(lights[i], N, V, input.worldPos, F0, albedoColor, metallic, roughness);
        }
    }

    // Ambient lighting
    float3 ambient = float3(0.03, 0.03, 0.03) * albedoColor * ao;

    float3 color = ambient + Lo;

    // HDR tonemapping (Reinhard)
    color = color / (color + float3(1.0, 1.0, 1.0));

    // Gamma correction
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(color, 1.0);
}
