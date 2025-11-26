// Shadow Map Vertex Shader
// Transforms vertices to light space for depth rendering

cbuffer ShadowConstants : register(b0)
{
    float4x4 lightViewProj;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Transform to light clip space
    output.position = mul(float4(input.position, 1.0f), lightViewProj);

    return output;
}
