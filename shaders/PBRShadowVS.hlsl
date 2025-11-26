// PBR Vertex Shader with Shadow Map support
// Same as PBRNormalVS but kept separate for clarity

cbuffer MVPConstants : register(b0)
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
    float3 worldPos : WORLD_POS;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Transform position
    float4 worldPos = mul(float4(input.position, 1.0f), model);
    output.worldPos = worldPos.xyz;

    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    // Pass through texture coordinates
    output.texCoord = input.texCoord;

    // Transform normal to world space
    float3x3 normalMatrix = (float3x3)model;
    output.normal = normalize(mul(input.normal, normalMatrix));
    output.tangent = normalize(mul(input.tangent, normalMatrix));

    // Calculate bitangent
    output.bitangent = cross(output.normal, output.tangent);

    return output;
}
