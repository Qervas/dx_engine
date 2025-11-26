#pragma once

#include "../RHI/CubemapTexture.h"
#include "../RHI/DescriptorHeap.h"
#include <string>
#include <array>

class GraphicsDevice;
class CommandList;
class CommandQueue;

class CubemapLoader
{
public:
    // Load cubemap from 6 individual face images
    // Face order: +X, -X, +Y, -Y, +Z, -Z (right, left, top, bottom, front, back)
    static CubemapTexture* LoadFromFiles(
        GraphicsDevice* device,
        CommandList* commandList,
        CommandQueue* commandQueue,
        DescriptorHeap* srvHeap,
        const std::array<std::string, 6>& facePaths
    );

    // Load cubemap from a folder with standard naming
    // Expects: px.jpg/png, nx.jpg/png, py.jpg/png, ny.jpg/png, pz.jpg/png, nz.jpg/png
    // or: right.jpg/png, left.jpg/png, top.jpg/png, bottom.jpg/png, front.jpg/png, back.jpg/png
    static CubemapTexture* LoadFromFolder(
        GraphicsDevice* device,
        CommandList* commandList,
        CommandQueue* commandQueue,
        DescriptorHeap* srvHeap,
        const std::string& folderPath,
        const std::string& extension = ".jpg"
    );

    // Generate a simple procedural skybox (gradient sky)
    static CubemapTexture* GenerateGradientSky(
        GraphicsDevice* device,
        CommandList* commandList,
        CommandQueue* commandQueue,
        DescriptorHeap* srvHeap,
        uint32_t size = 512
    );

private:
    CubemapLoader() = delete;
};
