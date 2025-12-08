// Bloom Blur Pixel Shader
// Gaussian blur for bloom effect (separable: horizontal then vertical)

cbuffer BloomBlurConstants : register(b0)
{
    float2 texelSize;
    float direction;  // 0 = horizontal, 1 = vertical
    float padding;
};

Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

// Gaussian weights for 9-tap blur
static const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

float4 main(PSInput input) : SV_TARGET
{
    float2 offset;
    if (direction < 0.5)
    {
        // Horizontal blur
        offset = float2(texelSize.x, 0.0);
    }
    else
    {
        // Vertical blur
        offset = float2(0.0, texelSize.y);
    }

    // Center sample
    float3 result = inputTexture.Sample(linearSampler, input.texCoord).rgb * weights[0];

    // Symmetric samples
    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 sampleOffset = offset * float(i);
        result += inputTexture.Sample(linearSampler, input.texCoord + sampleOffset).rgb * weights[i];
        result += inputTexture.Sample(linearSampler, input.texCoord - sampleOffset).rgb * weights[i];
    }

    return float4(result, 1.0);
}
