#pragma once

#include "../RHI/Texture.h"
#include "../RHI/DescriptorHeap.h"
#include "../RHI/Device.h"
#include <memory>

// G-Buffer for deferred rendering and screen-space effects
// Contains:
// - Position (view-space) - R16G16B16A16_FLOAT
// - Normal (view-space) - R16G16B16A16_FLOAT
// - Depth - uses existing depth buffer

class GBuffer
{
public:
    GBuffer(GraphicsDevice* device);
    ~GBuffer();

    bool Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    void Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);

    // Getters for textures
    Texture* GetPositionTexture() const { return m_positionTexture.get(); }
    Texture* GetNormalTexture() const { return m_normalTexture.get(); }

    // Getters for render target views
    D3D12_CPU_DESCRIPTOR_HANDLE GetPositionRTV() const { return m_positionRTV.cpu; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetNormalRTV() const { return m_normalRTV.cpu; }

    // Getters for shader resource views
    DescriptorHandle GetPositionSRV() const { return m_positionSRV; }
    DescriptorHandle GetNormalSRV() const { return m_normalSRV; }

    // Get dimensions
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    // Clear the G-Buffer
    void Clear(ID3D12GraphicsCommandList* commandList);

    // Transition barriers
    void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);
    void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);

private:
    bool CreateTextures(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap);
    void ReleaseTextures();

    GraphicsDevice* m_device = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // G-Buffer textures
    std::unique_ptr<Texture> m_positionTexture;
    std::unique_ptr<Texture> m_normalTexture;

    // Render target views
    DescriptorHandle m_positionRTV;
    DescriptorHandle m_normalRTV;

    // Shader resource views
    DescriptorHandle m_positionSRV;
    DescriptorHandle m_normalSRV;
};
