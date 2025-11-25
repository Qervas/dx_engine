// PBR Vertex Shader with Normal Mapping Support

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
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POS;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Transform to world space
    float4 worldPos = mul(float4(input.position, 1.0f), model);
    output.worldPos = worldPos.xyz;

    // Transform to clip space
    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    // Pass through texture coordinates
    output.texCoord = input.texCoord;

    // Transform normal and tangent to world space
    output.normal = normalize(mul(input.normal, (float3x3)model));
    output.tangent = normalize(mul(input.tangent, (float3x3)model));

    // Calculate bitangent (cross product of normal and tangent)
    output.bitangent = cross(output.normal, output.tangent);

    return output;
}
