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

private:
    DebugRenderer* m_debugRenderer = nullptr;
    SwapChain* m_swapChain = nullptr;
    RGResourceHandle m_renderTarget;
    RGResourceHandle m_depthTarget;
    uint32_t m_backBufferIndex = 0;
};
