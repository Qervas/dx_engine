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

    // Set custom output RTV (for rendering to scene viewport texture instead of swap chain)
    void SetCustomOutputRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, uint32_t width, uint32_t height)
    {
        m_customOutputRTV = rtv;
        m_customWidth = width;
        m_customHeight = height;
        m_useCustomOutput = true;
    }

    void ClearCustomOutputRTV()
    {
        m_useCustomOutput = false;
    }

private:
    PostProcess* m_postProcess = nullptr;
    SwapChain* m_swapChain = nullptr;
    bool m_enabled = true;

    // Custom output for editor viewport
    D3D12_CPU_DESCRIPTOR_HANDLE m_customOutputRTV = {};
    uint32_t m_customWidth = 0;
    uint32_t m_customHeight = 0;
    bool m_useCustomOutput = false;
};
