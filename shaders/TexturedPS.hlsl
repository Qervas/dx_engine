// Textured Pixel Shader

Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    // Sample the texture
    float4 color = albedoTexture.Sample(linearSampler, input.texCoord);
    return color;
}
