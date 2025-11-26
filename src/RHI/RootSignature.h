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

    // Accessors
    ID3D12RootSignature* GetD3D12RootSignature() const { return m_rootSignature.Get(); }
    ID3D12RootSignature** GetAddressOf() { return m_rootSignature.GetAddressOf(); }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
};
