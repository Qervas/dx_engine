#include "Texture.h"
#include "Device.h"
#include <windows.h>

Texture::Texture(GraphicsDevice* device)
    : Resource(device)
    , m_usage(TextureUsage::ShaderResource)
{
}

Texture::~Texture()
{
}

bool Texture::Create(
    uint32_t width,
    uint32_t height,
    DXGI_FORMAT format,
    TextureUsage usage,
    uint32_t mipLevels,
    const float* clearColor)
{
    m_width = width;
    m_height = height;
    m_format = format;
    m_usage = usage;
    m_mipLevels = mipLevels;

    // Create texture description
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = static_cast<UINT16>(mipLevels);
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = GetResourceFlags(usage);

    // Determine heap properties and initial state
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_STATES initialState;
    D3D12_CLEAR_VALUE clearValue = {};
    D3D12_CLEAR_VALUE* pClearValue = nullptr;

    switch (usage)
    {
    case TextureUsage::RenderTarget:
        initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (clearColor)
        {
            clearValue.Format = format;
            clearValue.Color[0] = clearColor[0];
            clearValue.Color[1] = clearColor[1];
            clearValue.Color[2] = clearColor[2];
            clearValue.Color[3] = clearColor[3];
            pClearValue = &clearValue;
        }
        break;

    case TextureUsage::DepthStencil:
        initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        clearValue.Format = format;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClearValue = &clearValue;
        break;

    case TextureUsage::ShaderResource:
        initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        break;

    case TextureUsage::UnorderedAccess:
        initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        break;

    default:
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    }

    return CreateCommittedResource(
        heapProps,
        D3D12_HEAP_FLAG_NONE,
        desc,
        initialState,
        pClearValue
    );
}

bool Texture::CreateDepth(uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    return Create(width, height, format, TextureUsage::DepthStencil);
}

D3D12_RESOURCE_FLAGS Texture::GetResourceFlags(TextureUsage usage) const
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    switch (usage)
    {
    case TextureUsage::RenderTarget:
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        break;

    case TextureUsage::DepthStencil:
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        // Depth stencils can't be shader resources without special handling
        // For simplicity, we'll deny shader resource access
        flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        break;

    case TextureUsage::UnorderedAccess:
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        break;

    case TextureUsage::ShaderResource:
    default:
        // No special flags needed
        break;
    }

    return flags;
}
