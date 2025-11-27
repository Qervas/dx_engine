// SSAO Fullscreen Triangle Vertex Shader

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate fullscreen triangle from vertex ID
    // Vertex 0: (-1, -1), Vertex 1: (3, -1), Vertex 2: (-1, 3)
    float2 texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(texCoord * 2.0f - 1.0f, 0.0f, 1.0f);
    output.position.y = -output.position.y;  // Flip Y for DirectX
    output.texCoord = texCoord;

    return output;
}
