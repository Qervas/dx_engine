#include "ResourceManager.h"
#include "../RHI/Buffer.h"
#include <stb_image.h>
#include <algorithm>

ResourceManager& ResourceManager::Get()
{
    static ResourceManager instance;
    return instance;
}

void ResourceManager::Initialize(GraphicsDevice* device, CommandQueue* commandQueue, DescriptorHeap* srvHeap)
{
    if (m_initialized)
        return;

    m_device = device;
    m_commandQueue = commandQueue;
    m_srvHeap = srvHeap;

    // Create upload command list
    m_uploadCommandList = std::make_unique<CommandList>(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_uploadCommandList->Initialize();

    // Create model loader
    m_modelLoader = std::make_unique<ModelLoader>(device);

    // Create default textures
    CreateDefaultTextures();

    m_initialized = true;
}

void ResourceManager::Shutdown()
{
    // Wait for GPU
    if (m_commandQueue)
        m_commandQueue->Flush();

    // Clear all resources
    m_textures.clear();
    m_models.clear();
    m_materials.clear();

    m_defaultWhite.reset();
    m_defaultNormal.reset();
    m_defaultBlack.reset();

    m_uploadCommandList.reset();
    m_modelLoader.reset();

    m_device = nullptr;
    m_commandQueue = nullptr;
    m_srvHeap = nullptr;
    m_initialized = false;
}

void ResourceManager::CreateDefaultTextures()
{
    // White texture (1x1)
    {
        m_defaultWhite = std::make_unique<Texture>(m_device);
        m_defaultWhite->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource);

        uint32_t whitePixel = 0xFFFFFFFF;
        Buffer uploadBuffer(m_device);
        uploadBuffer.Create(4, 0, BufferUsage::Upload, &whitePixel);

        m_uploadCommandList->Reset();
        m_uploadCommandList->TransitionBarrier(m_defaultWhite->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadBuffer.GetD3D12Resource();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.Width = 1;
        src.PlacedFootprint.Footprint.Height = 1;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.RowPitch = 256; // Minimum row pitch

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = m_defaultWhite->GetD3D12Resource();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        m_uploadCommandList->GetD3D12CommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        m_uploadCommandList->TransitionBarrier(m_defaultWhite->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_uploadCommandList->Close();

        ID3D12CommandList* cmdLists[] = { m_uploadCommandList->GetD3D12CommandList() };
        m_commandQueue->ExecuteCommandLists(cmdLists, 1);
        m_commandQueue->Flush();

        // Create SRV
        DescriptorHandle handle = m_srvHeap->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        m_device->GetD3D12Device()->CreateShaderResourceView(
            m_defaultWhite->GetD3D12Resource(), &srvDesc, handle.cpu);
        m_defaultWhite->SetSRV(handle);
    }

    // Default normal map (flat - pointing up in tangent space)
    {
        m_defaultNormal = std::make_unique<Texture>(m_device);
        m_defaultNormal->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource);

        // Normal (0.5, 0.5, 1.0) = flat surface pointing up
        uint32_t normalPixel = 0xFF8080FF;  // RGBA: 128, 128, 255, 255
        Buffer uploadBuffer(m_device);
        uploadBuffer.Create(4, 0, BufferUsage::Upload, &normalPixel);

        m_uploadCommandList->Reset();
        m_uploadCommandList->TransitionBarrier(m_defaultNormal->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadBuffer.GetD3D12Resource();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.Width = 1;
        src.PlacedFootprint.Footprint.Height = 1;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.RowPitch = 256;

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = m_defaultNormal->GetD3D12Resource();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        m_uploadCommandList->GetD3D12CommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        m_uploadCommandList->TransitionBarrier(m_defaultNormal->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_uploadCommandList->Close();

        ID3D12CommandList* cmdLists[] = { m_uploadCommandList->GetD3D12CommandList() };
        m_commandQueue->ExecuteCommandLists(cmdLists, 1);
        m_commandQueue->Flush();

        DescriptorHandle handle = m_srvHeap->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        m_device->GetD3D12Device()->CreateShaderResourceView(
            m_defaultNormal->GetD3D12Resource(), &srvDesc, handle.cpu);
        m_defaultNormal->SetSRV(handle);
    }

    // Black texture (1x1)
    {
        m_defaultBlack = std::make_unique<Texture>(m_device);
        m_defaultBlack->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource);

        uint32_t blackPixel = 0xFF000000;  // Black with alpha
        Buffer uploadBuffer(m_device);
        uploadBuffer.Create(4, 0, BufferUsage::Upload, &blackPixel);

        m_uploadCommandList->Reset();
        m_uploadCommandList->TransitionBarrier(m_defaultBlack->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadBuffer.GetD3D12Resource();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.Width = 1;
        src.PlacedFootprint.Footprint.Height = 1;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.RowPitch = 256;

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = m_defaultBlack->GetD3D12Resource();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        m_uploadCommandList->GetD3D12CommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        m_uploadCommandList->TransitionBarrier(m_defaultBlack->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_uploadCommandList->Close();

        ID3D12CommandList* cmdLists[] = { m_uploadCommandList->GetD3D12CommandList() };
        m_commandQueue->ExecuteCommandLists(cmdLists, 1);
        m_commandQueue->Flush();

        DescriptorHandle handle = m_srvHeap->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        m_device->GetD3D12Device()->CreateShaderResourceView(
            m_defaultBlack->GetD3D12Resource(), &srvDesc, handle.cpu);
        m_defaultBlack->SetSRV(handle);
    }
}

Texture* ResourceManager::LoadTexture(const std::string& path)
{
    // Check cache first
    auto it = m_textures.find(path);
    if (it != m_textures.end())
        return it->second.get();

    // Load image
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data)
        return nullptr;

    // Create texture
    auto texture = std::make_unique<Texture>(m_device);
    if (!texture->Create(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource))
    {
        stbi_image_free(data);
        return nullptr;
    }

    // Upload
    uint32_t dataSize = width * height * 4;
    Buffer uploadBuffer(m_device);
    if (!uploadBuffer.Create(dataSize, 0, BufferUsage::Upload, data))
    {
        stbi_image_free(data);
        return nullptr;
    }

    m_uploadCommandList->Reset();
    m_uploadCommandList->TransitionBarrier(texture->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = uploadBuffer.GetD3D12Resource();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = width;
    src.PlacedFootprint.Footprint.Height = height;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = width * 4;

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = texture->GetD3D12Resource();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    m_uploadCommandList->GetD3D12CommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    m_uploadCommandList->TransitionBarrier(texture->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_uploadCommandList->Close();

    ID3D12CommandList* cmdLists[] = { m_uploadCommandList->GetD3D12CommandList() };
    m_commandQueue->ExecuteCommandLists(cmdLists, 1);
    m_commandQueue->Flush();

    stbi_image_free(data);

    // Create SRV
    DescriptorHandle handle = m_srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    m_device->GetD3D12Device()->CreateShaderResourceView(
        texture->GetD3D12Resource(), &srvDesc, handle.cpu);
    texture->SetSRV(handle);

    Texture* result = texture.get();
    m_textures[path] = std::move(texture);
    return result;
}

Texture* ResourceManager::GetTexture(const std::string& path)
{
    auto it = m_textures.find(path);
    return (it != m_textures.end()) ? it->second.get() : nullptr;
}

ModelData* ResourceManager::LoadModel(const std::string& path)
{
    // Check cache
    auto it = m_models.find(path);
    if (it != m_models.end())
        return it->second.get();

    // Load model
    auto model = m_modelLoader->LoadFromFile(path);
    if (!model)
        return nullptr;

    ModelData* result = model.get();
    m_models[path] = std::move(model);
    return result;
}

ModelData* ResourceManager::GetModel(const std::string& path)
{
    auto it = m_models.find(path);
    return (it != m_models.end()) ? it->second.get() : nullptr;
}

Material* ResourceManager::CreateMaterial(const std::string& name)
{
    // Check if exists
    auto it = m_materials.find(name);
    if (it != m_materials.end())
        return it->second.get();

    // Create new material
    auto material = std::make_unique<Material>(m_device);
    Material* result = material.get();
    m_materials[name] = std::move(material);
    return result;
}

Material* ResourceManager::GetMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    return (it != m_materials.end()) ? it->second.get() : nullptr;
}

Texture* ResourceManager::GetDefaultWhiteTexture()
{
    return m_defaultWhite.get();
}

Texture* ResourceManager::GetDefaultNormalTexture()
{
    return m_defaultNormal.get();
}

Texture* ResourceManager::GetDefaultBlackTexture()
{
    return m_defaultBlack.get();
}

void ResourceManager::FlushUploads()
{
    if (m_hasPendingUploads && m_commandQueue)
    {
        m_commandQueue->Flush();
        m_hasPendingUploads = false;
    }
}
