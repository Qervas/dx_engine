#pragma once

#include "../Core/Types.h"
#include <d3d12.h>

class GraphicsDevice;
class Shader;
class RootSignature;

class PipelineState
{
public:
    PipelineState(GraphicsDevice* device);
    ~PipelineState();

    // Create graphics pipeline state
    bool CreateGraphics(
        RootSignature* rootSignature,
        Shader* vertexShader,
        Shader* pixelShader,
        const D3D12_INPUT_LAYOUT_DESC& inputLayout,
        DXGI_FORMAT rtvFormat,
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN
    );

    // Accessors
    ID3D12PipelineState* GetD3D12PipelineState() const { return m_pipelineState.Get(); }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12PipelineState> m_pipelineState;
};
