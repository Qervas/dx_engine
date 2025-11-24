#include "Resource.h"
#include "Device.h"
#include <windows.h>

Resource::Resource(GraphicsDevice* device)
    : m_device(device)
    , m_currentState(D3D12_RESOURCE_STATE_COMMON)
{
}

Resource::~Resource()
{
    // ComPtr will automatically release the resource
}

D3D12_GPU_VIRTUAL_ADDRESS Resource::GetGPUVirtualAddress() const
{
    if (m_resource)
    {
        return m_resource->GetGPUVirtualAddress();
    }
    return 0;
}

void Resource::SetName(const std::wstring& name)
{
    m_name = name;
    if (m_resource)
    {
        m_resource->SetName(name.c_str());
    }
}

bool Resource::CreateCommittedResource(
    const D3D12_HEAP_PROPERTIES& heapProps,
    D3D12_HEAP_FLAGS heapFlags,
    const D3D12_RESOURCE_DESC& desc,
    D3D12_RESOURCE_STATES initialState,
    const D3D12_CLEAR_VALUE* clearValue)
{
    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        heapFlags,
        &desc,
        initialState,
        clearValue,
        IID_PPV_ARGS(&m_resource)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create committed resource", L"Error", MB_OK);
        return false;
    }

    m_currentState = initialState;
    return true;
}
