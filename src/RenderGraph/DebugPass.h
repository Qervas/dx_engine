#pragma once

#include "RenderPass.h"
#include "../Renderer/DebugRenderer.h"
#include "../RHI/SwapChain.h"

class DebugPass : public RenderPass
{
public:
    DebugPass(DebugRenderer* debugRenderer, SwapChain* swapChain);
    ~DebugPass() override = default;

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    void SetRenderTarget(RGResourceHandle target) { m_renderTarget = target; }
    void SetDepthTarget(RGResourceHandle depth) { m_depthTarget = depth; }
    void SetBackBufferIndex(uint32_t index) { m_backBufferIndex = index; }

    // Set custom output RTV (for rendering to scene viewport texture)
    void SetCustomOutputRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, uint32_t width, uint32_t height)
    {
        m_customOutputRTV = rtv;
        m_customWidth = width;
        m_customHeight = height;
        m_useCustomOutput = true;
    }

    void ClearCustomOutputRTV()
    {
        m_useCustomOutput = false;
    }

private:
    DebugRenderer* m_debugRenderer = nullptr;
    SwapChain* m_swapChain = nullptr;
    RGResourceHandle m_renderTarget;
    RGResourceHandle m_depthTarget;
    uint32_t m_backBufferIndex = 0;

    // Custom output for editor viewport
    D3D12_CPU_DESCRIPTOR_HANDLE m_customOutputRTV = {};
    uint32_t m_customWidth = 0;
    uint32_t m_customHeight = 0;
    bool m_useCustomOutput = false;
};
