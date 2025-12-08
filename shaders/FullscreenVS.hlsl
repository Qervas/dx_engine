// Fullscreen Triangle Vertex Shader
// Generates a fullscreen triangle without vertex buffer

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate fullscreen triangle vertices
    // Vertex 0: (-1, -1) -> UV (0, 1)
    // Vertex 1: (-1,  3) -> UV (0, -1)
    // Vertex 2: ( 3, -1) -> UV (2, 1)
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    return output;
}
