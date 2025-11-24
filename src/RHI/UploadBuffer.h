#pragma once

#include "../Core/Types.h"
#include <d3d12.h>

class GraphicsDevice;
class CommandQueue;

// Simple upload buffer for dynamic data (CPU -> GPU)
// This is a basic version; a full ring buffer implementation will come later
class UploadBuffer
{
public:
    UploadBuffer(GraphicsDevice* device, uint32_t size);
    ~UploadBuffer();

    bool Initialize();
    void Shutdown();

    // Allocation (simple linear allocator for now)
    struct Allocation
    {
        void* cpuAddress = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
        uint32_t size = 0;
        uint32_t offset = 0;
    };

    Allocation Allocate(uint32_t size, uint32_t alignment = 256);
    void Reset();  // Reset allocator (call once per frame)

    // Accessors
    ID3D12Resource* GetResource() const { return m_resource.Get(); }
    uint32_t GetSize() const { return m_size; }
    uint32_t GetUsedSize() const { return m_currentOffset; }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12Resource> m_resource;
    uint32_t m_size;
    uint32_t m_currentOffset = 0;
    void* m_cpuAddress = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress = 0;
};
