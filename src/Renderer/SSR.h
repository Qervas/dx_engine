#pragma once

#include "../RHI/Device.h"
#include "../RHI/Texture.h"
#include "../RHI/Buffer.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include "../RHI/Shader.h"
#include "../RHI/DescriptorHeap.h"
#include "GBuffer.h"
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

// SSR Constants for shader
struct SSRConstants
{
    XMFLOAT4X4 projection;
    XMFLOAT4X4 invProjection;
    XMFLOAT4X4 view;
    XMFLOAT2 screenSize;
    float maxDistance;       // Maximum ray march distance
    float thickness;         // Depth thickness threshold
    float stepSize;          // Ray march step size
    float maxSteps;          // Maximum ray march steps
    float fadeStart;         // Distance to start fading reflections
    float fadeEnd;           // Distance to end reflections
};

class SSR
{
public:
    SSR(GraphicsDevice* device);
    ~SSR();

    bool Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    void Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);

    // Render SSR from G-Buffer and scene color
    void Render(
        ID3D12GraphicsCommandList* commandList,
        DescriptorHeap* srvHeap,
        GBuffer* gBuffer,
        Texture* sceneColor,
        const XMMATRIX& view,
        const XMMATRIX& projection
    );

    // Get the SSR reflection texture
    Texture* GetSSRTexture() const { return m_ssrTexture.get(); }
    DescriptorHandle GetSSRSRV() const { return m_ssrSRV; }

    // Settings
    void SetMaxDistance(float dist) { m_maxDistance = dist; }
    void SetThickness(float thickness) { m_thickness = thickness; }
    void SetStepSize(float size) { m_stepSize = size; }
    void SetMaxSteps(float steps) { m_maxSteps = steps; }

    float GetMaxDistance() const { return m_maxDistance; }
    float GetThickness() const { return m_thickness; }
    float GetStepSize() const { return m_stepSize; }
    float GetMaxSteps() const { return m_maxSteps; }

private:
    bool CreateResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    bool CreateShaders();
    bool CreateRootSignature();
    bool CreatePipelineState();

    GraphicsDevice* m_device = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // SSR parameters
    float m_maxDistance = 100.0f;
    float m_thickness = 0.5f;
    float m_stepSize = 0.1f;
    float m_maxSteps = 64.0f;
    float m_fadeStart = 0.8f;
    float m_fadeEnd = 1.0f;

    // SSR output texture
    std::unique_ptr<Texture> m_ssrTexture;
    DescriptorHandle m_ssrRTV;
    DescriptorHandle m_ssrSRV;

    // Constant buffer
    std::unique_ptr<Buffer> m_constantBuffer;

    // Shaders
    std::unique_ptr<Shader> m_ssrVS;
    std::unique_ptr<Shader> m_ssrPS;

    // Root signature and pipeline
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;
};
