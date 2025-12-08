#include "DebugPass.h"
#include "RenderGraph.h"

DebugPass::DebugPass(DebugRenderer* debugRenderer, SwapChain* swapChain)
    : RenderPass("DebugPass")
    , m_debugRenderer(debugRenderer)
    , m_swapChain(swapChain)
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
    if (!m_debugRenderer || !m_debugRenderer->HasPrimitives() || !m_swapChain)
        return;

    // Set render targets - swap chain back buffer with depth buffer for proper depth testing
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetRTV(m_backBufferIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_swapChain->GetDSV();

    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set viewport and scissor
    commandList->SetViewport(0, 0,
        static_cast<float>(m_swapChain->GetWidth()),
        static_cast<float>(m_swapChain->GetHeight()));
    commandList->SetScissorRect(0, 0,
        m_swapChain->GetWidth(),
        m_swapChain->GetHeight());

    m_debugRenderer->Render(commandList);
}
