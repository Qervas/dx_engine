#include "GBuffer.h"
#include "../RHI/Device.h"

GBuffer::GBuffer(GraphicsDevice* device)
    : m_device(device)
{
}

GBuffer::~GBuffer()
{
    ReleaseTextures();
}

bool GBuffer::Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    m_width = width;
    m_height = height;

    return CreateTextures(srvHeap, rtvHeap);
}

void GBuffer::Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    if (m_width == width && m_height == height)
        return;

    ReleaseTextures();
    m_width = width;
    m_height = height;
    CreateTextures(srvHeap, rtvHeap);
}

bool GBuffer::CreateTextures(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Create position texture (view-space positions)
    m_positionTexture = std::make_unique<Texture>(m_device);
    if (!m_positionTexture->Create(m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureUsage::RenderTarget, 1, clearColor))
    {
        return false;
    }

    // Create normal texture (view-space normals)
    m_normalTexture = std::make_unique<Texture>(m_device);
    if (!m_normalTexture->Create(m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureUsage::RenderTarget, 1, clearColor))
    {
        return false;
    }

    // Create RTVs
    m_positionRTV = rtvHeap->Allocate();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    m_device->GetD3D12Device()->CreateRenderTargetView(
        m_positionTexture->GetD3D12Resource(),
        &rtvDesc,
        m_positionRTV.cpu
    );

    m_normalRTV = rtvHeap->Allocate();
    m_device->GetD3D12Device()->CreateRenderTargetView(
        m_normalTexture->GetD3D12Resource(),
        &rtvDesc,
        m_normalRTV.cpu
    );

    // Create SRVs
    m_positionSRV = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_positionTexture->GetD3D12Resource(),
        &srvDesc,
        m_positionSRV.cpu
    );

    m_normalSRV = srvHeap->Allocate();
    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_normalTexture->GetD3D12Resource(),
        &srvDesc,
        m_normalSRV.cpu
    );

    return true;
}

void GBuffer::ReleaseTextures()
{
    m_positionTexture.reset();
    m_normalTexture.reset();
}

void GBuffer::Clear(ID3D12GraphicsCommandList* commandList)
{
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(m_positionRTV.cpu, clearColor, 0, nullptr);
    commandList->ClearRenderTargetView(m_normalRTV.cpu, clearColor, 0, nullptr);
}

void GBuffer::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList)
{
    D3D12_RESOURCE_BARRIER barriers[2] = {};

    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = m_positionTexture->GetD3D12Resource();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = m_normalTexture->GetD3D12Resource();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(2, barriers);
}

void GBuffer::TransitionToShaderResource(ID3D12GraphicsCommandList* commandList)
{
    D3D12_RESOURCE_BARRIER barriers[2] = {};

    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = m_positionTexture->GetD3D12Resource();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = m_normalTexture->GetD3D12Resource();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(2, barriers);
}
