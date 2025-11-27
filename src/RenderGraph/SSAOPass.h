#pragma once

#include "RenderPass.h"
#include "../Renderer/SSAO.h"
#include "../Renderer/GBuffer.h"
#include "../Renderer/Camera.h"

class SSAOPass : public RenderPass
{
public:
    SSAOPass(SSAO* ssao, GBuffer* gBuffer, Camera* camera);
    ~SSAOPass() override = default;

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

private:
    SSAO* m_ssao = nullptr;
    GBuffer* m_gBuffer = nullptr;
    Camera* m_camera = nullptr;
};
