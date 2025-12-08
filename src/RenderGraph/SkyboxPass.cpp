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

    // Get render targets (use custom RTV if set, otherwise use swap chain)
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    uint32_t width, height;

    if (m_useCustomRTV)
    {
        rtv = m_customRTV;
        width = m_customWidth;
        height = m_customHeight;
    }
    else
    {
        rtv = m_swapChain->GetRTV(m_backBufferIndex);
        width = m_swapChain->GetWidth();
        height = m_swapChain->GetHeight();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_swapChain->GetDSV();

    // Set render target with depth buffer (depth read only)
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set viewport and scissor
    commandList->SetViewport(0, 0,
        static_cast<float>(width),
        static_cast<float>(height));
    commandList->SetScissorRect(0, 0, width, height);

    // Render skybox
    m_skybox->Render(commandList, srvHeap, m_camera);
}

void SkyboxPass::SetCustomRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, uint32_t width, uint32_t height)
{
    m_useCustomRTV = true;
    m_customRTV = rtv;
    m_customWidth = width;
    m_customHeight = height;
}

void SkyboxPass::ClearCustomRTV()
{
    m_useCustomRTV = false;
}
