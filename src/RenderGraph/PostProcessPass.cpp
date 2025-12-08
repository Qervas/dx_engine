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
    if (!m_postProcess || !m_enabled)
        return;

    D3D12_CPU_DESCRIPTOR_HANDLE outputRTV;
    uint32_t width, height;

    if (m_useCustomOutput)
    {
        // Render to custom output (scene viewport texture)
        outputRTV = m_customOutputRTV;
        width = m_customWidth;
        height = m_customHeight;
    }
    else
    {
        // Render to swap chain back buffer (fallback)
        if (!m_swapChain)
            return;
        outputRTV = m_swapChain->GetRTV(m_swapChain->GetCurrentBackBufferIndex());
        width = m_swapChain->GetWidth();
        height = m_swapChain->GetHeight();
    }

    // Render post-processing
    m_postProcess->Render(
        commandList->GetD3D12CommandList(),
        srvHeap,
        outputRTV,
        width,
        height
    );
}
