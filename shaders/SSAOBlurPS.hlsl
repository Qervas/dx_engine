// SSAO Blur Pixel Shader
// Simple box blur to smooth SSAO results

cbuffer BlurConstants : register(b0)
{
    float2 texelSize;
    float2 padding;
};

Texture2D ssaoTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float main(PSInput input) : SV_TARGET
{
    float result = 0.0f;

    // 4x4 box blur
    [unroll]
    for (int x = -2; x < 2; ++x)
    {
        [unroll]
        for (int y = -2; y < 2; ++y)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += ssaoTexture.Sample(linearSampler, input.texCoord + offset).r;
        }
    }

    return result / 16.0f;
}
