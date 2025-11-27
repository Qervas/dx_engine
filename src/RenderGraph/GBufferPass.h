#pragma once

#include "RenderPass.h"
#include "../Renderer/GBuffer.h"
#include "../Renderer/SceneRenderer.h"
#include "../RHI/SwapChain.h"
#include "../RHI/Shader.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include <memory>

class GBufferPass : public RenderPass
{
public:
    GBufferPass(GraphicsDevice* device, GBuffer* gBuffer, SceneRenderer* sceneRenderer, SwapChain* swapChain);
    ~GBufferPass() override = default;

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    bool Initialize();

private:
    GraphicsDevice* m_device = nullptr;
    GBuffer* m_gBuffer = nullptr;
    SceneRenderer* m_sceneRenderer = nullptr;
    SwapChain* m_swapChain = nullptr;

    std::unique_ptr<Shader> m_vertexShader;
    std::unique_ptr<Shader> m_pixelShader;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;
};
