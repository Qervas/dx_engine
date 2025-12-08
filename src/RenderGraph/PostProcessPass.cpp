#include "PostProcessPass.h"
#include "../RHI/CommandList.h"

PostProcessPass::PostProcessPass(PostProcess* postProcess, SwapChain* swapChain)
    : RenderPass("Post Process Pass")
    , m_postProcess(postProcess)
    , m_swapChain(swapChain)
{
}

void PostProcessPass::Setup(RenderGraph& graph)
{
    // Post-process pass reads from HDR texture and bloom, writes to swap chain
}

void PostProcessPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_postProcess || !m_swapChain || !m_enabled)
        return;

    // Get current back buffer RTV
    D3D12_CPU_DESCRIPTOR_HANDLE outputRTV = m_swapChain->GetRTV(m_swapChain->GetCurrentBackBufferIndex());

    // Render post-processing to swap chain back buffer
    m_postProcess->Render(
        commandList->GetD3D12CommandList(),
        srvHeap,
        outputRTV,
        m_swapChain->GetWidth(),
        m_swapChain->GetHeight()
    );
}
