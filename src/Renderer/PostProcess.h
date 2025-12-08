#pragma once

#include "../RHI/Device.h"
#include "../RHI/Texture.h"
#include "../RHI/Buffer.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include "../RHI/Shader.h"
#include "../RHI/DescriptorHeap.h"
#include <memory>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

// Tone mapping modes
enum class ToneMapMode
{
    None,       // No tone mapping (linear)
    Reinhard,   // Simple Reinhard
    ACES,       // ACES filmic tone mapping
    Uncharted2  // Uncharted 2 filmic
};

// Post-process constants
struct PostProcessConstants
{
    float exposure;
    float gamma;
    uint32_t toneMapMode;
    float bloomIntensity;
    float bloomThreshold;
    float padding[3];
};

// Bloom constants for blur passes
struct BloomBlurConstants
{
    XMFLOAT2 texelSize;
    float direction;  // 0 = horizontal, 1 = vertical
    float padding;
};

class PostProcess
{
public:
    PostProcess(GraphicsDevice* device);
    ~PostProcess();

    bool Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    void Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);

    // Get HDR render target for scene rendering
    Texture* GetHDRRenderTarget() const { return m_hdrTexture.get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetHDRRTV() const { return m_hdrRTV.cpu; }
    DescriptorHandle GetHDRSRV() const { return m_hdrSRV; }

    // Render post-processing effects
    void Render(
        ID3D12GraphicsCommandList* commandList,
        DescriptorHeap* srvHeap,
        D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
        uint32_t outputWidth,
        uint32_t outputHeight
    );

    // Settings
    void SetExposure(float exposure) { m_exposure = exposure; }
    void SetGamma(float gamma) { m_gamma = gamma; }
    void SetToneMapMode(ToneMapMode mode) { m_toneMapMode = mode; }
    void SetBloomEnabled(bool enabled) { m_bloomEnabled = enabled; }
    void SetBloomIntensity(float intensity) { m_bloomIntensity = intensity; }
    void SetBloomThreshold(float threshold) { m_bloomThreshold = threshold; }

    float GetExposure() const { return m_exposure; }
    float GetGamma() const { return m_gamma; }
    ToneMapMode GetToneMapMode() const { return m_toneMapMode; }
    bool IsBloomEnabled() const { return m_bloomEnabled; }
    float GetBloomIntensity() const { return m_bloomIntensity; }
    float GetBloomThreshold() const { return m_bloomThreshold; }

private:
    bool CreateHDRResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    bool CreateBloomResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    bool CreateShaders();
    bool CreateRootSignatures();
    bool CreatePipelineStates();

    void RenderBloom(ID3D12GraphicsCommandList* commandList, DescriptorHeap* srvHeap);
    void RenderToneMap(
        ID3D12GraphicsCommandList* commandList,
        DescriptorHeap* srvHeap,
        D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
        uint32_t outputWidth,
        uint32_t outputHeight
    );

    GraphicsDevice* m_device = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // Post-process parameters
    float m_exposure = 1.0f;
    float m_gamma = 2.2f;
    ToneMapMode m_toneMapMode = ToneMapMode::ACES;
    bool m_bloomEnabled = true;
    float m_bloomIntensity = 0.5f;
    float m_bloomThreshold = 1.0f;

    // HDR scene render target
    std::unique_ptr<Texture> m_hdrTexture;
    DescriptorHandle m_hdrRTV;
    DescriptorHandle m_hdrSRV;

    // Bloom mip chain (for downsampling/upsampling)
    static constexpr uint32_t BLOOM_MIP_COUNT = 5;
    std::unique_ptr<Texture> m_bloomTextures[BLOOM_MIP_COUNT];
    DescriptorHandle m_bloomRTVs[BLOOM_MIP_COUNT];
    DescriptorHandle m_bloomSRVs[BLOOM_MIP_COUNT];

    // Bloom blur intermediate texture
    std::unique_ptr<Texture> m_bloomBlurTexture;
    DescriptorHandle m_bloomBlurRTV;
    DescriptorHandle m_bloomBlurSRV;

    // Constant buffers
    std::unique_ptr<Buffer> m_postProcessCB;
    std::unique_ptr<Buffer> m_bloomBlurCB;

    // Shaders
    std::unique_ptr<Shader> m_fullscreenVS;
    std::unique_ptr<Shader> m_toneMapPS;
    std::unique_ptr<Shader> m_bloomExtractPS;
    std::unique_ptr<Shader> m_bloomBlurPS;
    std::unique_ptr<Shader> m_bloomCompositePS;

    // Root signatures
    std::unique_ptr<RootSignature> m_toneMapRootSig;
    std::unique_ptr<RootSignature> m_bloomRootSig;

    // Pipeline states
    std::unique_ptr<PipelineState> m_toneMapPSO;
    std::unique_ptr<PipelineState> m_bloomExtractPSO;
    std::unique_ptr<PipelineState> m_bloomBlurPSO;
    std::unique_ptr<PipelineState> m_bloomCompositePSO;
};
