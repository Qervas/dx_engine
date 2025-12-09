// Texture Sampling Utilities
#ifndef SAMPLING_HLSLI
#define SAMPLING_HLSLI

// Get normal from normal map using TBN matrix
float3 GetNormalFromMap(
    Texture2D normalMap,
    SamplerState samp,
    float2 texCoord,
    float3 normal,
    float3 tangent,
    float3 bitangent)
{
    float3 tangentNormal = normalMap.Sample(samp, texCoord).xyz;
    tangentNormal = tangentNormal * 2.0 - 1.0;

    float3 N = normalize(normal);
    float3 T = normalize(tangent);
    float3 B = normalize(bitangent);
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

// Fullscreen triangle vertex generation
// Use SV_VertexID to generate a fullscreen triangle without vertex buffer
void GenerateFullscreenTriangle(uint vertexID, out float4 position, out float2 texCoord)
{
    // Generate vertices for a fullscreen triangle
    // Vertex 0: (-1, -1), Vertex 1: (3, -1), Vertex 2: (-1, 3)
    texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    position = float4(texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

#endif // SAMPLING_HLSLI
