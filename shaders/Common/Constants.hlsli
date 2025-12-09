// Shared Constant Buffers and Structures
#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI

// Light types
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

// Light data structure
struct LightData
{
    float3 position;
    uint type;

    float3 direction;
    float range;

    float3 color;
    float intensity;

    float spotAngle;
    float3 _padding;
};

// Transform constant buffer (b0)
cbuffer TransformCB : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 worldViewProj;
    float4x4 normalMatrix;
};

// Material constant buffer (b1)
cbuffer MaterialCB : register(b1)
{
    float3 g_Albedo;
    float g_Metallic;

    float g_Roughness;
    float g_AO;
    float g_UseNormalMap;
    float _matPadding;
};

// Scene lighting constant buffer (b2)
cbuffer SceneLightingCB : register(b2)
{
    float3 g_CameraPosition;
    uint g_LightCount;

    LightData g_Lights[4];
};

// Shadow constant buffer (b3)
cbuffer ShadowCB : register(b3)
{
    float4x4 g_LightViewProj;
    float2 g_ShadowMapSize;
    float g_ShadowBias;
    float _shadowPadding;
};

// IBL constant buffer (b4)
cbuffer IBLCB : register(b4)
{
    uint g_PrefilteredMipLevels;
    float3 _iblPadding;
};

// Post-process constant buffer (b0 for post-process passes)
cbuffer PostProcessCB : register(b0)
{
    float g_Exposure;
    float g_Gamma;
    uint g_ToneMapMode;
    float g_BloomIntensity;
    float g_BloomThreshold;
    float3 _postPadding;
};

// Common vertex shader input
struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

// Vertex shader input with tangents
struct VSInputTangent
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

// Common pixel shader input
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POS;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

// Pixel shader input with tangent frame
struct PSInputTangent
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POS;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// Fullscreen quad input
struct FullscreenPSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

#endif // CONSTANTS_HLSLI
