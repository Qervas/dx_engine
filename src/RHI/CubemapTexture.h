#pragma once

#include "Resource.h"

class GraphicsDevice;
class DescriptorHeap;

// Cubemap face order (matches D3D12 convention)
enum class CubemapFace : uint32_t
{
    PositiveX = 0,  // Right
    NegativeX = 1,  // Left
    PositiveY = 2,  // Up
    NegativeZ = 3,  // Back (note: D3D uses +Y as up, +Z as forward)
    NegativeY = 4,  // Down
    PositiveZ = 5   // Front
};

class CubemapTexture : public Resource
{
public:
    CubemapTexture(GraphicsDevice* device);
    ~CubemapTexture() override;

    // Create empty cubemap texture
    bool Create(
        uint32_t size,
        DXGI_FORMAT format,
        uint32_t mipLevels = 1
    );

    // Create SRV for shader access
    bool CreateSRV(DescriptorHeap* srvHeap);

    // Accessors
    uint32_t GetSize() const { return m_size; }
    uint32_t GetMipLevels() const { return m_mipLevels; }
    DXGI_FORMAT GetFormat() const { return m_format; }

    void SetSRV(const DescriptorHandle& handle) { m_srv = handle; }
    const DescriptorHandle& GetSRV() const { return m_srv; }

    // Get subresource index for a specific face and mip level
    uint32_t GetSubresourceIndex(CubemapFace face, uint32_t mipLevel = 0) const
    {
        return static_cast<uint32_t>(face) * m_mipLevels + mipLevel;
    }

private:
    uint32_t m_size = 0;        // Width and height (cubemaps are square)
    uint32_t m_mipLevels = 1;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    DescriptorHandle m_srv;
};
