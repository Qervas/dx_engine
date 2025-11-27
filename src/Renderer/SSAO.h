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
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

// SSAO Constants for shader
struct SSAOConstants
{
    XMFLOAT4X4 projection;
    XMFLOAT4X4 invProjection;
    XMFLOAT4 samples[64];  // Hemisphere sample kernel
    XMFLOAT2 noiseScale;   // Screen dimensions / noise texture size
    float radius;          // Sample radius
    float bias;            // Depth bias to prevent self-occlusion
    float intensity;       // SSAO intensity multiplier
    float padding[3];
};

// Blur constants
struct BlurConstants
{
    XMFLOAT2 texelSize;
    float padding[2];
};

class SSAO
{
public:
    SSAO(GraphicsDevice* device);
    ~SSAO();

    bool Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    void Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);

    // Render SSAO from G-Buffer
    void Render(
        ID3D12GraphicsCommandList* commandList,
        DescriptorHeap* srvHeap,
        GBuffer* gBuffer,
        const XMMATRIX& projection
    );

    // Get the final SSAO texture (blurred)
    Texture* GetSSAOTexture() const { return m_ssaoBlurTexture.get(); }
    DescriptorHandle GetSSAOSRV() const { return m_ssaoBlurSRV; }

    // Settings
    void SetRadius(float radius) { m_radius = radius; }
    void SetBias(float bias) { m_bias = bias; }
    void SetIntensity(float intensity) { m_intensity = intensity; }

    float GetRadius() const { return m_radius; }
    float GetBias() const { return m_bias; }
    float GetIntensity() const { return m_intensity; }

private:
    bool CreateResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    bool CreateShaders();
    bool CreateRootSignatures();
    bool CreatePipelineStates();
    void GenerateSampleKernel();
    void GenerateNoiseTexture(DescriptorHeap* srvHeap);

    GraphicsDevice* m_device = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // SSAO parameters
    float m_radius = 0.5f;
    float m_bias = 0.025f;
    float m_intensity = 1.5f;

    // Sample kernel (hemisphere samples)
    std::vector<XMFLOAT4> m_sampleKernel;

    // Noise texture (4x4 random rotation vectors)
    std::unique_ptr<Texture> m_noiseTexture;
    DescriptorHandle m_noiseSRV;

    // SSAO output texture
    std::unique_ptr<Texture> m_ssaoTexture;
    DescriptorHandle m_ssaoRTV;
    DescriptorHandle m_ssaoSRV;

    // Blurred SSAO texture
    std::unique_ptr<Texture> m_ssaoBlurTexture;
    DescriptorHandle m_ssaoBlurRTV;
    DescriptorHandle m_ssaoBlurSRV;

    // Constant buffers
    std::unique_ptr<Buffer> m_ssaoConstantBuffer;
    std::unique_ptr<Buffer> m_blurConstantBuffer;

    // Shaders
    std::unique_ptr<Shader> m_ssaoVS;
    std::unique_ptr<Shader> m_ssaoPS;
    std::unique_ptr<Shader> m_blurPS;

    // Root signatures
    std::unique_ptr<RootSignature> m_ssaoRootSignature;
    std::unique_ptr<RootSignature> m_blurRootSignature;

    // Pipeline states
    std::unique_ptr<PipelineState> m_ssaoPipelineState;
    std::unique_ptr<PipelineState> m_blurPipelineState;
};
