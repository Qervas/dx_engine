#pragma once

#include "RenderPass.h"
#include "../RHI/Device.h"
#include "../RHI/Texture.h"
#include "../RHI/Buffer.h"
#include <memory>
#include <vector>
#include <unordered_map>

// Resource type in render graph
enum class RGResourceType
{
    Texture,
    Buffer,
    External    // Not managed by render graph
};

// Resource descriptor for render graph
struct RGResourceDesc
{
    std::string name;
    RGResourceType type = RGResourceType::Texture;

    // For textures
    uint32_t width = 0;
    uint32_t height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

    // Current state tracking
    D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;

    // The actual resource (owned or external)
    ID3D12Resource* resource = nullptr;
    bool isExternal = false;
};

// Render Graph - manages render passes and resource transitions
class RenderGraph
{
public:
    RenderGraph(GraphicsDevice* device);
    ~RenderGraph();

    // Resource registration
    RGResourceHandle ImportTexture(const std::string& name, ID3D12Resource* texture, D3D12_RESOURCE_STATES initialState);
    RGResourceHandle ImportBuffer(const std::string& name, ID3D12Resource* buffer, D3D12_RESOURCE_STATES initialState);
    RGResourceHandle CreateTexture(const std::string& name, uint32_t width, uint32_t height, DXGI_FORMAT format);

    // Get resource by handle
    RGResourceDesc* GetResource(RGResourceHandle handle);
    ID3D12Resource* GetD3D12Resource(RGResourceHandle handle);

    // Pass management
    template<typename T, typename... Args>
    T* AddPass(Args&&... args)
    {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = pass.get();
        m_passes.push_back(std::move(pass));
        return ptr;
    }

    // Add lambda pass for quick prototyping
    LambdaRenderPass* AddLambdaPass(
        const std::string& name,
        LambdaRenderPass::SetupFunc setup,
        LambdaRenderPass::ExecuteFunc execute);

    // Compile the graph - analyze dependencies and order passes
    void Compile();

    // Execute all passes
    void Execute(CommandList* commandList, DescriptorHeap* srvHeap);

    // Reset for next frame (clears transient resources)
    void BeginFrame();
    void EndFrame();

    // Utility
    void TransitionResource(CommandList* commandList, RGResourceHandle handle, D3D12_RESOURCE_STATES newState);

private:
    void SetupPasses();
    void SortPasses();
    void InsertBarriers(CommandList* commandList, RenderPass* pass);

    GraphicsDevice* m_device = nullptr;

    // Resources
    std::vector<RGResourceDesc> m_resources;
    std::unordered_map<std::string, RGResourceHandle> m_resourceNameMap;

    // Passes
    std::vector<std::unique_ptr<RenderPass>> m_passes;
    std::vector<RenderPass*> m_sortedPasses;  // Execution order

    bool m_compiled = false;
};
