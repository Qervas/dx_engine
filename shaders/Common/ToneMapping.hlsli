// Tone Mapping Functions
#ifndef TONEMAPPING_HLSLI
#define TONEMAPPING_HLSLI

// Reinhard tone mapping
float3 ReinhardToneMap(float3 hdr)
{
    return hdr / (hdr + 1.0);
}

// Extended Reinhard with white point
float3 ReinhardExtended(float3 hdr, float whitePoint)
{
    float3 numerator = hdr * (1.0 + hdr / (whitePoint * whitePoint));
    return numerator / (1.0 + hdr);
}

// ACES Filmic tone mapping
// From: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
float3 ACESFilm(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Uncharted 2 tone mapping helper
float3 Uncharted2ToneMapHelper(float3 x)
{
    float A = 0.15;  // Shoulder Strength
    float B = 0.50;  // Linear Strength
    float C = 0.10;  // Linear Angle
    float D = 0.20;  // Toe Strength
    float E = 0.02;  // Toe Numerator
    float F = 0.30;  // Toe Denominator

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Uncharted 2 tone mapping
float3 Uncharted2ToneMap(float3 hdr)
{
    float W = 11.2;  // Linear White Point Value
    float exposureBias = 2.0;

    float3 curr = Uncharted2ToneMapHelper(exposureBias * hdr);
    float3 whiteScale = 1.0 / Uncharted2ToneMapHelper(float3(W, W, W));

    return curr * whiteScale;
}

// Apply gamma correction
float3 GammaCorrect(float3 color, float gamma)
{
    return pow(color, 1.0 / gamma);
}

// Apply inverse gamma (linear to sRGB)
float3 LinearToSRGB(float3 color)
{
    return pow(color, 1.0 / 2.2);
}

// Apply gamma (sRGB to linear)
float3 SRGBToLinear(float3 color)
{
    return pow(color, 2.2);
}

#endif // TONEMAPPING_HLSLI
