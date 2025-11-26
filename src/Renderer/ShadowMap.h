#pragma once

#include "../RHI/Device.h"
#include "../RHI/Texture.h"
#include "../RHI/Buffer.h"
#include "../RHI/Shader.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include "../RHI/DescriptorHeap.h"
#include "Light.h"
#include <DirectXMath.h>
#include <memory>

using namespace DirectX;

class CommandList;
class Scene;

// Shadow map constants sent to GPU
struct ShadowConstants
{
    XMFLOAT4X4 lightViewProj;
};

class ShadowMap
{
public:
    ShadowMap(GraphicsDevice* device);
    ~ShadowMap();

    bool Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap);
    void Shutdown();

    // Update light matrices for shadow rendering
    void UpdateLightMatrix(const Light& light, const XMFLOAT3& sceneCenter, float sceneRadius);

    // Begin shadow pass - sets render target, viewport, clears depth
    void BeginShadowPass(CommandList* commandList);

    // End shadow pass - transitions texture for sampling
    void EndShadowPass(CommandList* commandList);

    // Get the light view-projection matrix for shadow sampling
    XMMATRIX GetLightViewProjection() const { return m_lightViewProj; }

    // Get shadow map texture for binding to main pass
    Texture* GetShadowTexture() const { return m_shadowTexture.get(); }
    DescriptorHandle GetSRV() const { return m_srv; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;

    // Get shadow pass resources
    RootSignature* GetRootSignature() const { return m_rootSignature.get(); }
    PipelineState* GetPipelineState() const { return m_pipelineState.get(); }
    Buffer* GetConstantBuffer() const { return m_constantBuffer.get(); }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

private:
    bool CreateShadowTexture();
    bool CreatePipeline();

    GraphicsDevice* m_device = nullptr;
    DescriptorHeap* m_srvHeap = nullptr;

    uint32_t m_width = 2048;
    uint32_t m_height = 2048;

    // Shadow map texture (depth-only)
    std::unique_ptr<Texture> m_shadowTexture;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    DescriptorHandle m_srv;

    // Shadow pass pipeline
    std::unique_ptr<Shader> m_vertexShader;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;

    // Constants
    std::unique_ptr<Buffer> m_constantBuffer;
    XMMATRIX m_lightViewProj;
    XMMATRIX m_lightView;
    XMMATRIX m_lightProj;
};
