#pragma once

#include "RenderPass.h"
#include "../Renderer/PostProcess.h"
#include "../RHI/SwapChain.h"

class PostProcessPass : public RenderPass
{
public:
    PostProcessPass(PostProcess* postProcess, SwapChain* swapChain);
    ~PostProcessPass() override = default;

    void Setup(RenderGraph& graph) override;
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override;

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

private:
    PostProcess* m_postProcess = nullptr;
    SwapChain* m_swapChain = nullptr;
    bool m_enabled = true;
};
