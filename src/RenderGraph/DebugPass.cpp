#include "DebugPass.h"
#include "RenderGraph.h"

DebugPass::DebugPass(DebugRenderer* debugRenderer)
    : RenderPass("DebugPass")
    , m_debugRenderer(debugRenderer)
{
}

void DebugPass::Setup(RenderGraph& graph)
{
    // Read/write render target (we overlay on top of existing content)
    AddInput(m_renderTarget, ResourceAccess::Read, D3D12_RESOURCE_STATE_RENDER_TARGET);
    AddOutput(m_renderTarget, ResourceAccess::Write, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Read depth buffer for depth testing
    AddInput(m_depthTarget, ResourceAccess::Read, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void DebugPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_debugRenderer || !m_debugRenderer->HasPrimitives())
        return;

    m_debugRenderer->Render(commandList);
}
