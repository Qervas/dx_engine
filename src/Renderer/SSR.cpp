#include "SSR.h"
#include "../RHI/CommandList.h"
#include <d3dcompiler.h>

SSR::SSR(GraphicsDevice* device)
    : m_device(device)
{
}

SSR::~SSR()
{
}

bool SSR::Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    m_width = width;
    m_height = height;

    if (!CreateResources(srvHeap, rtvHeap))
        return false;

    if (!CreateShaders())
        return false;

    if (!CreateRootSignature())
        return false;

    if (!CreatePipelineState())
        return false;

    return true;
}

void SSR::Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    if (m_width == width && m_height == height)
        return;

    m_width = width;
    m_height = height;

    // Recreate textures at new size
    CreateResources(srvHeap, rtvHeap);
}

bool SSR::CreateResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    // Create SSR output texture (RGBA for reflection color + alpha for reflection intensity)
    m_ssrTexture = std::make_unique<Texture>(m_device);
    if (!m_ssrTexture->Create(m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureUsage::RenderTarget))
    {
        return false;
    }

    // Create RTV for SSR texture
    m_ssrRTV = rtvHeap->Allocate();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    m_device->GetD3D12Device()->CreateRenderTargetView(
        m_ssrTexture->GetD3D12Resource(),
        &rtvDesc,
        m_ssrRTV.cpu
    );

    // Create SRV for SSR texture
    m_ssrSRV = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_ssrTexture->GetD3D12Resource(),
        &srvDesc,
        m_ssrSRV.cpu
    );

    m_ssrTexture->SetSRV(m_ssrSRV);

    // Create constant buffer
    m_constantBuffer = std::make_unique<Buffer>(m_device);
    if (!m_constantBuffer->Create(sizeof(SSRConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    return true;
}

bool SSR::CreateShaders()
{
    // Compile vertex shader (fullscreen triangle)
    m_ssrVS = std::make_unique<Shader>();
    if (!m_ssrVS->CompileFromFile(L"shaders/SSRVS.hlsl", "main", "vs_5_1"))
    {
        return false;
    }

    // Compile pixel shader
    m_ssrPS = std::make_unique<Shader>();
    if (!m_ssrPS->CompileFromFile(L"shaders/SSRPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    return true;
}

bool SSR::CreateRootSignature()
{
    // Root signature for SSR
    // b0: SSR constants
    // t0: Position texture (from G-Buffer)
    // t1: Normal texture (from G-Buffer)
    // t2: Scene color texture
    // s0: Point sampler
    // s1: Linear sampler

    D3D12_ROOT_PARAMETER rootParams[4] = {};

    // Root parameter 0: CBV for SSR constants
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 1: Position texture (t0)
    D3D12_DESCRIPTOR_RANGE posRange = {};
    posRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    posRange.NumDescriptors = 1;
    posRange.BaseShaderRegister = 0;
    posRange.RegisterSpace = 0;
    posRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &posRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 2: Normal texture (t1)
    D3D12_DESCRIPTOR_RANGE normalRange = {};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &normalRange;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 3: Scene color texture (t2)
    D3D12_DESCRIPTOR_RANGE colorRange = {};
    colorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    colorRange.NumDescriptors = 1;
    colorRange.BaseShaderRegister = 2;
    colorRange.RegisterSpace = 0;
    colorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[3].DescriptorTable.pDescriptorRanges = &colorRange;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static samplers
    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

    // Point sampler for G-Buffer sampling
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 1;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Linear sampler for color sampling
    samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].MipLODBias = 0.0f;
    samplers[1].MaxAnisotropy = 1;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplers[1].MinLOD = 0.0f;
    samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister = 1;
    samplers[1].RegisterSpace = 0;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 4;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 2;
    rootSigDesc.pStaticSamplers = samplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char*)error->GetBufferPointer());
        return false;
    }

    m_rootSignature = std::make_unique<RootSignature>(m_device);
    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(m_rootSignature->GetAddressOf())
    );

    return SUCCEEDED(hr);
}

bool SSR::CreatePipelineState()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature->GetD3D12RootSignature();
    psoDesc.VS = { m_ssrVS->GetBufferPointer(), m_ssrVS->GetBufferSize() };
    psoDesc.PS = { m_ssrPS->GetBufferPointer(), m_ssrPS->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    m_pipelineState = std::make_unique<PipelineState>(m_device);
    HRESULT hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(m_pipelineState->GetAddressOf())
    );

    return SUCCEEDED(hr);
}

void SSR::Render(
    ID3D12GraphicsCommandList* commandList,
    DescriptorHeap* srvHeap,
    GBuffer* gBuffer,
    Texture* sceneColor,
    const XMMATRIX& view,
    const XMMATRIX& projection
)
{
    if (!gBuffer || !sceneColor)
        return;

    // Transition SSR texture to render target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_ssrTexture->GetD3D12Resource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // Set render target
    commandList->OMSetRenderTargets(1, &m_ssrRTV.cpu, FALSE, nullptr);

    // Clear SSR texture
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(m_ssrRTV.cpu, clearColor, 0, nullptr);

    // Set viewport and scissor
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, (LONG)m_width, (LONG)m_height };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    // Set pipeline state
    commandList->SetPipelineState(m_pipelineState->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { srvHeap->GetD3D12DescriptorHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Update constants
    SSRConstants constants;
    XMStoreFloat4x4(&constants.projection, XMMatrixTranspose(projection));
    XMStoreFloat4x4(&constants.invProjection, XMMatrixTranspose(XMMatrixInverse(nullptr, projection)));
    XMStoreFloat4x4(&constants.view, XMMatrixTranspose(view));
    constants.screenSize = XMFLOAT2((float)m_width, (float)m_height);
    constants.maxDistance = m_maxDistance;
    constants.thickness = m_thickness;
    constants.stepSize = m_stepSize;
    constants.maxSteps = m_maxSteps;
    constants.fadeStart = m_fadeStart;
    constants.fadeEnd = m_fadeEnd;

    m_constantBuffer->UpdateData(&constants, sizeof(SSRConstants));

    // Bind resources
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(1, gBuffer->GetPositionSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(2, gBuffer->GetNormalSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(3, sceneColor->GetSRV().gpu);

    // Draw fullscreen triangle
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    // Transition SSR texture back to shader resource
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}
