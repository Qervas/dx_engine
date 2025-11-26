#include "CubemapLoader.h"
#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/CommandQueue.h"
#include "../RHI/Buffer.h"
#include <stb_image.h>
#include <windows.h>
#include <cmath>
#include <algorithm>

CubemapTexture* CubemapLoader::LoadFromFiles(
    GraphicsDevice* device,
    CommandList* commandList,
    CommandQueue* commandQueue,
    DescriptorHeap* srvHeap,
    const std::array<std::string, 6>& facePaths)
{
    // Load all 6 faces
    struct FaceData
    {
        unsigned char* data = nullptr;
        int width = 0;
        int height = 0;
    };
    std::array<FaceData, 6> faces;

    int cubemapSize = 0;

    for (int i = 0; i < 6; ++i)
    {
        int channels;
        faces[i].data = stbi_load(facePaths[i].c_str(), &faces[i].width, &faces[i].height, &channels, 4);

        if (!faces[i].data)
        {
            MessageBoxA(nullptr, ("Failed to load cubemap face: " + facePaths[i]).c_str(), "Error", MB_OK);
            // Clean up already loaded faces
            for (int j = 0; j < i; ++j)
            {
                stbi_image_free(faces[j].data);
            }
            return nullptr;
        }

        // Verify all faces are the same size (and square)
        if (i == 0)
        {
            cubemapSize = faces[i].width;
            if (faces[i].width != faces[i].height)
            {
                MessageBoxA(nullptr, "Cubemap faces must be square", "Error", MB_OK);
                for (int j = 0; j <= i; ++j)
                {
                    stbi_image_free(faces[j].data);
                }
                return nullptr;
            }
        }
        else if (faces[i].width != cubemapSize || faces[i].height != cubemapSize)
        {
            MessageBoxA(nullptr, "All cubemap faces must be the same size", "Error", MB_OK);
            for (int j = 0; j <= i; ++j)
            {
                stbi_image_free(faces[j].data);
            }
            return nullptr;
        }
    }

    // Create cubemap texture
    CubemapTexture* cubemap = new CubemapTexture(device);
    if (!cubemap->Create(cubemapSize, DXGI_FORMAT_R8G8B8A8_UNORM, 1))
    {
        for (int i = 0; i < 6; ++i)
        {
            stbi_image_free(faces[i].data);
        }
        delete cubemap;
        return nullptr;
    }

    // Calculate row pitch (must be 256-byte aligned for D3D12)
    uint32_t rowPitch = cubemapSize * 4;
    uint32_t alignedRowPitch = (rowPitch + 255) & ~255;
    uint32_t faceSize = alignedRowPitch * cubemapSize;

    // Create upload buffer for all faces
    uint32_t totalUploadSize = faceSize * 6;
    Buffer uploadBuffer(device);
    if (!uploadBuffer.Create(totalUploadSize, 0, BufferUsage::Upload))
    {
        for (int i = 0; i < 6; ++i)
        {
            stbi_image_free(faces[i].data);
        }
        delete cubemap;
        return nullptr;
    }

    // Map and copy data with proper row pitch alignment
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    uploadBuffer.GetD3D12Resource()->Map(0, &readRange, &mappedData);

    for (int face = 0; face < 6; ++face)
    {
        uint8_t* dstFaceStart = static_cast<uint8_t*>(mappedData) + face * faceSize;
        for (int row = 0; row < cubemapSize; ++row)
        {
            memcpy(dstFaceStart + row * alignedRowPitch,
                   faces[face].data + row * rowPitch,
                   rowPitch);
        }
    }

    uploadBuffer.GetD3D12Resource()->Unmap(0, nullptr);

    // Record copy commands
    commandList->Reset();

    commandList->TransitionBarrier(
        cubemap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    // Copy each face
    for (int face = 0; face < 6; ++face)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer.GetD3D12Resource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Offset = face * faceSize;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = cubemapSize;
        srcLocation.PlacedFootprint.Footprint.Height = cubemapSize;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = alignedRowPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = cubemap->GetD3D12Resource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = face;  // Each face is a subresource

        commandList->GetD3D12CommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    commandList->TransitionBarrier(
        cubemap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->Close();

    // Execute and wait
    ID3D12CommandList* commandLists[] = { commandList->GetD3D12CommandList() };
    commandQueue->ExecuteCommandLists(commandLists, 1);
    commandQueue->Flush();

    // Create SRV
    if (!cubemap->CreateSRV(srvHeap))
    {
        for (int i = 0; i < 6; ++i)
        {
            stbi_image_free(faces[i].data);
        }
        delete cubemap;
        return nullptr;
    }

    // Free image data
    for (int i = 0; i < 6; ++i)
    {
        stbi_image_free(faces[i].data);
    }

    return cubemap;
}

CubemapTexture* CubemapLoader::LoadFromFolder(
    GraphicsDevice* device,
    CommandList* commandList,
    CommandQueue* commandQueue,
    DescriptorHeap* srvHeap,
    const std::string& folderPath,
    const std::string& extension)
{
    std::string basePath = folderPath;
    if (!basePath.empty() && basePath.back() != '/' && basePath.back() != '\\')
    {
        basePath += "/";
    }

    // Try standard naming conventions
    std::array<std::string, 6> facePaths;

    // Try px/nx/py/ny/pz/nz naming first
    facePaths[0] = basePath + "px" + extension;
    facePaths[1] = basePath + "nx" + extension;
    facePaths[2] = basePath + "py" + extension;
    facePaths[3] = basePath + "ny" + extension;
    facePaths[4] = basePath + "pz" + extension;
    facePaths[5] = basePath + "nz" + extension;

    // Check if first file exists, if not try right/left/top/bottom/front/back
    FILE* test = fopen(facePaths[0].c_str(), "rb");
    if (!test)
    {
        facePaths[0] = basePath + "right" + extension;
        facePaths[1] = basePath + "left" + extension;
        facePaths[2] = basePath + "top" + extension;
        facePaths[3] = basePath + "bottom" + extension;
        facePaths[4] = basePath + "front" + extension;
        facePaths[5] = basePath + "back" + extension;
    }
    else
    {
        fclose(test);
    }

    return LoadFromFiles(device, commandList, commandQueue, srvHeap, facePaths);
}

CubemapTexture* CubemapLoader::GenerateGradientSky(
    GraphicsDevice* device,
    CommandList* commandList,
    CommandQueue* commandQueue,
    DescriptorHeap* srvHeap,
    uint32_t size)
{
    // Create cubemap texture
    CubemapTexture* cubemap = new CubemapTexture(device);
    if (!cubemap->Create(size, DXGI_FORMAT_R8G8B8A8_UNORM, 1))
    {
        delete cubemap;
        return nullptr;
    }

    // Calculate row pitch (must be 256-byte aligned for D3D12)
    uint32_t rowPitch = size * 4;
    uint32_t alignedRowPitch = (rowPitch + 255) & ~255;
    uint32_t faceSize = alignedRowPitch * size;

    // Create upload buffer for all faces
    uint32_t totalUploadSize = faceSize * 6;
    Buffer uploadBuffer(device);
    if (!uploadBuffer.Create(totalUploadSize, 0, BufferUsage::Upload))
    {
        delete cubemap;
        return nullptr;
    }

    // Sky colors
    float horizonColor[3] = { 0.7f, 0.8f, 0.9f };   // Light blue-gray at horizon
    float zenithColor[3] = { 0.2f, 0.4f, 0.8f };    // Deeper blue at zenith
    float nadirColor[3] = { 0.3f, 0.25f, 0.2f };    // Brown-gray below horizon

    // Map and generate procedural sky
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    uploadBuffer.GetD3D12Resource()->Map(0, &readRange, &mappedData);

    for (int face = 0; face < 6; ++face)
    {
        uint8_t* faceData = static_cast<uint8_t*>(mappedData) + face * faceSize;

        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                // Calculate direction for this texel
                float u = (x + 0.5f) / size * 2.0f - 1.0f;
                float v = (y + 0.5f) / size * 2.0f - 1.0f;

                float dirX, dirY, dirZ;
                switch (face)
                {
                case 0: dirX = 1.0f;  dirY = -v;    dirZ = -u;    break;  // +X
                case 1: dirX = -1.0f; dirY = -v;    dirZ = u;     break;  // -X
                case 2: dirX = u;     dirY = 1.0f;  dirZ = v;     break;  // +Y
                case 3: dirX = u;     dirY = -1.0f; dirZ = -v;    break;  // -Y
                case 4: dirX = u;     dirY = -v;    dirZ = 1.0f;  break;  // +Z
                case 5: dirX = -u;    dirY = -v;    dirZ = -1.0f; break;  // -Z
                }

                // Normalize direction
                float len = sqrtf(dirX * dirX + dirY * dirY + dirZ * dirZ);
                dirY /= len;

                // Compute sky color based on vertical direction
                float r, g, b;
                if (dirY >= 0.0f)
                {
                    // Above horizon: blend from horizon to zenith
                    float t = dirY;
                    t = t * t;  // Non-linear falloff
                    r = horizonColor[0] + t * (zenithColor[0] - horizonColor[0]);
                    g = horizonColor[1] + t * (zenithColor[1] - horizonColor[1]);
                    b = horizonColor[2] + t * (zenithColor[2] - horizonColor[2]);
                }
                else
                {
                    // Below horizon: blend from horizon to nadir
                    float t = -dirY;
                    t = t * t;  // Non-linear falloff
                    r = horizonColor[0] + t * (nadirColor[0] - horizonColor[0]);
                    g = horizonColor[1] + t * (nadirColor[1] - horizonColor[1]);
                    b = horizonColor[2] + t * (nadirColor[2] - horizonColor[2]);
                }

                // Write pixel (use (std::min) to avoid Windows macro)
                uint8_t* pixel = faceData + y * alignedRowPitch + x * 4;
                pixel[0] = static_cast<uint8_t>((std::min)(1.0f, r) * 255.0f);
                pixel[1] = static_cast<uint8_t>((std::min)(1.0f, g) * 255.0f);
                pixel[2] = static_cast<uint8_t>((std::min)(1.0f, b) * 255.0f);
                pixel[3] = 255;
            }
        }
    }

    uploadBuffer.GetD3D12Resource()->Unmap(0, nullptr);

    // Record copy commands
    commandList->Reset();

    commandList->TransitionBarrier(
        cubemap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    // Copy each face
    for (int face = 0; face < 6; ++face)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer.GetD3D12Resource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Offset = face * faceSize;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = size;
        srcLocation.PlacedFootprint.Footprint.Height = size;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = alignedRowPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = cubemap->GetD3D12Resource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = face;

        commandList->GetD3D12CommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    commandList->TransitionBarrier(
        cubemap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->Close();

    // Execute and wait
    ID3D12CommandList* commandLists[] = { commandList->GetD3D12CommandList() };
    commandQueue->ExecuteCommandLists(commandLists, 1);
    commandQueue->Flush();

    // Create SRV
    if (!cubemap->CreateSRV(srvHeap))
    {
        delete cubemap;
        return nullptr;
    }

    return cubemap;
}
