// Textured Vertex Shader with MVP

cbuffer MVP : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Transform position through MVP matrices
    float4 worldPos = mul(float4(input.position, 1.0f), model);
    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    // Pass through texture coordinates
    output.texCoord = input.texCoord;

    return output;
}
