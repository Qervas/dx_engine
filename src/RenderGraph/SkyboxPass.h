#pragma once

#include "RenderPass.h"
#include "../Renderer/Skybox.h"
#include "../RHI/SwapChain.h"

class Camera;

class SkyboxPass : public RenderPass
{
public:
    SkyboxPass(Skybox* skybox, SwapChain* swapChain, Camera* camera);

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    // Set resource handles
    void SetBackBufferHandle(RGResourceHandle handle) { m_backBufferHandle = handle; }
    void SetDepthBufferHandle(RGResourceHandle handle) { m_depthBufferHandle = handle; }

    // Update per-frame data
    void SetBackBufferIndex(uint32_t index) { m_backBufferIndex = index; }
    void SetCamera(Camera* camera) { m_camera = camera; }

private:
    Skybox* m_skybox = nullptr;
    SwapChain* m_swapChain = nullptr;
    Camera* m_camera = nullptr;

    RGResourceHandle m_backBufferHandle;
    RGResourceHandle m_depthBufferHandle;

    uint32_t m_backBufferIndex = 0;
};
