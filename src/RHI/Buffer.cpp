#include "Buffer.h"
#include "Device.h"
#include <windows.h>

Buffer::Buffer(GraphicsDevice* device)
    : Resource(device)
    , m_usage(BufferUsage::Vertex)
{
}

Buffer::~Buffer()
{
    if (m_mappedData)
    {
        Unmap();
    }
}

bool Buffer::Create(
    uint32_t size,
    uint32_t stride,
    BufferUsage usage,
    const void* initialData)
{
    m_size = size;
    m_stride = stride;
    m_usage = usage;

    D3D12_HEAP_TYPE heapType;
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_RESOURCE_STATES initialState;

    switch (usage)
    {
    case BufferUsage::Vertex:
    case BufferUsage::Index:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;  // Will upload data
        break;

    case BufferUsage::Constant:
        // Constant buffers are typically in upload heap for frequent updates
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;

    case BufferUsage::Structured:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;

    case BufferUsage::Upload:
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;

    case BufferUsage::Readback:
        heapType = D3D12_HEAP_TYPE_READBACK;
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;

    default:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    }

    if (!CreateInternal(size, heapType, flags, initialState))
    {
        return false;
    }

    // Upload initial data if provided
    if (initialData && heapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        // For upload heaps, we can directly map and copy
        void* mappedData = Map();
        if (mappedData)
        {
            memcpy(mappedData, initialData, size);
            Unmap();
        }
    }
    else if (initialData && heapType == D3D12_HEAP_TYPE_DEFAULT)
    {
        // For default heaps, we need an upload buffer and copy command
        // This will be handled by a higher-level system later
        // For now, we'll just note that initial data upload is deferred
    }

    return true;
}

void* Buffer::Map()
{
    if (m_mappedData)
    {
        return m_mappedData;  // Already mapped
    }

    // Only upload and readback heaps can be mapped
    if (m_usage != BufferUsage::Upload &&
        m_usage != BufferUsage::Readback &&
        m_usage != BufferUsage::Constant)
    {
        return nullptr;
    }

    D3D12_RANGE readRange = { 0, 0 };  // We don't intend to read from this resource on the CPU
    HRESULT hr = m_resource->Map(0, &readRange, &m_mappedData);

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to map buffer", L"Error", MB_OK);
        return nullptr;
    }

    return m_mappedData;
}

void Buffer::Unmap()
{
    if (m_mappedData)
    {
        m_resource->Unmap(0, nullptr);
        m_mappedData = nullptr;
    }
}

void Buffer::UpdateData(const void* data, uint32_t size, uint32_t offset)
{
    void* dest = Map();
    if (dest)
    {
        memcpy(static_cast<uint8_t*>(dest) + offset, data, size);
        // Note: We're not unmapping here to allow for multiple updates
        // The caller should unmap when done, or it will unmap in destructor
    }
}

D3D12_VERTEX_BUFFER_VIEW Buffer::GetVertexBufferView() const
{
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = GetGPUVirtualAddress();
    vbv.SizeInBytes = m_size;
    vbv.StrideInBytes = m_stride;
    return vbv;
}

D3D12_INDEX_BUFFER_VIEW Buffer::GetIndexBufferView(DXGI_FORMAT format) const
{
    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation = GetGPUVirtualAddress();
    ibv.SizeInBytes = m_size;
    ibv.Format = format;
    return ibv;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC Buffer::GetConstantBufferView() const
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
    cbv.BufferLocation = GetGPUVirtualAddress();
    cbv.SizeInBytes = (m_size + 255) & ~255;  // Align to 256 bytes
    return cbv;
}

bool Buffer::CreateInternal(
    uint32_t size,
    D3D12_HEAP_TYPE heapType,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES initialState)
{
    // Align size to 256 bytes for constant buffers
    if (m_usage == BufferUsage::Constant)
    {
        size = (size + 255) & ~255;
        m_size = size;
    }

    // Create buffer description
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;

    // Create heap properties
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    return CreateCommittedResource(
        heapProps,
        D3D12_HEAP_FLAG_NONE,
        desc,
        initialState,
        nullptr
    );
}
