#include "Material.h"
#include "../RHI/Device.h"

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
