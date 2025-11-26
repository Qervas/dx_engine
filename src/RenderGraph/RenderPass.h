#pragma once

#include "../RHI/CommandList.h"
#include "../RHI/DescriptorHeap.h"
#include <string>
#include <vector>
#include <functional>

// Forward declarations
class RenderGraph;
class RenderGraphResource;

// Resource access types
enum class ResourceAccess
{
    Read,           // Shader read (SRV)
    Write,          // Render target or UAV write
    ReadWrite,      // Both read and write (UAV)
    DepthRead,      // Depth buffer read
    DepthWrite      // Depth buffer write
};

// Resource handle for render graph
struct RGResourceHandle
{
    uint32_t index = UINT32_MAX;
    bool IsValid() const { return index != UINT32_MAX; }
};

// Resource dependency declaration
struct ResourceDependency
{
    RGResourceHandle resource;
    ResourceAccess access;
    D3D12_RESOURCE_STATES requiredState;
};

// Base class for all render passes
class RenderPass
{
public:
    RenderPass(const std::string& name) : m_name(name) {}
    virtual ~RenderPass() = default;

    // Setup phase - declare resource dependencies
    virtual void Setup(RenderGraph& graph) = 0;

    // Execute phase - record commands
    virtual void Execute(CommandList* commandList, DescriptorHeap* srvHeap) = 0;

    // Accessors
    const std::string& GetName() const { return m_name; }
    const std::vector<ResourceDependency>& GetInputs() const { return m_inputs; }
    const std::vector<ResourceDependency>& GetOutputs() const { return m_outputs; }

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

protected:
    // Called by derived classes during Setup
    void AddInput(RGResourceHandle resource, ResourceAccess access, D3D12_RESOURCE_STATES state);
    void AddOutput(RGResourceHandle resource, ResourceAccess access, D3D12_RESOURCE_STATES state);

    std::string m_name;
    std::vector<ResourceDependency> m_inputs;
    std::vector<ResourceDependency> m_outputs;
    bool m_enabled = true;
};

// Lambda-based render pass for quick prototyping
class LambdaRenderPass : public RenderPass
{
public:
    using SetupFunc = std::function<void(RenderGraph&, LambdaRenderPass&)>;
    using ExecuteFunc = std::function<void(CommandList*, DescriptorHeap*)>;

    LambdaRenderPass(const std::string& name, SetupFunc setup, ExecuteFunc execute)
        : RenderPass(name)
        , m_setupFunc(setup)
        , m_executeFunc(execute)
    {}

    void Setup(RenderGraph& graph) override
    {
        if (m_setupFunc) m_setupFunc(graph, *this);
    }

    void Execute(CommandList* commandList, DescriptorHeap* srvHeap) override
    {
        if (m_executeFunc) m_executeFunc(commandList, srvHeap);
    }

    // Public access to add dependencies (for lambda setup)
    void DeclareInput(RGResourceHandle resource, ResourceAccess access, D3D12_RESOURCE_STATES state)
    {
        AddInput(resource, access, state);
    }

    void DeclareOutput(RGResourceHandle resource, ResourceAccess access, D3D12_RESOURCE_STATES state)
    {
        AddOutput(resource, access, state);
    }

private:
    SetupFunc m_setupFunc;
    ExecuteFunc m_executeFunc;
};
