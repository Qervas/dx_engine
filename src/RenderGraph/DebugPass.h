#pragma once

#include "RenderPass.h"
#include "../Renderer/DebugRenderer.h"

class DebugPass : public RenderPass
{
public:
    DebugPass(DebugRenderer* debugRenderer);
    ~DebugPass() override = default;

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    void SetRenderTarget(RGResourceHandle target) { m_renderTarget = target; }
    void SetDepthTarget(RGResourceHandle depth) { m_depthTarget = depth; }

private:
    DebugRenderer* m_debugRenderer = nullptr;
    RGResourceHandle m_renderTarget;
    RGResourceHandle m_depthTarget;
};
