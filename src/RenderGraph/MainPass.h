#pragma once

#include "RenderPass.h"
#include "../Renderer/SceneRenderer.h"
#include "../Renderer/ShadowMap.h"
#include "../RHI/SwapChain.h"
#include "../RHI/Buffer.h"

class MainPass : public RenderPass
{
public:
    MainPass(SceneRenderer* sceneRenderer, SwapChain* swapChain);

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    // Set resource handles
    void SetBackBufferHandle(RGResourceHandle handle) { m_backBufferHandle = handle; }
    void SetDepthBufferHandle(RGResourceHandle handle) { m_depthBufferHandle = handle; }
    void SetShadowMapHandle(RGResourceHandle handle) { m_shadowMapHandle = handle; }

    // Set shadow resources for binding
    void SetShadowResources(ShadowMap* shadowMap, Buffer* shadowCB);

    // Update per-frame data
    void SetBackBufferIndex(uint32_t index) { m_backBufferIndex = index; }

    // Set custom render target (for HDR rendering)
    void SetCustomRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, uint32_t width, uint32_t height);
    void ClearCustomRTV();

private:
    SceneRenderer* m_sceneRenderer = nullptr;
    SwapChain* m_swapChain = nullptr;
    ShadowMap* m_shadowMap = nullptr;
    Buffer* m_shadowCB = nullptr;

    RGResourceHandle m_backBufferHandle;
    RGResourceHandle m_depthBufferHandle;
    RGResourceHandle m_shadowMapHandle;

    uint32_t m_backBufferIndex = 0;
    float m_clearColor[4] = { 0.02f, 0.02f, 0.02f, 1.0f };

    // Custom render target override
    bool m_useCustomRTV = false;
    D3D12_CPU_DESCRIPTOR_HANDLE m_customRTV = {};
    uint32_t m_customWidth = 0;
    uint32_t m_customHeight = 0;
};
