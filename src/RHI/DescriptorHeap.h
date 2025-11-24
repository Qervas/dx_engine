#pragma once

#include "../Core/Types.h"
#include "Resource.h"
#include <d3d12.h>
#include <vector>

class GraphicsDevice;

class DescriptorHeap
{
public:
    DescriptorHeap(GraphicsDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, bool shaderVisible = false);
    ~DescriptorHeap();

    bool Initialize();
    void Shutdown();

    // Allocation
    DescriptorHandle Allocate();
    void Free(const DescriptorHandle& handle);

    // Accessors
    ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return m_heap.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const;
    bool IsShaderVisible() const { return m_shaderVisible; }

    // Statistics
    uint32_t GetAllocatedCount() const { return m_allocatedCount; }
    uint32_t GetTotalCount() const { return m_numDescriptors; }
    uint32_t GetFreeCount() const { return m_numDescriptors - m_allocatedCount; }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    uint32_t m_numDescriptors;
    uint32_t m_descriptorSize;
    bool m_shaderVisible;

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart;

    // Simple allocation tracking
    uint32_t m_allocatedCount = 0;
    std::vector<bool> m_freeList;  // true = free, false = allocated
};
