#pragma once

#include "../RHI/Texture.h"
#include "../RHI/CubemapTexture.h"
#include "../RHI/DescriptorHeap.h"
#include <memory>

class GraphicsDevice;
class CommandList;
class CommandQueue;

// Image-Based Lighting (IBL) resource manager
// Handles generation and storage of:
// - BRDF LUT (2D texture for split-sum approximation)
// - Irradiance map (diffuse IBL - convolved cubemap)
// - Pre-filtered environment map (specular IBL - mip-mapped cubemap)
class IBL
{
public:
    IBL(GraphicsDevice* device);
    ~IBL();

    // Generate all IBL textures from a source environment cubemap
    bool Generate(
        CubemapTexture* environmentMap,
        CommandList* commandList,
        CommandQueue* commandQueue,
        DescriptorHeap* srvHeap
    );

    // Generate only BRDF LUT (can be reused across environments)
    bool GenerateBRDFLUT(
        CommandList* commandList,
        CommandQueue* commandQueue,
        DescriptorHeap* srvHeap,
        uint32_t size = 512
    );

    // Generate irradiance map from environment
    bool GenerateIrradianceMap(
        CubemapTexture* environmentMap,
        CommandList* commandList,
        CommandQueue* commandQueue,
        DescriptorHeap* srvHeap,
        uint32_t size = 32
    );

    // Generate pre-filtered specular map from environment
    bool GeneratePrefilteredMap(
        CubemapTexture* environmentMap,
        CommandList* commandList,
        CommandQueue* commandQueue,
        DescriptorHeap* srvHeap,
        uint32_t size = 128,
        uint32_t mipLevels = 5
    );

    // Accessors
    Texture* GetBRDFLUT() const { return m_brdfLUT.get(); }
    CubemapTexture* GetIrradianceMap() const { return m_irradianceMap.get(); }
    CubemapTexture* GetPrefilteredMap() const { return m_prefilteredMap.get(); }

    const DescriptorHandle& GetBRDFLUTSRV() const { return m_brdfLUT ? m_brdfLUT->GetSRV() : m_nullHandle; }
    const DescriptorHandle& GetIrradianceSRV() const { return m_irradianceMap ? m_irradianceMap->GetSRV() : m_nullHandle; }
    const DescriptorHandle& GetPrefilteredSRV() const { return m_prefilteredMap ? m_prefilteredMap->GetSRV() : m_nullHandle; }

    uint32_t GetPrefilteredMipLevels() const { return m_prefilteredMipLevels; }

private:
    GraphicsDevice* m_device = nullptr;

    std::unique_ptr<Texture> m_brdfLUT;
    std::unique_ptr<CubemapTexture> m_irradianceMap;
    std::unique_ptr<CubemapTexture> m_prefilteredMap;

    uint32_t m_prefilteredMipLevels = 1;
    DescriptorHandle m_nullHandle;

    // CPU-based generation helpers (until we add compute shaders)
    void GenerateBRDFLUTData(std::vector<uint8_t>& data, uint32_t size);
    void GenerateIrradianceData(CubemapTexture* env, std::vector<std::vector<uint8_t>>& faceData, uint32_t size);
    void GeneratePrefilteredData(CubemapTexture* env, std::vector<std::vector<std::vector<uint8_t>>>& mipData, uint32_t size, uint32_t mipLevels);

    // Math helpers
    float RadicalInverse_VdC(uint32_t bits);
    void Hammersley(uint32_t i, uint32_t N, float& xi1, float& xi2);
    void ImportanceSampleGGX(float xi1, float xi2, float roughness, float Nx, float Ny, float Nz, float& Hx, float& Hy, float& Hz);
    float GeometrySchlickGGX(float NdotV, float roughness);
    float GeometrySmith(float NdotV, float NdotL, float roughness);
    void IntegrateBRDF(float NdotV, float roughness, float& scale, float& bias);
};
