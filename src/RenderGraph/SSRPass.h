#pragma once

#include "RenderPass.h"
#include "../Renderer/SSR.h"
#include "../Renderer/GBuffer.h"
#include "../Renderer/Camera.h"
#include "../RHI/Texture.h"

class SSRPass : public RenderPass
{
public:
    SSRPass(SSR* ssr, GBuffer* gBuffer, Camera* camera);
    ~SSRPass() override = default;

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    // Set the scene color texture to sample reflections from
    void SetSceneColorTexture(Texture* sceneColor) { m_sceneColor = sceneColor; }

private:
    SSR* m_ssr = nullptr;
    GBuffer* m_gBuffer = nullptr;
    Camera* m_camera = nullptr;
    Texture* m_sceneColor = nullptr;
};
