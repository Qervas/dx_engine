#include "SSAO.h"
#include "../RHI/CommandList.h"
#include <random>
#include <algorithm>

SSAO::SSAO(GraphicsDevice* device)
    : m_device(device)
{
}

SSAO::~SSAO()
{
}

bool SSAO::Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    m_width = width;
    m_height = height;

    // Generate sample kernel
    GenerateSampleKernel();

    // Create shaders first
    if (!CreateShaders())
        return false;

    // Create root signatures
    if (!CreateRootSignatures())
        return false;

    // Create pipeline states
    if (!CreatePipelineStates())
        return false;

    // Create textures and resources
    if (!CreateResources(srvHeap, rtvHeap))
        return false;

    // Generate noise texture
    GenerateNoiseTexture(srvHeap);

    return true;
}

void SSAO::Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    if (m_width == width && m_height == height)
        return;

    m_width = width;
    m_height = height;

    // Recreate textures at new size
    CreateResources(srvHeap, rtvHeap);
}

void SSAO::GenerateSampleKernel()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    m_sampleKernel.resize(64);

    for (uint32_t i = 0; i < 64; ++i)
    {
        // Random point in hemisphere (tangent space, z up)
        XMFLOAT3 sample(
            dist(gen) * 2.0f - 1.0f,  // x: -1 to 1
            dist(gen) * 2.0f - 1.0f,  // y: -1 to 1
            dist(gen)                  // z: 0 to 1 (hemisphere)
        );

        // Normalize and scale
        XMVECTOR sampleVec = XMLoadFloat3(&sample);
        sampleVec = XMVector3Normalize(sampleVec);
        sampleVec = XMVectorScale(sampleVec, dist(gen));

        // Scale samples to be more aligned to center (accelerating interpolation)
        float scale = (float)i / 64.0f;
        scale = 0.1f + scale * scale * (1.0f - 0.1f);  // lerp(0.1, 1.0, scale*scale)
        sampleVec = XMVectorScale(sampleVec, scale);

        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&m_sampleKernel[i]), sampleVec);
        m_sampleKernel[i].w = 0.0f;
    }
}

void SSAO::GenerateNoiseTexture(DescriptorHeap* srvHeap)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // 4x4 noise texture (random rotation vectors around z-axis)
    const uint32_t noiseSize = 4;
    std::vector<XMFLOAT4> noiseData(noiseSize * noiseSize);

    for (uint32_t i = 0; i < noiseSize * noiseSize; ++i)
    {
        // Random rotation vector in tangent plane (z = 0)
        noiseData[i] = XMFLOAT4(
            dist(gen) * 2.0f - 1.0f,
            dist(gen) * 2.0f - 1.0f,
            0.0f,
            0.0f
        );
    }

    // Create noise texture
    m_noiseTexture = std::make_unique<Texture>(m_device);
    m_noiseTexture->Create(noiseSize, noiseSize, DXGI_FORMAT_R32G32B32A32_FLOAT, TextureUsage::ShaderResource);

    // Create SRV for noise
    m_noiseSRV = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_noiseTexture->GetD3D12Resource(),
        &srvDesc,
        m_noiseSRV.cpu
    );
}

bool SSAO::CreateResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    const float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // Default to no occlusion

    // Create SSAO texture (single channel would be better, but R8 for simplicity)
    m_ssaoTexture = std::make_unique<Texture>(m_device);
    if (!m_ssaoTexture->Create(m_width, m_height, DXGI_FORMAT_R8_UNORM, TextureUsage::RenderTarget, 1, clearColor))
    {
        return false;
    }

    // Create blurred SSAO texture
    m_ssaoBlurTexture = std::make_unique<Texture>(m_device);
    if (!m_ssaoBlurTexture->Create(m_width, m_height, DXGI_FORMAT_R8_UNORM, TextureUsage::RenderTarget, 1, clearColor))
    {
        return false;
    }

    // Create RTVs
    m_ssaoRTV = rtvHeap->Allocate();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8_UNORM;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    m_device->GetD3D12Device()->CreateRenderTargetView(
        m_ssaoTexture->GetD3D12Resource(),
        &rtvDesc,
        m_ssaoRTV.cpu
    );

    m_ssaoBlurRTV = rtvHeap->Allocate();
    m_device->GetD3D12Device()->CreateRenderTargetView(
        m_ssaoBlurTexture->GetD3D12Resource(),
        &rtvDesc,
        m_ssaoBlurRTV.cpu
    );

    // Create SRVs
    m_ssaoSRV = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_ssaoTexture->GetD3D12Resource(),
        &srvDesc,
        m_ssaoSRV.cpu
    );

    m_ssaoBlurSRV = srvHeap->Allocate();
    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_ssaoBlurTexture->GetD3D12Resource(),
        &srvDesc,
        m_ssaoBlurSRV.cpu
    );

    // Create constant buffers
    m_ssaoConstantBuffer = std::make_unique<Buffer>(m_device);
    if (!m_ssaoConstantBuffer->Create(sizeof(SSAOConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    m_blurConstantBuffer = std::make_unique<Buffer>(m_device);
    if (!m_blurConstantBuffer->Create(sizeof(BlurConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    return true;
}

bool SSAO::CreateShaders()
{
    // SSAO vertex shader (fullscreen triangle)
    m_ssaoVS = std::make_unique<Shader>();
    if (!m_ssaoVS->CompileFromFile(L"shaders/SSAOVS.hlsl", "main", "vs_5_1"))
    {
        return false;
    }

    // SSAO pixel shader
    m_ssaoPS = std::make_unique<Shader>();
    if (!m_ssaoPS->CompileFromFile(L"shaders/SSAOPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    // Blur pixel shader
    m_blurPS = std::make_unique<Shader>();
    if (!m_blurPS->CompileFromFile(L"shaders/SSAOBlurPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    return true;
}

bool SSAO::CreateRootSignatures()
{
    // SSAO root signature
    // b0: SSAO constants
    // t0: Position texture (view-space)
    // t1: Normal texture (view-space)
    // t2: Noise texture
    // s0: Point sampler (for G-Buffer)
    // s1: Wrap sampler (for noise)

    D3D12_ROOT_PARAMETER ssaoParams[4] = {};

    // CBV for SSAO constants
    ssaoParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    ssaoParams[0].Descriptor.ShaderRegister = 0;
    ssaoParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Descriptor tables for textures
    D3D12_DESCRIPTOR_RANGE posRange = {};
    posRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    posRange.NumDescriptors = 1;
    posRange.BaseShaderRegister = 0;
    posRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    ssaoParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ssaoParams[1].DescriptorTable.NumDescriptorRanges = 1;
    ssaoParams[1].DescriptorTable.pDescriptorRanges = &posRange;
    ssaoParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_DESCRIPTOR_RANGE normalRange = {};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    ssaoParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ssaoParams[2].DescriptorTable.NumDescriptorRanges = 1;
    ssaoParams[2].DescriptorTable.pDescriptorRanges = &normalRange;
    ssaoParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_DESCRIPTOR_RANGE noiseRange = {};
    noiseRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    noiseRange.NumDescriptors = 1;
    noiseRange.BaseShaderRegister = 2;
    noiseRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    ssaoParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ssaoParams[3].DescriptorTable.NumDescriptorRanges = 1;
    ssaoParams[3].DescriptorTable.pDescriptorRanges = &noiseRange;
    ssaoParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Samplers
    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

    // Point sampler for G-Buffer
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].ShaderRegister = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Wrap sampler for noise
    samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[1].ShaderRegister = 1;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC ssaoRootSigDesc = {};
    ssaoRootSigDesc.NumParameters = 4;
    ssaoRootSigDesc.pParameters = ssaoParams;
    ssaoRootSigDesc.NumStaticSamplers = 2;
    ssaoRootSigDesc.pStaticSamplers = samplers;
    ssaoRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&ssaoRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char*)error->GetBufferPointer());
        return false;
    }

    m_ssaoRootSignature = std::make_unique<RootSignature>(m_device);
    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(m_ssaoRootSignature->GetAddressOf())
    );

    if (FAILED(hr))
        return false;

    // Blur root signature
    // b0: Blur constants
    // t0: SSAO texture
    // s0: Linear sampler

    D3D12_ROOT_PARAMETER blurParams[2] = {};

    blurParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    blurParams[0].Descriptor.ShaderRegister = 0;
    blurParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_DESCRIPTOR_RANGE ssaoRange = {};
    ssaoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ssaoRange.NumDescriptors = 1;
    ssaoRange.BaseShaderRegister = 0;
    ssaoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    blurParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    blurParams[1].DescriptorTable.NumDescriptorRanges = 1;
    blurParams[1].DescriptorTable.pDescriptorRanges = &ssaoRange;
    blurParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC blurSampler = {};
    blurSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    blurSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    blurSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    blurSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    blurSampler.ShaderRegister = 0;
    blurSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC blurRootSigDesc = {};
    blurRootSigDesc.NumParameters = 2;
    blurRootSigDesc.pParameters = blurParams;
    blurRootSigDesc.NumStaticSamplers = 1;
    blurRootSigDesc.pStaticSamplers = &blurSampler;
    blurRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    hr = D3D12SerializeRootSignature(&blurRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char*)error->GetBufferPointer());
        return false;
    }

    m_blurRootSignature = std::make_unique<RootSignature>(m_device);
    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(m_blurRootSignature->GetAddressOf())
    );

    return SUCCEEDED(hr);
}

bool SSAO::CreatePipelineStates()
{
    // SSAO pipeline state
    D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = {};
    ssaoPsoDesc.pRootSignature = m_ssaoRootSignature->GetD3D12RootSignature();
    ssaoPsoDesc.VS = { m_ssaoVS->GetBufferPointer(), m_ssaoVS->GetBufferSize() };
    ssaoPsoDesc.PS = { m_ssaoPS->GetBufferPointer(), m_ssaoPS->GetBufferSize() };
    ssaoPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    ssaoPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ssaoPsoDesc.RasterizerState.DepthClipEnable = TRUE;
    ssaoPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    ssaoPsoDesc.DepthStencilState.DepthEnable = FALSE;
    ssaoPsoDesc.SampleMask = UINT_MAX;
    ssaoPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ssaoPsoDesc.NumRenderTargets = 1;
    ssaoPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;
    ssaoPsoDesc.SampleDesc.Count = 1;

    m_ssaoPipelineState = std::make_unique<PipelineState>(m_device);
    HRESULT hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
        &ssaoPsoDesc,
        IID_PPV_ARGS(m_ssaoPipelineState->GetAddressOf())
    );

    if (FAILED(hr))
        return false;

    // Blur pipeline state (same structure, different root sig)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC blurPsoDesc = ssaoPsoDesc;
    blurPsoDesc.pRootSignature = m_blurRootSignature->GetD3D12RootSignature();
    blurPsoDesc.PS = { m_blurPS->GetBufferPointer(), m_blurPS->GetBufferSize() };

    m_blurPipelineState = std::make_unique<PipelineState>(m_device);
    hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
        &blurPsoDesc,
        IID_PPV_ARGS(m_blurPipelineState->GetAddressOf())
    );

    return SUCCEEDED(hr);
}

void SSAO::Render(
    ID3D12GraphicsCommandList* commandList,
    DescriptorHeap* srvHeap,
    GBuffer* gBuffer,
    const XMMATRIX& projection)
{
    // Update SSAO constants
    SSAOConstants ssaoConst = {};
    XMStoreFloat4x4(&ssaoConst.projection, XMMatrixTranspose(projection));
    XMStoreFloat4x4(&ssaoConst.invProjection, XMMatrixTranspose(XMMatrixInverse(nullptr, projection)));

    for (uint32_t i = 0; i < 64; ++i)
    {
        ssaoConst.samples[i] = m_sampleKernel[i];
    }

    ssaoConst.noiseScale = XMFLOAT2((float)m_width / 4.0f, (float)m_height / 4.0f);
    ssaoConst.radius = m_radius;
    ssaoConst.bias = m_bias;
    ssaoConst.intensity = m_intensity;

    m_ssaoConstantBuffer->UpdateData(&ssaoConst, sizeof(SSAOConstants));

    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { srvHeap->GetD3D12DescriptorHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // ===== SSAO Pass =====

    // Transition SSAO texture to render target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_ssaoTexture->GetD3D12Resource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // Set render target
    commandList->OMSetRenderTargets(1, &m_ssaoRTV.cpu, FALSE, nullptr);

    // Clear
    const float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    commandList->ClearRenderTargetView(m_ssaoRTV.cpu, clearColor, 0, nullptr);

    // Set viewport and scissor
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, (LONG)m_width, (LONG)m_height };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    // Set pipeline
    commandList->SetPipelineState(m_ssaoPipelineState->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_ssaoRootSignature->GetD3D12RootSignature());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Bind resources
    commandList->SetGraphicsRootConstantBufferView(0, m_ssaoConstantBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(1, gBuffer->GetPositionSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(2, gBuffer->GetNormalSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(3, m_noiseSRV.gpu);

    // Draw fullscreen triangle
    commandList->DrawInstanced(3, 1, 0, 0);

    // ===== Blur Pass =====

    // Transition SSAO to shader resource
    barrier.Transition.pResource = m_ssaoTexture->GetD3D12Resource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);

    // Transition blur texture to render target
    barrier.Transition.pResource = m_ssaoBlurTexture->GetD3D12Resource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // Update blur constants
    BlurConstants blurConst = {};
    blurConst.texelSize = XMFLOAT2(1.0f / m_width, 1.0f / m_height);
    m_blurConstantBuffer->UpdateData(&blurConst, sizeof(BlurConstants));

    // Set render target
    commandList->OMSetRenderTargets(1, &m_ssaoBlurRTV.cpu, FALSE, nullptr);
    commandList->ClearRenderTargetView(m_ssaoBlurRTV.cpu, clearColor, 0, nullptr);

    // Set pipeline
    commandList->SetPipelineState(m_blurPipelineState->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_blurRootSignature->GetD3D12RootSignature());

    // Bind resources
    commandList->SetGraphicsRootConstantBufferView(0, m_blurConstantBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(1, m_ssaoSRV.gpu);

    // Draw fullscreen triangle
    commandList->DrawInstanced(3, 1, 0, 0);

    // Transition blur texture to shader resource
    barrier.Transition.pResource = m_ssaoBlurTexture->GetD3D12Resource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}
