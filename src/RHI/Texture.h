#pragma once

#include "Resource.h"

enum class TextureUsage
{
    RenderTarget,
    DepthStencil,
    ShaderResource,
    UnorderedAccess
};

class Texture : public Resource
{
public:
    Texture(GraphicsDevice* device);
    ~Texture() override;

    // Creation
    bool Create(
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        TextureUsage usage,
        uint32_t mipLevels = 1,
        const float* clearColor = nullptr
    );

    bool CreateDepth(
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT
    );

    // Accessors
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetMipLevels() const { return m_mipLevels; }
    DXGI_FORMAT GetFormat() const { return m_format; }

    // Descriptor handles (set externally by descriptor heap manager)
    void SetRTV(const DescriptorHandle& handle) { m_rtv = handle; }
    void SetDSV(const DescriptorHandle& handle) { m_dsv = handle; }
    void SetSRV(const DescriptorHandle& handle) { m_srv = handle; }
    void SetUAV(const DescriptorHandle& handle) { m_uav = handle; }

    const DescriptorHandle& GetRTV() const { return m_rtv; }
    const DescriptorHandle& GetDSV() const { return m_dsv; }
    const DescriptorHandle& GetSRV() const { return m_srv; }
    const DescriptorHandle& GetUAV() const { return m_uav; }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_mipLevels = 1;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    TextureUsage m_usage;

    // Descriptor handles
    DescriptorHandle m_rtv;  // Render Target View
    DescriptorHandle m_dsv;  // Depth Stencil View
    DescriptorHandle m_srv;  // Shader Resource View
    DescriptorHandle m_uav;  // Unordered Access View

    D3D12_RESOURCE_FLAGS GetResourceFlags(TextureUsage usage) const;
};
