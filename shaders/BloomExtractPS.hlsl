// Bloom Extract Pixel Shader
// Extracts bright pixels above threshold for bloom effect

cbuffer BloomConstants : register(b0)
{
    float2 texelSize;
    float threshold;  // Brightness threshold
    float padding;
};

Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float Luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float4 main(PSInput input) : SV_TARGET
{
    // Sample HDR color
    float3 color = inputTexture.Sample(linearSampler, input.texCoord).rgb;

    // Calculate luminance
    float lum = Luminance(color);

    // Soft threshold with knee
    float knee = threshold * 0.5;
    float soft = lum - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);

    // Contribution based on brightness
    float contribution = max(soft, lum - threshold) / max(lum, 0.00001);

    // Extract bright pixels
    float3 bloomColor = color * contribution;

    return float4(bloomColor, 1.0);
}
