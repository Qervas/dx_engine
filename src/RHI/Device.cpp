#include "Device.h"
#include <windows.h>

GraphicsDevice::GraphicsDevice()
{
}

GraphicsDevice::~GraphicsDevice()
{
    Shutdown();
}

bool GraphicsDevice::Initialize()
{
    // Enable debug layer in debug builds
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        // Enable GPU-based validation (catches more errors, but slower)
        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1)))
        {
            debugController1->SetEnableGPUBasedValidation(true);
        }
    }
#endif

    // Create DXGI factory
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_factory));
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create DXGI factory", L"Error", MB_OK);
        return false;
    }

    // Create device
    if (!CreateDevice())
    {
        return false;
    }

    // Query feature support
    QueryFeatureSupport();

    // Cache descriptor sizes
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    return true;
}

void GraphicsDevice::Shutdown()
{
    // COM objects will be released automatically by ComPtr
    m_device.Reset();
    m_factory.Reset();
    m_adapter.Reset();
}

uint32_t GraphicsDevice::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        return m_rtvDescriptorSize;
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
        return m_dsvDescriptorSize;
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return m_cbvSrvUavDescriptorSize;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return m_samplerDescriptorSize;
    default:
        return 0;
    }
}

bool GraphicsDevice::CreateDevice()
{
    // Try to find a hardware adapter
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0;
         DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &adapter);
         ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Skip software adapter
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        // Try to create device with this adapter
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
        {
            m_adapter = adapter;
            break;
        }
    }

    if (!m_device)
    {
        // Fallback to WARP (software) adapter
        ComPtr<IDXGIAdapter> warpAdapter;
        if (FAILED(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))))
        {
            MessageBox(nullptr, L"Failed to find any adapter", L"Error", MB_OK);
            return false;
        }

        if (FAILED(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
        {
            MessageBox(nullptr, L"Failed to create D3D12 device", L"Error", MB_OK);
            return false;
        }
    }

    // Set debug device name
#if defined(_DEBUG)
    m_device->SetName(L"Main Graphics Device");
#endif

    return true;
}

void GraphicsDevice::QueryFeatureSupport()
{
    // Query raytracing support (DXR)
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
    if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(features5))))
    {
        m_raytracingSupported = (features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
    }

    // Query mesh shader support
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 features7 = {};
    if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features7, sizeof(features7))))
    {
        m_meshShaderSupported = (features7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
    }
}
