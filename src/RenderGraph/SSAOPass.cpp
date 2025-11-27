#include "SSAOPass.h"
#include "../RHI/CommandList.h"

SSAOPass::SSAOPass(SSAO* ssao, GBuffer* gBuffer, Camera* camera)
    : RenderPass("SSAO Pass")
    , m_ssao(ssao)
    , m_gBuffer(gBuffer)
    , m_camera(camera)
{
}

void SSAOPass::Setup(RenderGraph& graph)
{
    // SSAO pass reads from G-Buffer (position, normal) and writes to SSAO texture
    // Dependencies are managed externally for now
}

void SSAOPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_ssao || !m_gBuffer || !m_camera)
        return;

    // Get projection matrix for SSAO calculations
    XMMATRIX projection = m_camera->GetProjectionMatrix();

    // Render SSAO
    m_ssao->Render(commandList->GetD3D12CommandList(), srvHeap, m_gBuffer, projection);
}
