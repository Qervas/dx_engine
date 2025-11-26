#include "Material.h"
#include "../RHI/Device.h"
#include <fstream>
#include <sstream>
#include <algorithm>

Material::Material(GraphicsDevice* device)
    : m_device(device)
{
    // Create constant buffer for material properties
    m_constantBuffer = std::make_unique<Buffer>(device);
    m_constantBuffer->Create(sizeof(MaterialProperties), 0, BufferUsage::Constant);
}

Material::~Material()
{
}

void Material::SetAlbedoTexture(Texture* texture, DescriptorHandle srv)
{
    m_albedoTexture = texture;
    m_albedoSRV = srv;
}

void Material::SetNormalTexture(Texture* texture, DescriptorHandle srv)
{
    m_normalTexture = texture;
    m_normalSRV = srv;
    m_properties.useNormalMap = (texture != nullptr) ? 1.0f : 0.0f;
    m_dirty = true;
}

void Material::SetMetallicRoughnessTexture(Texture* texture, DescriptorHandle srv)
{
    m_metallicRoughnessTexture = texture;
    m_metallicRoughnessSRV = srv;
}

void Material::UpdateConstants()
{
    if (m_dirty)
    {
        m_constantBuffer->UpdateData(&m_properties, sizeof(MaterialProperties));
        m_dirty = false;
    }
}

bool Material::LoadFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        // Find key=value separator
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
            continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        if (key == "name")
        {
            m_name = value;
        }
        else if (key == "albedo")
        {
            // Parse r,g,b
            std::stringstream ss(value);
            char comma;
            ss >> m_properties.albedo.x >> comma >> m_properties.albedo.y >> comma >> m_properties.albedo.z;
            m_dirty = true;
        }
        else if (key == "metallic")
        {
            m_properties.metallic = std::stof(value);
            m_dirty = true;
        }
        else if (key == "roughness")
        {
            m_properties.roughness = std::stof(value);
            m_dirty = true;
        }
        else if (key == "ao")
        {
            m_properties.ao = std::stof(value);
            m_dirty = true;
        }
        else if (key == "albedo_texture")
        {
            m_albedoTexturePath = value;
        }
        else if (key == "normal_texture")
        {
            m_normalTexturePath = value;
            m_properties.useNormalMap = 1.0f;
            m_dirty = true;
        }
        else if (key == "metallic_roughness_texture")
        {
            m_metallicRoughnessTexturePath = value;
        }
    }

    return true;
}

bool Material::SaveToFile(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open())
        return false;

    file << "# Material definition file\n";
    file << "name=" << m_name << "\n";
    file << "\n# PBR Properties\n";
    file << "albedo=" << m_properties.albedo.x << "," << m_properties.albedo.y << "," << m_properties.albedo.z << "\n";
    file << "metallic=" << m_properties.metallic << "\n";
    file << "roughness=" << m_properties.roughness << "\n";
    file << "ao=" << m_properties.ao << "\n";

    if (!m_albedoTexturePath.empty())
    {
        file << "\n# Textures\n";
        file << "albedo_texture=" << m_albedoTexturePath << "\n";
    }
    if (!m_normalTexturePath.empty())
    {
        file << "normal_texture=" << m_normalTexturePath << "\n";
    }
    if (!m_metallicRoughnessTexturePath.empty())
    {
        file << "metallic_roughness_texture=" << m_metallicRoughnessTexturePath << "\n";
    }

    return true;
}
