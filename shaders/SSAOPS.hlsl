// SSAO Pixel Shader
// Screen-Space Ambient Occlusion using hemisphere sampling

cbuffer SSAOConstants : register(b0)
{
    float4x4 projection;
    float4x4 invProjection;
    float4 samples[64];
    float2 noiseScale;
    float radius;
    float bias;
    float intensity;
    float3 padding;
};

Texture2D positionTexture : register(t0);  // View-space positions
Texture2D normalTexture : register(t1);    // View-space normals
Texture2D noiseTexture : register(t2);     // Random rotation vectors

SamplerState pointSampler : register(s0);
SamplerState noiseSampler : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float main(PSInput input) : SV_TARGET
{
    // Get view-space position and normal
    float3 fragPos = positionTexture.Sample(pointSampler, input.texCoord).xyz;
    float3 normal = normalTexture.Sample(pointSampler, input.texCoord).xyz;

    // Early out if no geometry (far plane)
    if (length(normal) < 0.1f)
    {
        return 1.0f;
    }

    normal = normalize(normal);

    // Get random rotation vector from noise texture
    float3 randomVec = noiseTexture.Sample(noiseSampler, input.texCoord * noiseScale).xyz;

    // Create TBN matrix (Gram-Schmidt process)
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    // Sample and accumulate occlusion
    float occlusion = 0.0f;
    int validSamples = 0;

    [unroll]
    for (int i = 0; i < 64; ++i)
    {
        // Get sample position in view space
        float3 sampleDir = mul(samples[i].xyz, TBN);
        float3 samplePos = fragPos + sampleDir * radius;

        // Project sample position to screen space
        float4 offset = float4(samplePos, 1.0f);
        offset = mul(offset, projection);
        offset.xyz /= offset.w;

        // Transform to [0,1] UV space
        float2 sampleUV;
        sampleUV.x = offset.x * 0.5f + 0.5f;
        sampleUV.y = -offset.y * 0.5f + 0.5f;

        // Check if sample is within screen bounds
        if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f)
        {
            continue;
        }

        // Get depth at sample position
        float3 sampleDepthPos = positionTexture.Sample(pointSampler, sampleUV).xyz;
        float sampleDepth = sampleDepthPos.z;

        // Range check and accumulate
        float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(fragPos.z - sampleDepth));

        // If sample is in front of the fragment (occluder), add occlusion
        if (sampleDepth >= samplePos.z + bias)
        {
            occlusion += rangeCheck;
        }

        validSamples++;
    }

    // Normalize and apply intensity
    if (validSamples > 0)
    {
        occlusion = 1.0f - (occlusion / float(validSamples)) * intensity;
    }
    else
    {
        occlusion = 1.0f;
    }

    return saturate(occlusion);
}
