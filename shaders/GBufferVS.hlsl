// G-Buffer Vertex Shader
// Outputs view-space position and normal for SSAO

cbuffer PerObjectCB : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
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
    float3 viewPos : VIEW_POS;
    float3 viewNormal : VIEW_NORMAL;
    float2 texCoord : TEXCOORD;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // World space position
    float4 worldPos = mul(float4(input.position, 1.0f), model);

    // View space position
    float4 viewPos = mul(worldPos, view);
    output.viewPos = viewPos.xyz;

    // Clip space position
    output.position = mul(viewPos, projection);

    // View space normal (using normal matrix)
    float3x3 normalMatrix = (float3x3)mul(model, view);
    output.viewNormal = normalize(mul(input.normal, normalMatrix));

    output.texCoord = input.texCoord;

    return output;
}
