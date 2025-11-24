#include "DescriptorHeap.h"
#include "Device.h"
#include <windows.h>

DescriptorHeap::DescriptorHeap(GraphicsDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, bool shaderVisible)
    : m_device(device)
    , m_type(type)
    , m_numDescriptors(numDescriptors)
    , m_shaderVisible(shaderVisible)
{
    m_descriptorSize = device->GetDescriptorSize(type);
    m_freeList.resize(numDescriptors, true);  // All descriptors start as free
}

DescriptorHeap::~DescriptorHeap()
{
    Shutdown();
}

bool DescriptorHeap::Initialize()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = m_type;
    desc.NumDescriptors = m_numDescriptors;
    desc.Flags = m_shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    HRESULT hr = m_device->GetD3D12Device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create descriptor heap", L"Error", MB_OK);
        return false;
    }

    m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();

    if (m_shaderVisible)
    {
        m_gpuStart = m_heap->GetGPUDescriptorHandleForHeapStart();
    }

#if defined(_DEBUG)
    // Set debug name
    switch (m_type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        m_heap->SetName(m_shaderVisible ? L"CBV_SRV_UAV Heap (Shader Visible)" : L"CBV_SRV_UAV Heap (CPU)");
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        m_heap->SetName(L"Sampler Heap");
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        m_heap->SetName(L"RTV Heap");
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
        m_heap->SetName(L"DSV Heap");
        break;
    }
#endif

    return true;
}

void DescriptorHeap::Shutdown()
{
    m_heap.Reset();
}

DescriptorHandle DescriptorHeap::Allocate()
{
    // Find first free descriptor
    for (uint32_t i = 0; i < m_numDescriptors; i++)
    {
        if (m_freeList[i])
        {
            m_freeList[i] = false;
            m_allocatedCount++;

            DescriptorHandle handle;
            handle.cpu = GetCPUHandle(i);
            handle.heapIndex = i;

            if (m_shaderVisible)
            {
                handle.gpu = GetGPUHandle(i);
            }

            return handle;
        }
    }

    // Out of descriptors
    MessageBox(nullptr, L"Descriptor heap is full", L"Error", MB_OK);
    return DescriptorHandle();
}

void DescriptorHeap::Free(const DescriptorHandle& handle)
{
    if (handle.heapIndex < m_numDescriptors)
    {
        if (!m_freeList[handle.heapIndex])
        {
            m_freeList[handle.heapIndex] = true;
            m_allocatedCount--;
        }
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(uint32_t index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cpuStart;
    handle.ptr += index * m_descriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(uint32_t index) const
{
    if (!m_shaderVisible)
    {
        return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_gpuStart;
    handle.ptr += index * m_descriptorSize;
    return handle;
}
