// G-Buffer Pixel Shader
// Outputs view-space position and normal to render targets

struct PSInput
{
    float4 position : SV_POSITION;
    float3 viewPos : VIEW_POS;
    float3 viewNormal : VIEW_NORMAL;
    float2 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 position : SV_TARGET0;  // View-space position (RGB) + depth (A)
    float4 normal : SV_TARGET1;    // View-space normal (RGB)
};

PSOutput main(PSInput input)
{
    PSOutput output;

    // Output view-space position with linear depth in alpha
    output.position = float4(input.viewPos, 1.0f);

    // Output view-space normal (normalized)
    output.normal = float4(normalize(input.viewNormal), 1.0f);

    return output;
}
