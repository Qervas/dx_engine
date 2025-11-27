#pragma once

#include "../Core/Types.h"
#include <d3d12.h>

class GraphicsDevice;

class RootSignature
{
public:
    RootSignature(GraphicsDevice* device);
    ~RootSignature();

    // Create empty root signature (no parameters)
    bool CreateEmpty();

    // Create root signature with CBV and texture (for textured rendering)
    bool CreateWithTextureAndCBV();

    // Create root signature for PBR rendering (MVP + Material + Lighting CBVs + texture)
    bool CreateForPBR();

    // Create root signature for PBR with normal mapping (MVP + Material + Lighting CBVs + albedo + normal textures)
    bool CreateForPBRWithNormalMap();

    // Create root signature for PBR with shadows (MVP + Material + Lighting + Shadow CBVs + albedo + normal + shadow textures)
    bool CreateForPBRWithShadows();

    // Create root signature for PBR with shadows and environment reflections
    bool CreateForPBRWithEnvironment();

    // Create root signature for PBR with full IBL (irradiance, prefiltered, BRDF LUT)
    bool CreateForPBRWithIBL();

    // Create root signature for PBR with IBL and SSAO
    bool CreateForPBRWithSSAO();

    // Create root signature for skybox rendering (CBV + cubemap texture)
    bool CreateForSkybox();

    // Accessors
    ID3D12RootSignature* GetD3D12RootSignature() const { return m_rootSignature.Get(); }
    ID3D12RootSignature** GetAddressOf() { return m_rootSignature.GetAddressOf(); }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
};
