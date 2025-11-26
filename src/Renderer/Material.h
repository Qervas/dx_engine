#pragma once

#include "../RHI/Texture.h"
#include "../RHI/Buffer.h"
#include "../RHI/DescriptorHeap.h"
#include <DirectXMath.h>
#include <memory>
#include <string>

using namespace DirectX;

// Forward declarations
class GraphicsDevice;

// PBR material properties
struct MaterialProperties
{
    XMFLOAT3 albedo = XMFLOAT3(1.0f, 1.0f, 1.0f);
    float metallic = 0.0f;

    float roughness = 0.5f;
    float ao = 1.0f;  // Ambient occlusion
    float useNormalMap = 0.0f;  // 1.0 if normal map should be used
    float _padding;
};

class Material
{
public:
    Material(GraphicsDevice* device);
    ~Material();

    // Set material properties
    void SetAlbedo(const XMFLOAT3& albedo) { m_properties.albedo = albedo; m_dirty = true; }
    void SetMetallic(float metallic) { m_properties.metallic = metallic; m_dirty = true; }
    void SetRoughness(float roughness) { m_properties.roughness = roughness; m_dirty = true; }
    void SetAO(float ao) { m_properties.ao = ao; m_dirty = true; }

    // Set textures (optional - can use constant values instead)
    void SetAlbedoTexture(Texture* texture, DescriptorHandle srv);
    void SetNormalTexture(Texture* texture, DescriptorHandle srv);
    void SetMetallicRoughnessTexture(Texture* texture, DescriptorHandle srv);

    // Update constant buffer if properties changed
    void UpdateConstants();

    // Material name
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    // Load/Save material definition file (.mat format)
    // Format: key=value pairs, one per line
    // albedo=1.0,1.0,1.0
    // metallic=0.0
    // roughness=0.5
    // albedo_texture=path/to/texture.png
    // normal_texture=path/to/normal.png
    bool LoadFromFile(const std::string& filepath);
    bool SaveToFile(const std::string& filepath) const;

    // Texture paths (for serialization)
    void SetAlbedoTexturePath(const std::string& path) { m_albedoTexturePath = path; }
    void SetNormalTexturePath(const std::string& path) { m_normalTexturePath = path; }
    const std::string& GetAlbedoTexturePath() const { return m_albedoTexturePath; }
    const std::string& GetNormalTexturePath() const { return m_normalTexturePath; }

    // Accessors
    const MaterialProperties& GetProperties() const { return m_properties; }
    Buffer* GetConstantBuffer() const { return m_constantBuffer.get(); }

    Texture* GetAlbedoTexture() const { return m_albedoTexture; }
    Texture* GetNormalTexture() const { return m_normalTexture; }
    Texture* GetMetallicRoughnessTexture() const { return m_metallicRoughnessTexture; }

    DescriptorHandle GetAlbedoSRV() const { return m_albedoSRV; }
    DescriptorHandle GetNormalSRV() const { return m_normalSRV; }
    DescriptorHandle GetMetallicRoughnessSRV() const { return m_metallicRoughnessSRV; }

    bool HasAlbedoTexture() const { return m_albedoTexture != nullptr; }
    bool HasNormalTexture() const { return m_normalTexture != nullptr; }
    bool HasMetallicRoughnessTexture() const { return m_metallicRoughnessTexture != nullptr; }

private:
    GraphicsDevice* m_device;
    MaterialProperties m_properties;
    std::unique_ptr<Buffer> m_constantBuffer;
    bool m_dirty = true;
    std::string m_name;

    // Texture resources (optional)
    Texture* m_albedoTexture = nullptr;
    Texture* m_normalTexture = nullptr;
    Texture* m_metallicRoughnessTexture = nullptr;

    // Texture paths (for serialization)
    std::string m_albedoTexturePath;
    std::string m_normalTexturePath;
    std::string m_metallicRoughnessTexturePath;

    DescriptorHandle m_albedoSRV = {};
    DescriptorHandle m_normalSRV = {};
    DescriptorHandle m_metallicRoughnessSRV = {};
};
