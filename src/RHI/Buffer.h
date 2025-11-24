#pragma once

#include "Resource.h"

enum class BufferUsage
{
    Vertex,
    Index,
    Constant,
    Structured,
    Upload,     // CPU-writable staging buffer
    Readback    // CPU-readable buffer
};

class Buffer : public Resource
{
public:
    Buffer(GraphicsDevice* device);
    ~Buffer() override;

    // Creation
    bool Create(
        uint32_t size,
        uint32_t stride,  // Element stride (0 for raw buffers)
        BufferUsage usage,
        const void* initialData = nullptr
    );

    // CPU access (only for Upload/Readback buffers)
    void* Map();
    void Unmap();
    void UpdateData(const void* data, uint32_t size, uint32_t offset = 0);

    // Accessors
    uint32_t GetSize() const { return m_size; }
    uint32_t GetStride() const { return m_stride; }
    uint32_t GetElementCount() const { return m_stride > 0 ? m_size / m_stride : 0; }
    BufferUsage GetUsage() const { return m_usage; }

    // Views
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(DXGI_FORMAT format = DXGI_FORMAT_R32_UINT) const;
    D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantBufferView() const;

    // Descriptor handles (set externally by descriptor heap manager)
    void SetCBV(const DescriptorHandle& handle) { m_cbv = handle; }
    void SetSRV(const DescriptorHandle& handle) { m_srv = handle; }
    void SetUAV(const DescriptorHandle& handle) { m_uav = handle; }

    const DescriptorHandle& GetCBV() const { return m_cbv; }
    const DescriptorHandle& GetSRV() const { return m_srv; }
    const DescriptorHandle& GetUAV() const { return m_uav; }

private:
    uint32_t m_size = 0;
    uint32_t m_stride = 0;
    BufferUsage m_usage;
    void* m_mappedData = nullptr;

    // Descriptor handles
    DescriptorHandle m_cbv;  // Constant Buffer View
    DescriptorHandle m_srv;  // Shader Resource View
    DescriptorHandle m_uav;  // Unordered Access View

    bool CreateInternal(
        uint32_t size,
        D3D12_HEAP_TYPE heapType,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_RESOURCE_STATES initialState
    );
};
