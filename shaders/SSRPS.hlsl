// Screen-Space Reflections Pixel Shader
// Uses ray marching in screen space to find reflections

cbuffer SSRConstants : register(b0)
{
    float4x4 projection;
    float4x4 invProjection;
    float4x4 view;
    float2 screenSize;
    float maxDistance;
    float thickness;
    float stepSize;
    float maxSteps;
    float fadeStart;
    float fadeEnd;
};

Texture2D positionTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D sceneColor : register(t2);

SamplerState pointSampler : register(s0);
SamplerState linearSampler : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

// Project view-space position to screen space
float3 ProjectToScreen(float3 viewPos)
{
    float4 clipPos = mul(float4(viewPos, 1.0), projection);
    float3 ndcPos = clipPos.xyz / clipPos.w;

    // Convert from NDC (-1 to 1) to UV (0 to 1)
    float3 screenPos;
    screenPos.x = ndcPos.x * 0.5 + 0.5;
    screenPos.y = -ndcPos.y * 0.5 + 0.5;
    screenPos.z = ndcPos.z;  // Depth in NDC

    return screenPos;
}

// Get view-space position at UV
float3 GetViewPosition(float2 uv)
{
    return positionTexture.Sample(pointSampler, uv).xyz;
}

// Get view-space normal at UV
float3 GetViewNormal(float2 uv)
{
    return normalize(normalTexture.Sample(pointSampler, uv).xyz);
}

// Check if UV is valid (within screen bounds)
bool IsValidUV(float2 uv)
{
    return uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
}

// Binary search refinement for more accurate hit position
float3 BinarySearch(float3 rayOrigin, float3 rayDir, float3 hitPos)
{
    float rayLength = length(hitPos - rayOrigin);

    for (int i = 0; i < 8; i++)
    {
        float3 midPoint = rayOrigin + rayDir * rayLength;
        float3 screenPos = ProjectToScreen(midPoint);

        if (!IsValidUV(screenPos.xy))
            break;

        float3 sampledPos = GetViewPosition(screenPos.xy);
        float depthDiff = midPoint.z - sampledPos.z;

        if (depthDiff > 0.0)
        {
            rayLength -= rayLength * 0.5;
        }
        else
        {
            rayLength += rayLength * 0.25;
        }
    }

    return rayOrigin + rayDir * rayLength;
}

// Ray march in view space
float4 RayMarch(float3 rayOrigin, float3 rayDir)
{
    float3 currentPos = rayOrigin;
    float3 step = rayDir * stepSize;

    for (int i = 0; i < (int)maxSteps; i++)
    {
        currentPos += step;

        // Check distance limit
        float dist = length(currentPos - rayOrigin);
        if (dist > maxDistance)
            break;

        // Project to screen space
        float3 screenPos = ProjectToScreen(currentPos);

        // Check if outside screen bounds
        if (!IsValidUV(screenPos.xy))
            break;

        // Sample depth at this screen position
        float3 sampledPos = GetViewPosition(screenPos.xy);

        // Check for valid geometry (non-zero position means geometry exists)
        if (length(sampledPos) < 0.001)
            continue;

        // Check depth intersection
        float depthDiff = currentPos.z - sampledPos.z;

        // If we're behind geometry (positive depth diff) and within thickness
        if (depthDiff > 0.0 && depthDiff < thickness)
        {
            // Refine hit position with binary search
            float3 refinedPos = BinarySearch(rayOrigin, rayDir, currentPos);
            float3 refinedScreen = ProjectToScreen(refinedPos);

            if (!IsValidUV(refinedScreen.xy))
                break;

            // Sample scene color at hit position
            float3 hitColor = sceneColor.Sample(linearSampler, refinedScreen.xy).rgb;

            // Calculate fade based on ray distance
            float rayDist = length(refinedPos - rayOrigin) / maxDistance;
            float fade = 1.0 - saturate((rayDist - fadeStart) / (fadeEnd - fadeStart));

            // Edge fade (fade out reflections near screen edges)
            float2 edgeFade = 1.0 - pow(abs(refinedScreen.xy * 2.0 - 1.0), 8.0);
            fade *= min(edgeFade.x, edgeFade.y);

            return float4(hitColor, fade);
        }
    }

    // No hit found
    return float4(0.0, 0.0, 0.0, 0.0);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.texCoord;

    // Get view-space position and normal
    float3 viewPos = GetViewPosition(uv);
    float3 viewNormal = GetViewNormal(uv);

    // Skip pixels with no geometry
    if (length(viewPos) < 0.001)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    // Calculate reflection direction in view space
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = reflect(viewDir, viewNormal);

    // Skip reflections pointing away from camera
    if (reflectDir.z > 0.0)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    // Perform ray march
    float4 reflection = RayMarch(viewPos + reflectDir * stepSize, reflectDir);

    return reflection;
}
