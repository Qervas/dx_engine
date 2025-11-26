#pragma once

#include "RenderPass.h"
#include "../Renderer/ShadowMap.h"
#include "../Renderer/SceneRenderer.h"

class ShadowPass : public RenderPass
{
public:
    ShadowPass(ShadowMap* shadowMap, SceneRenderer* sceneRenderer);

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    // Set the shadow map resource handle (set after import)
    void SetShadowMapHandle(RGResourceHandle handle) { m_shadowMapHandle = handle; }

    // Update light and scene info before rendering
    void SetLightInfo(const Light* light, const XMFLOAT3& sceneCenter, float sceneRadius);

private:
    ShadowMap* m_shadowMap = nullptr;
    SceneRenderer* m_sceneRenderer = nullptr;
    RGResourceHandle m_shadowMapHandle;

    const Light* m_light = nullptr;
    XMFLOAT3 m_sceneCenter = { 0, 0, 0 };
    float m_sceneRadius = 10.0f;
};
