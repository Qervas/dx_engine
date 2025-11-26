#include "RenderGraph.h"
#include <algorithm>

RenderGraph::RenderGraph(GraphicsDevice* device)
    : m_device(device)
{
}

RenderGraph::~RenderGraph()
{
}

RGResourceHandle RenderGraph::ImportTexture(const std::string& name, ID3D12Resource* texture, D3D12_RESOURCE_STATES initialState)
{
    RGResourceDesc desc;
    desc.name = name;
    desc.type = RGResourceType::External;
    desc.resource = texture;
    desc.currentState = initialState;
    desc.isExternal = true;

    // Get dimensions from resource
    D3D12_RESOURCE_DESC resDesc = texture->GetDesc();
    desc.width = static_cast<uint32_t>(resDesc.Width);
    desc.height = resDesc.Height;
    desc.format = resDesc.Format;

    RGResourceHandle handle;
    handle.index = static_cast<uint32_t>(m_resources.size());
    m_resources.push_back(desc);
    m_resourceNameMap[name] = handle;

    return handle;
}

RGResourceHandle RenderGraph::ImportBuffer(const std::string& name, ID3D12Resource* buffer, D3D12_RESOURCE_STATES initialState)
{
    RGResourceDesc desc;
    desc.name = name;
    desc.type = RGResourceType::External;
    desc.resource = buffer;
    desc.currentState = initialState;
    desc.isExternal = true;

    RGResourceHandle handle;
    handle.index = static_cast<uint32_t>(m_resources.size());
    m_resources.push_back(desc);
    m_resourceNameMap[name] = handle;

    return handle;
}

RGResourceHandle RenderGraph::CreateTexture(const std::string& name, uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    RGResourceDesc desc;
    desc.name = name;
    desc.type = RGResourceType::Texture;
    desc.width = width;
    desc.height = height;
    desc.format = format;
    desc.currentState = D3D12_RESOURCE_STATE_COMMON;
    desc.isExternal = false;

    // TODO: Actually create the texture resource here or lazily
    // For now, we'll handle this in the compile phase

    RGResourceHandle handle;
    handle.index = static_cast<uint32_t>(m_resources.size());
    m_resources.push_back(desc);
    m_resourceNameMap[name] = handle;

    return handle;
}

RGResourceDesc* RenderGraph::GetResource(RGResourceHandle handle)
{
    if (!handle.IsValid() || handle.index >= m_resources.size())
        return nullptr;
    return &m_resources[handle.index];
}

ID3D12Resource* RenderGraph::GetD3D12Resource(RGResourceHandle handle)
{
    RGResourceDesc* desc = GetResource(handle);
    return desc ? desc->resource : nullptr;
}

LambdaRenderPass* RenderGraph::AddLambdaPass(
    const std::string& name,
    LambdaRenderPass::SetupFunc setup,
    LambdaRenderPass::ExecuteFunc execute)
{
    auto pass = std::make_unique<LambdaRenderPass>(name, setup, execute);
    LambdaRenderPass* ptr = pass.get();
    m_passes.push_back(std::move(pass));
    return ptr;
}

void RenderGraph::Compile()
{
    if (m_compiled)
        return;

    // Setup all passes (let them declare dependencies)
    SetupPasses();

    // Sort passes based on dependencies
    SortPasses();

    m_compiled = true;
}

void RenderGraph::SetupPasses()
{
    for (auto& pass : m_passes)
    {
        pass->Setup(*this);
    }
}

void RenderGraph::SortPasses()
{
    // Simple topological sort based on resource dependencies
    // For now, we just use declaration order (assumes user adds passes in correct order)
    // A full implementation would analyze read/write dependencies

    m_sortedPasses.clear();
    for (auto& pass : m_passes)
    {
        if (pass->IsEnabled())
        {
            m_sortedPasses.push_back(pass.get());
        }
    }

    // TODO: Implement proper dependency analysis and topological sort
    // For now, passes execute in the order they were added
}

void RenderGraph::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_compiled)
    {
        Compile();
    }

    // Execute each pass with proper barriers
    for (RenderPass* pass : m_sortedPasses)
    {
        // Insert resource barriers for this pass
        InsertBarriers(commandList, pass);

        // Execute the pass
        pass->Execute(commandList, srvHeap);
    }
}

void RenderGraph::InsertBarriers(CommandList* commandList, RenderPass* pass)
{
    // Transition resources to required states for inputs
    for (const auto& input : pass->GetInputs())
    {
        TransitionResource(commandList, input.resource, input.requiredState);
    }

    // Transition resources to required states for outputs
    for (const auto& output : pass->GetOutputs())
    {
        TransitionResource(commandList, output.resource, output.requiredState);
    }
}

void RenderGraph::TransitionResource(CommandList* commandList, RGResourceHandle handle, D3D12_RESOURCE_STATES newState)
{
    RGResourceDesc* desc = GetResource(handle);
    if (!desc || !desc->resource)
        return;

    if (desc->currentState != newState)
    {
        commandList->TransitionBarrier(desc->resource, desc->currentState, newState);
        desc->currentState = newState;
    }
}

void RenderGraph::BeginFrame()
{
    // Reset transient resources for the new frame
    // For external resources, we might need to re-import their states
}

void RenderGraph::EndFrame()
{
    // Cleanup or state reset if needed
}
