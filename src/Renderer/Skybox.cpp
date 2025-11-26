#include "Skybox.h"
#include "Camera.h"
#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/DescriptorHeap.h"

using namespace DirectX;

Skybox::Skybox(GraphicsDevice* device)
    : m_device(device)
{
}

Skybox::~Skybox()
{
}

bool Skybox::Initialize(CubemapTexture* cubemap, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
    m_cubemap = cubemap;

    // Compile shaders
    m_vertexShader = std::make_unique<Shader>();
    if (!m_vertexShader->CompileFromFile(L"shaders/SkyboxVS.hlsl", "main", "vs_5_1"))
    {
        return false;
    }

    m_pixelShader = std::make_unique<Shader>();
    if (!m_pixelShader->CompileFromFile(L"shaders/SkyboxPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    // Create root signature
    m_rootSignature = std::make_unique<RootSignature>(m_device);
    if (!m_rootSignature->CreateForSkybox())
    {
        return false;
    }

    // Create pipeline state
    m_pipelineState = std::make_unique<PipelineState>(m_device);
    if (!m_pipelineState->CreateForSkybox(
        m_rootSignature.get(),
        m_vertexShader.get(),
        m_pixelShader.get(),
        rtvFormat,
        dsvFormat))
    {
        return false;
    }

    // Create constant buffer
    m_constantBuffer = std::make_unique<Buffer>(m_device);
    if (!m_constantBuffer->Create(sizeof(SkyboxConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    return true;
}

void Skybox::Render(CommandList* commandList, DescriptorHeap* srvHeap, Camera* camera)
{
    if (!m_cubemap || !camera)
        return;

    // Update constant buffer
    XMMATRIX view = camera->GetViewMatrix();
    XMMATRIX proj = camera->GetProjectionMatrix();
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    SkyboxConstants constants;
    constants.inverseViewProjection = XMMatrixTranspose(invViewProj);  // Transpose for HLSL
    constants.cameraPosition = camera->GetPosition();
    constants.exposure = m_exposure;

    m_constantBuffer->UpdateData(&constants, sizeof(constants));

    // Set pipeline state
    commandList->GetD3D12CommandList()->SetPipelineState(m_pipelineState->GetD3D12PipelineState());
    commandList->GetD3D12CommandList()->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { srvHeap->GetD3D12DescriptorHeap() };
    commandList->GetD3D12CommandList()->SetDescriptorHeaps(1, heaps);

    // Set root parameters
    commandList->GetD3D12CommandList()->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    commandList->GetD3D12CommandList()->SetGraphicsRootDescriptorTable(1, m_cubemap->GetSRV().gpu);

    // Set primitive topology
    commandList->GetD3D12CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw fullscreen triangle (3 vertices, no vertex buffer needed)
    commandList->GetD3D12CommandList()->DrawInstanced(3, 1, 0, 0);
}
