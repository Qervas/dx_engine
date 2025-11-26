#include "SkyboxPass.h"
#include "RenderGraph.h"

SkyboxPass::SkyboxPass(Skybox* skybox, SwapChain* swapChain, Camera* camera)
    : RenderPass("SkyboxPass")
    , m_skybox(skybox)
    , m_swapChain(swapChain)
    , m_camera(camera)
{
}

void SkyboxPass::Setup(RenderGraph& graph)
{
    // Skybox reads from depth buffer (for depth test) and writes to back buffer
    if (m_backBufferHandle.IsValid())
    {
        AddOutput(m_backBufferHandle, ResourceAccess::Write, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    if (m_depthBufferHandle.IsValid())
    {
        AddInput(m_depthBufferHandle, ResourceAccess::DepthRead, D3D12_RESOURCE_STATE_DEPTH_READ);
    }
}

void SkyboxPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_skybox || !m_swapChain || !m_camera)
        return;

    // Get render targets
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetRTV(m_backBufferIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_swapChain->GetDSV();

    // Set render target with depth buffer (depth read only)
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set viewport and scissor
    commandList->SetViewport(0, 0,
        static_cast<float>(m_swapChain->GetWidth()),
        static_cast<float>(m_swapChain->GetHeight()));
    commandList->SetScissorRect(0, 0,
        m_swapChain->GetWidth(),
        m_swapChain->GetHeight());

    // Render skybox
    m_skybox->Render(commandList, srvHeap, m_camera);
}
