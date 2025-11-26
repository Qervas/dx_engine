#pragma once

#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/CommandQueue.h"
#include "../RHI/DescriptorHeap.h"
#include "../RHI/Texture.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../Renderer/ModelLoader.h"
#include <string>
#include <memory>
#include <unordered_map>

// Singleton resource manager for caching loaded assets
class ResourceManager
{
public:
    static ResourceManager& Get();

    // Initialize with graphics context
    void Initialize(GraphicsDevice* device, CommandQueue* commandQueue, DescriptorHeap* srvHeap);
    void Shutdown();

    // Texture loading (cached by path)
    Texture* LoadTexture(const std::string& path);
    Texture* GetTexture(const std::string& path);

    // Model loading (cached by path)
    ModelData* LoadModel(const std::string& path);
    ModelData* GetModel(const std::string& path);

    // Material creation (cached by name)
    Material* CreateMaterial(const std::string& name);
    Material* GetMaterial(const std::string& name);

    // Default resources
    Texture* GetDefaultWhiteTexture();
    Texture* GetDefaultNormalTexture();
    Texture* GetDefaultBlackTexture();

    // Execute pending GPU uploads
    void FlushUploads();

    // Statistics
    size_t GetTextureCount() const { return m_textures.size(); }
    size_t GetModelCount() const { return m_models.size(); }
    size_t GetMaterialCount() const { return m_materials.size(); }

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    void CreateDefaultTextures();

    GraphicsDevice* m_device = nullptr;
    CommandQueue* m_commandQueue = nullptr;
    DescriptorHeap* m_srvHeap = nullptr;
    std::unique_ptr<CommandList> m_uploadCommandList;
    std::unique_ptr<ModelLoader> m_modelLoader;

    // Cached resources
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
    std::unordered_map<std::string, std::unique_ptr<ModelData>> m_models;
    std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;

    // Default textures
    std::unique_ptr<Texture> m_defaultWhite;
    std::unique_ptr<Texture> m_defaultNormal;
    std::unique_ptr<Texture> m_defaultBlack;

    bool m_initialized = false;
    bool m_hasPendingUploads = false;
};
