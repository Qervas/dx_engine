#include "ShadowPass.h"
#include "RenderGraph.h"

ShadowPass::ShadowPass(ShadowMap* shadowMap, SceneRenderer* sceneRenderer)
    : RenderPass("ShadowPass")
    , m_shadowMap(shadowMap)
    , m_sceneRenderer(sceneRenderer)
{
}

void ShadowPass::Setup(RenderGraph& graph)
{
    // Declare shadow map as output (we write depth to it)
    if (m_shadowMapHandle.IsValid())
    {
        AddOutput(m_shadowMapHandle, ResourceAccess::DepthWrite, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }
}

void ShadowPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_shadowMap || !m_sceneRenderer || !m_light)
        return;

    // Update shadow map matrices
    m_shadowMap->UpdateLightMatrix(*m_light, m_sceneCenter, m_sceneRadius);

    // Begin shadow pass (sets pipeline, viewport, clears depth)
    m_shadowMap->BeginShadowPass(commandList);

    // Render scene geometry to shadow map
    m_sceneRenderer->RenderShadowPass(commandList);

    // End shadow pass (transition for sampling)
    m_shadowMap->EndShadowPass(commandList);
}

void ShadowPass::SetLightInfo(const Light* light, const XMFLOAT3& sceneCenter, float sceneRadius)
{
    m_light = light;
    m_sceneCenter = sceneCenter;
    m_sceneRadius = sceneRadius;
}
