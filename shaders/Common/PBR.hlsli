// PBR Lighting Functions
#ifndef PBR_HLSLI
#define PBR_HLSLI

#include "Math.hlsli"
#include "Constants.hlsli"

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

// Geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

// Geometry function (Smith's method)
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel (Schlick approximation)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * Pow5(saturate(1.0 - cosTheta));
}

// Fresnel with roughness (for environment reflections)
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 maxReflectivity = max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0);
    return F0 + (maxReflectivity - F0) * Pow5(saturate(1.0 - cosTheta));
}

// Calculate point light contribution
float3 CalculatePointLight(
    LightData light,
    float3 N,
    float3 V,
    float3 worldPos,
    float3 F0,
    float3 albedoColor,
    float metallicValue,
    float roughnessValue)
{
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
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    float3 specular = numerator / denominator;

    // Energy conservation
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallicValue);

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedoColor / PI + specular) * radiance * NdotL;
}

// Calculate directional light contribution
float3 CalculateDirectionalLight(
    LightData light,
    float3 N,
    float3 V,
    float3 F0,
    float3 albedoColor,
    float metallicValue,
    float roughnessValue,
    float shadow)
{
    float3 L = normalize(-light.direction);
    float3 radiance = light.color * light.intensity;

    // Cook-Torrance BRDF
    float3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    float3 specular = numerator / denominator;

    // Energy conservation
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallicValue);

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedoColor / PI + specular) * radiance * NdotL * shadow;
}

// Calculate base reflectivity (F0)
float3 CalculateF0(float3 albedo, float metallic)
{
    float3 dielectricF0 = float3(0.04, 0.04, 0.04);
    return lerp(dielectricF0, albedo, metallic);
}

#endif // PBR_HLSLI
