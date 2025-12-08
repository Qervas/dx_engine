// Tone Mapping Pixel Shader
// Converts HDR to LDR with various tone mapping operators

cbuffer PostProcessConstants : register(b0)
{
    float exposure;
    float gamma;
    uint toneMapMode;
    float bloomIntensity;
    float bloomThreshold;
    float3 padding;
};

Texture2D hdrTexture : register(t0);
Texture2D bloomTexture : register(t1);
SamplerState linearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

// Reinhard tone mapping
float3 ReinhardToneMap(float3 hdr)
{
    return hdr / (hdr + float3(1.0, 1.0, 1.0));
}

// ACES Filmic tone mapping
// From: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Uncharted 2 tone mapping
float3 Uncharted2ToneMap(float3 x)
{
    float A = 0.15;  // Shoulder Strength
    float B = 0.50;  // Linear Strength
    float C = 0.10;  // Linear Angle
    float D = 0.20;  // Toe Strength
    float E = 0.02;  // Toe Numerator
    float F = 0.30;  // Toe Denominator

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 Uncharted2(float3 hdr)
{
    float W = 11.2;  // Linear White Point Value
    float exposureBias = 2.0;

    float3 curr = Uncharted2ToneMap(exposureBias * hdr);
    float3 whiteScale = 1.0 / Uncharted2ToneMap(float3(W, W, W));

    return curr * whiteScale;
}

float4 main(PSInput input) : SV_TARGET
{
    // Sample HDR scene
    float3 hdrColor = hdrTexture.Sample(linearSampler, input.texCoord).rgb;

    // Sample and add bloom
    float3 bloom = bloomTexture.Sample(linearSampler, input.texCoord).rgb;
    hdrColor += bloom * bloomIntensity;

    // Apply exposure
    hdrColor *= exposure;

    // Apply tone mapping
    float3 ldrColor;

    switch (toneMapMode)
    {
        case 0:  // None (linear clamp)
            ldrColor = saturate(hdrColor);
            break;
        case 1:  // Reinhard
            ldrColor = ReinhardToneMap(hdrColor);
            break;
        case 2:  // ACES
            ldrColor = ACESFilm(hdrColor);
            break;
        case 3:  // Uncharted 2
            ldrColor = Uncharted2(hdrColor);
            break;
        default:
            ldrColor = saturate(hdrColor);
            break;
    }

    // Apply gamma correction
    ldrColor = pow(ldrColor, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));

    return float4(ldrColor, 1.0);
}
