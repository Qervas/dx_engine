// Skybox Vertex Shader
// Uses fullscreen triangle with ray direction calculation

cbuffer SkyboxConstants : register(b0)
{
    float4x4 inverseViewProjection;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldDir : TEXCOORD0;
};

// Generate fullscreen triangle from vertex ID (0, 1, 2)
// This covers the entire screen without needing a vertex buffer
VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate fullscreen triangle vertices
    // VertexID 0: (-1, -1) -> bottom-left
    // VertexID 1: (3, -1)  -> far right (off screen)
    // VertexID 2: (-1, 3)  -> far top (off screen)
    float2 texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    float4 clipPos = float4(texCoord * 2.0f - 1.0f, 1.0f, 1.0f);

    // Flip Y for D3D coordinate system
    clipPos.y = -clipPos.y;

    // Output position at far plane (depth = 1)
    output.position = float4(clipPos.xy, 1.0f, 1.0f);

    // Calculate world direction by unprojecting
    float4 worldPos = mul(clipPos, inverseViewProjection);
    output.worldDir = worldPos.xyz / worldPos.w;

    return output;
}
