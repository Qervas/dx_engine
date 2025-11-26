#include "CubemapTexture.h"
#include "Device.h"
#include "DescriptorHeap.h"

CubemapTexture::CubemapTexture(GraphicsDevice* device)
    : Resource(device)
{
}

CubemapTexture::~CubemapTexture()
{
}

bool CubemapTexture::Create(uint32_t size, DXGI_FORMAT format, uint32_t mipLevels)
{
    m_size = size;
    m_format = format;
    m_mipLevels = mipLevels;

    // Create texture description for a cubemap (6 array slices)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = size;
    desc.DepthOrArraySize = 6;  // 6 faces for cubemap
    desc.MipLevels = static_cast<UINT16>(mipLevels);
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    return CreateCommittedResource(
        heapProps,
        D3D12_HEAP_FLAG_NONE,
        desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr
    );
}

bool CubemapTexture::CreateSRV(DescriptorHeap* srvHeap)
{
    if (!m_resource || !srvHeap)
        return false;

    m_srv = srvHeap->Allocate();
    if (!m_srv.IsValid())
        return false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = m_format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = m_mipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_resource.Get(),
        &srvDesc,
        m_srv.cpu
    );

    return true;
}
