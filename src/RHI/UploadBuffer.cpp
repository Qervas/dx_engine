#include "UploadBuffer.h"
#include "Device.h"
#include <windows.h>

UploadBuffer::UploadBuffer(GraphicsDevice* device, uint32_t size)
    : m_device(device)
    , m_size(size)
{
}

UploadBuffer::~UploadBuffer()
{
    Shutdown();
}

bool UploadBuffer::Initialize()
{
    // Create upload buffer
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = m_size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create upload buffer", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_resource->SetName(L"Upload Buffer");
#endif

    // Map the buffer (keep it mapped for the lifetime)
    D3D12_RANGE readRange = { 0, 0 };  // We don't read from upload buffers
    hr = m_resource->Map(0, &readRange, &m_cpuAddress);

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to map upload buffer", L"Error", MB_OK);
        return false;
    }

    m_gpuAddress = m_resource->GetGPUVirtualAddress();

    return true;
}

void UploadBuffer::Shutdown()
{
    if (m_cpuAddress && m_resource)
    {
        m_resource->Unmap(0, nullptr);
        m_cpuAddress = nullptr;
    }

    m_resource.Reset();
}

UploadBuffer::Allocation UploadBuffer::Allocate(uint32_t size, uint32_t alignment)
{
    // Align current offset
    uint32_t alignedOffset = (m_currentOffset + (alignment - 1)) & ~(alignment - 1);

    // Check if we have space
    if (alignedOffset + size > m_size)
    {
        MessageBox(nullptr, L"Upload buffer is full", L"Error", MB_OK);
        return Allocation();
    }

    Allocation alloc;
    alloc.cpuAddress = static_cast<uint8_t*>(m_cpuAddress) + alignedOffset;
    alloc.gpuAddress = m_gpuAddress + alignedOffset;
    alloc.size = size;
    alloc.offset = alignedOffset;

    m_currentOffset = alignedOffset + size;

    return alloc;
}

void UploadBuffer::Reset()
{
    m_currentOffset = 0;
}
