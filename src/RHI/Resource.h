#pragma once

#include "../Core/Types.h"
#include <d3d12.h>
#include <string>

class GraphicsDevice;

// Base class for all GPU resources
class Resource
{
public:
    Resource(GraphicsDevice* device);
    virtual ~Resource();

    // State management
    D3D12_RESOURCE_STATES GetState() const { return m_currentState; }
    void SetState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

    // Accessors
    ID3D12Resource* GetD3D12Resource() const { return m_resource.Get(); }
    ID3D12Resource** GetAddressOfResource() { return m_resource.GetAddressOf(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

    // Debug naming
    void SetName(const std::wstring& name);
    const std::wstring& GetName() const { return m_name; }

protected:
    GraphicsDevice* m_device;
    ComPtr<ID3D12Resource> m_resource;
    D3D12_RESOURCE_STATES m_currentState;
    std::wstring m_name;

    // Helper for subclasses
    bool CreateCommittedResource(
        const D3D12_HEAP_PROPERTIES& heapProps,
        D3D12_HEAP_FLAGS heapFlags,
        const D3D12_RESOURCE_DESC& desc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* clearValue = nullptr
    );
};

// Descriptor handle that can reference CPU and/or GPU descriptors
struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE gpu = { 0 };
    uint32_t heapIndex = 0;

    bool IsValid() const { return cpu.ptr != 0; }
    bool IsShaderVisible() const { return gpu.ptr != 0; }
};
