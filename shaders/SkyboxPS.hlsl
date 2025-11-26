// Skybox Pixel Shader
// Samples cubemap using world direction

TextureCube skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);

cbuffer SkyboxConstants : register(b0)
{
    float4x4 inverseViewProjection;
    float3 cameraPosition;
    float exposure;  // For HDR skyboxes (future use)
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldDir : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    // Normalize direction (it gets interpolated)
    float3 dir = normalize(input.worldDir - cameraPosition);

    // Sample cubemap
    float3 color = skyboxTexture.Sample(skyboxSampler, dir).rgb;

    // Apply exposure (for future HDR support)
    color *= exposure;

    return float4(color, 1.0f);
}
