#include "TextureLoader.h"
#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/Buffer.h"
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture* TextureLoader::LoadFromFile(
    GraphicsDevice* device,
    CommandList* commandList,
    DescriptorHeap* srvHeap,
    const std::string& filePath)
{
    // Load image using STB
    int width, height, channels;
    unsigned char* imageData = stbi_load(filePath.c_str(), &width, &height, &channels, 4); // Force RGBA

    if (!imageData)
    {
        MessageBoxA(nullptr, ("Failed to load texture: " + filePath).c_str(), "Error", MB_OK);
        return nullptr;
    }

    // Create texture
    Texture* texture = new Texture(device);
    if (!texture->Create(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource))
    {
        stbi_image_free(imageData);
        delete texture;
        return nullptr;
    }

    // Create upload buffer for texture data
    uint32_t uploadSize = width * height * 4; // RGBA
    Buffer uploadBuffer(device);
    if (!uploadBuffer.Create(uploadSize, 0, BufferUsage::Upload, imageData))
    {
        stbi_image_free(imageData);
        delete texture;
        return nullptr;
    }

    // Copy from upload buffer to texture
    commandList->TransitionBarrier(
        texture->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = uploadBuffer.GetD3D12Resource();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint.Offset = 0;
    srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcLocation.PlacedFootprint.Footprint.Width = width;
    srcLocation.PlacedFootprint.Footprint.Height = height;
    srcLocation.PlacedFootprint.Footprint.Depth = 1;
    srcLocation.PlacedFootprint.Footprint.RowPitch = width * 4;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = texture->GetD3D12Resource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    commandList->GetD3D12CommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    commandList->TransitionBarrier(
        texture->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    // Create SRV
    DescriptorHandle srvHandle = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    device->GetD3D12Device()->CreateShaderResourceView(
        texture->GetD3D12Resource(),
        &srvDesc,
        srvHandle.cpu
    );

    texture->SetSRV(srvHandle);

    // Free STB image data
    stbi_image_free(imageData);

    return texture;
}
