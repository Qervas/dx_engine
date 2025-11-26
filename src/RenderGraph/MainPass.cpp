#include "MainPass.h"
#include "RenderGraph.h"

MainPass::MainPass(SceneRenderer* sceneRenderer, SwapChain* swapChain)
    : RenderPass("MainPass")
    , m_sceneRenderer(sceneRenderer)
    , m_swapChain(swapChain)
{
}

void MainPass::Setup(RenderGraph& graph)
{
    // Declare inputs
    if (m_shadowMapHandle.IsValid())
    {
        AddInput(m_shadowMapHandle, ResourceAccess::Read, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // Declare outputs
    if (m_backBufferHandle.IsValid())
    {
        AddOutput(m_backBufferHandle, ResourceAccess::Write, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    if (m_depthBufferHandle.IsValid())
    {
        AddOutput(m_depthBufferHandle, ResourceAccess::DepthWrite, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }
}

void MainPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_sceneRenderer || !m_swapChain)
        return;

    // Get render targets
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetRTV(m_backBufferIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_swapChain->GetDSV();

    // Set render target with depth buffer
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set viewport and scissor
    commandList->SetViewport(0, 0,
        static_cast<float>(m_swapChain->GetWidth()),
        static_cast<float>(m_swapChain->GetHeight()));
    commandList->SetScissorRect(0, 0,
        m_swapChain->GetWidth(),
        m_swapChain->GetHeight());

    // Clear render target and depth buffer
    commandList->ClearRenderTargetView(rtv, m_clearColor);
    commandList->ClearDepthStencilView(dsv, 1.0f);

    // Update shadow constant buffer if available
    if (m_shadowMap && m_shadowCB)
    {
        ShadowConstants shadowConst;
        XMStoreFloat4x4(&shadowConst.lightViewProj,
            XMMatrixTranspose(m_shadowMap->GetLightViewProjection()));
        m_shadowCB->UpdateData(&shadowConst, sizeof(ShadowConstants));
    }

    // Render scene
    m_sceneRenderer->Render(commandList, srvHeap);
}

void MainPass::SetShadowResources(ShadowMap* shadowMap, Buffer* shadowCB)
{
    m_shadowMap = shadowMap;
    m_shadowCB = shadowCB;
}
