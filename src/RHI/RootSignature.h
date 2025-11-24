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

    // Accessors
    ID3D12RootSignature* GetD3D12RootSignature() const { return m_rootSignature.Get(); }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
};
