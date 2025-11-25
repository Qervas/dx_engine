#pragma once

#include "../RHI/Texture.h"
#include "../RHI/DescriptorHeap.h"
#include <string>

class GraphicsDevice;
class CommandList;

class TextureLoader
{
public:
    // Load texture from file
    static Texture* LoadFromFile(
        GraphicsDevice* device,
        CommandList* commandList,
        DescriptorHeap* srvHeap,
        const std::string& filePath
    );

private:
    TextureLoader() = delete;
};
