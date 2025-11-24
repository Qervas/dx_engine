#pragma once

#include "../Core/Types.h"
#include <d3d12.h>
#include <dxgi1_6.h>

class GraphicsDevice
{
public:
    GraphicsDevice();
    ~GraphicsDevice();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Accessors
    ID3D12Device* GetD3D12Device() const { return m_device.Get(); }
    IDXGIFactory4* GetDXGIFactory() const { return m_factory.Get(); }

    // Feature queries
    bool IsRaytracingSupported() const { return m_raytracingSupported; }
    bool IsMeshShaderSupported() const { return m_meshShaderSupported; }

    // Descriptor sizes
    uint32_t GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

private:
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;

    // Feature support flags
    bool m_raytracingSupported = false;
    bool m_meshShaderSupported = false;

    // Descriptor sizes (cached)
    uint32_t m_rtvDescriptorSize = 0;
    uint32_t m_dsvDescriptorSize = 0;
    uint32_t m_cbvSrvUavDescriptorSize = 0;
    uint32_t m_samplerDescriptorSize = 0;

    // Helper methods
    bool CreateDevice();
    void QueryFeatureSupport();
};
