#include "SSRPass.h"
#include "../RHI/CommandList.h"

SSRPass::SSRPass(SSR* ssr, GBuffer* gBuffer, Camera* camera)
    : RenderPass("SSR Pass")
    , m_ssr(ssr)
    , m_gBuffer(gBuffer)
    , m_camera(camera)
{
}

void SSRPass::Setup(RenderGraph& graph)
{
    // SSR pass reads from G-Buffer and scene color, writes to SSR texture
    // Dependencies are managed externally
}

void SSRPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_ssr || !m_gBuffer || !m_camera || !m_sceneColor)
        return;

    // Get view and projection matrices
    XMMATRIX view = m_camera->GetViewMatrix();
    XMMATRIX projection = m_camera->GetProjectionMatrix();

    // Render SSR
    m_ssr->Render(commandList->GetD3D12CommandList(), srvHeap, m_gBuffer, m_sceneColor, view, projection);
}
