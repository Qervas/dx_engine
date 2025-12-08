#include "PostProcess.h"
#include <d3dcompiler.h>

PostProcess::PostProcess(GraphicsDevice* device)
    : m_device(device)
{
}

PostProcess::~PostProcess()
{
}

bool PostProcess::Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    m_width = width;
    m_height = height;

    if (!CreateHDRResources(srvHeap, rtvHeap))
        return false;

    if (!CreateBloomResources(srvHeap, rtvHeap))
        return false;

    if (!CreateShaders())
        return false;

    if (!CreateRootSignatures())
        return false;

    if (!CreatePipelineStates())
        return false;

    return true;
}

void PostProcess::Resize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    if (m_width == width && m_height == height)
        return;

    m_width = width;
    m_height = height;

    CreateHDRResources(srvHeap, rtvHeap);
    CreateBloomResources(srvHeap, rtvHeap);
}

bool PostProcess::CreateHDRResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    // Create HDR render target (R16G16B16A16_FLOAT for HDR values)
    m_hdrTexture = std::make_unique<Texture>(m_device);
    if (!m_hdrTexture->Create(m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureUsage::RenderTarget))
    {
        return false;
    }

    // Create RTV
    m_hdrRTV = rtvHeap->Allocate();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    m_device->GetD3D12Device()->CreateRenderTargetView(
        m_hdrTexture->GetD3D12Resource(),
        &rtvDesc,
        m_hdrRTV.cpu
    );

    // Create SRV
    m_hdrSRV = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_hdrTexture->GetD3D12Resource(),
        &srvDesc,
        m_hdrSRV.cpu
    );

    m_hdrTexture->SetSRV(m_hdrSRV);

    // Create constant buffer
    m_postProcessCB = std::make_unique<Buffer>(m_device);
    if (!m_postProcessCB->Create(sizeof(PostProcessConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    return true;
}

bool PostProcess::CreateBloomResources(DescriptorHeap* srvHeap, DescriptorHeap* rtvHeap)
{
    // Create bloom mip chain textures (each half the size of previous)
    uint32_t mipWidth = m_width / 2;
    uint32_t mipHeight = m_height / 2;

    for (uint32_t i = 0; i < BLOOM_MIP_COUNT; ++i)
    {
        m_bloomTextures[i] = std::make_unique<Texture>(m_device);
        if (!m_bloomTextures[i]->Create(mipWidth, mipHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureUsage::RenderTarget))
        {
            return false;
        }

        // Create RTV
        m_bloomRTVs[i] = rtvHeap->Allocate();
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;

        m_device->GetD3D12Device()->CreateRenderTargetView(
            m_bloomTextures[i]->GetD3D12Resource(),
            &rtvDesc,
            m_bloomRTVs[i].cpu
        );

        // Create SRV
        m_bloomSRVs[i] = srvHeap->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        m_device->GetD3D12Device()->CreateShaderResourceView(
            m_bloomTextures[i]->GetD3D12Resource(),
            &srvDesc,
            m_bloomSRVs[i].cpu
        );

        m_bloomTextures[i]->SetSRV(m_bloomSRVs[i]);

        // Halve dimensions for next mip
        mipWidth = max(1u, mipWidth / 2);
        mipHeight = max(1u, mipHeight / 2);
    }

    // Create bloom blur intermediate texture (same size as first bloom mip)
    m_bloomBlurTexture = std::make_unique<Texture>(m_device);
    if (!m_bloomBlurTexture->Create(m_width / 2, m_height / 2, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureUsage::RenderTarget))
    {
        return false;
    }

    m_bloomBlurRTV = rtvHeap->Allocate();
    D3D12_RENDER_TARGET_VIEW_DESC blurRtvDesc = {};
    blurRtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    blurRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    blurRtvDesc.Texture2D.MipSlice = 0;

    m_device->GetD3D12Device()->CreateRenderTargetView(
        m_bloomBlurTexture->GetD3D12Resource(),
        &blurRtvDesc,
        m_bloomBlurRTV.cpu
    );

    m_bloomBlurSRV = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC blurSrvDesc = {};
    blurSrvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    blurSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    blurSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    blurSrvDesc.Texture2D.MipLevels = 1;
    blurSrvDesc.Texture2D.MostDetailedMip = 0;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_bloomBlurTexture->GetD3D12Resource(),
        &blurSrvDesc,
        m_bloomBlurSRV.cpu
    );

    m_bloomBlurTexture->SetSRV(m_bloomBlurSRV);

    // Create bloom blur constant buffer
    m_bloomBlurCB = std::make_unique<Buffer>(m_device);
    if (!m_bloomBlurCB->Create(sizeof(BloomBlurConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    return true;
}

bool PostProcess::CreateShaders()
{
    // Fullscreen vertex shader
    m_fullscreenVS = std::make_unique<Shader>();
    if (!m_fullscreenVS->CompileFromFile(L"shaders/FullscreenVS.hlsl", "main", "vs_5_1"))
    {
        return false;
    }

    // Tone mapping pixel shader
    m_toneMapPS = std::make_unique<Shader>();
    if (!m_toneMapPS->CompileFromFile(L"shaders/ToneMapPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    // Bloom extract pixel shader (extracts bright pixels)
    m_bloomExtractPS = std::make_unique<Shader>();
    if (!m_bloomExtractPS->CompileFromFile(L"shaders/BloomExtractPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    // Bloom blur pixel shader (Gaussian blur)
    m_bloomBlurPS = std::make_unique<Shader>();
    if (!m_bloomBlurPS->CompileFromFile(L"shaders/BloomBlurPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    return true;
}

bool PostProcess::CreateRootSignatures()
{
    // Tone map root signature
    // b0: Post-process constants
    // t0: HDR scene texture
    // t1: Bloom texture
    // s0: Linear sampler
    {
        D3D12_ROOT_PARAMETER rootParams[3] = {};

        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].Descriptor.RegisterSpace = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_DESCRIPTOR_RANGE hdrRange = {};
        hdrRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        hdrRange.NumDescriptors = 1;
        hdrRange.BaseShaderRegister = 0;
        hdrRange.RegisterSpace = 0;
        hdrRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges = &hdrRange;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_DESCRIPTOR_RANGE bloomRange = {};
        bloomRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        bloomRange.NumDescriptors = 1;
        bloomRange.BaseShaderRegister = 1;
        bloomRange.RegisterSpace = 0;
        bloomRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[2].DescriptorTable.pDescriptorRanges = &bloomRange;
        rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MipLODBias = 0.0f;
        sampler.MaxAnisotropy = 1;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 3;
        rootSigDesc.pParameters = rootParams;
        rootSigDesc.NumStaticSamplers = 1;
        rootSigDesc.pStaticSamplers = &sampler;
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

        m_toneMapRootSig = std::make_unique<RootSignature>(m_device);
        hr = m_device->GetD3D12Device()->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(m_toneMapRootSig->GetAddressOf())
        );

        if (FAILED(hr))
            return false;
    }

    // Bloom root signature
    // b0: Bloom blur constants
    // t0: Input texture
    // s0: Linear sampler
    {
        D3D12_ROOT_PARAMETER rootParams[2] = {};

        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].Descriptor.RegisterSpace = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_DESCRIPTOR_RANGE inputRange = {};
        inputRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        inputRange.NumDescriptors = 1;
        inputRange.BaseShaderRegister = 0;
        inputRange.RegisterSpace = 0;
        inputRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges = &inputRange;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MipLODBias = 0.0f;
        sampler.MaxAnisotropy = 1;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 2;
        rootSigDesc.pParameters = rootParams;
        rootSigDesc.NumStaticSamplers = 1;
        rootSigDesc.pStaticSamplers = &sampler;
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

        m_bloomRootSig = std::make_unique<RootSignature>(m_device);
        hr = m_device->GetD3D12Device()->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(m_bloomRootSig->GetAddressOf())
        );

        if (FAILED(hr))
            return false;
    }

    return true;
}

bool PostProcess::CreatePipelineStates()
{
    // Tone map pipeline (outputs to LDR back buffer)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_toneMapRootSig->GetD3D12RootSignature();
        psoDesc.VS = { m_fullscreenVS->GetBufferPointer(), m_fullscreenVS->GetBufferSize() };
        psoDesc.PS = { m_toneMapPS->GetBufferPointer(), m_toneMapPS->GetBufferSize() };
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
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  // LDR output
        psoDesc.SampleDesc.Count = 1;

        m_toneMapPSO = std::make_unique<PipelineState>(m_device);
        HRESULT hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
            &psoDesc,
            IID_PPV_ARGS(m_toneMapPSO->GetAddressOf())
        );

        if (FAILED(hr))
            return false;
    }

    // Bloom extract pipeline
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_bloomRootSig->GetD3D12RootSignature();
        psoDesc.VS = { m_fullscreenVS->GetBufferPointer(), m_fullscreenVS->GetBufferSize() };
        psoDesc.PS = { m_bloomExtractPS->GetBufferPointer(), m_bloomExtractPS->GetBufferSize() };
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
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;  // HDR bloom
        psoDesc.SampleDesc.Count = 1;

        m_bloomExtractPSO = std::make_unique<PipelineState>(m_device);
        HRESULT hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
            &psoDesc,
            IID_PPV_ARGS(m_bloomExtractPSO->GetAddressOf())
        );

        if (FAILED(hr))
            return false;
    }

    // Bloom blur pipeline
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_bloomRootSig->GetD3D12RootSignature();
        psoDesc.VS = { m_fullscreenVS->GetBufferPointer(), m_fullscreenVS->GetBufferSize() };
        psoDesc.PS = { m_bloomBlurPS->GetBufferPointer(), m_bloomBlurPS->GetBufferSize() };
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

        m_bloomBlurPSO = std::make_unique<PipelineState>(m_device);
        HRESULT hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
            &psoDesc,
            IID_PPV_ARGS(m_bloomBlurPSO->GetAddressOf())
        );

        if (FAILED(hr))
            return false;
    }

    return true;
}

void PostProcess::RenderBloom(ID3D12GraphicsCommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_bloomEnabled)
        return;

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { srvHeap->GetD3D12DescriptorHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Step 1: Extract bright pixels from HDR scene to first bloom mip
    {
        // Transition HDR texture to shader resource
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_hdrTexture->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        // Transition first bloom texture to render target
        barrier.Transition.pResource = m_bloomTextures[0]->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

        // Set render target
        commandList->OMSetRenderTargets(1, &m_bloomRTVs[0].cpu, FALSE, nullptr);

        // Set viewport
        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)(m_width / 2), (float)(m_height / 2), 0.0f, 1.0f };
        D3D12_RECT scissor = { 0, 0, (LONG)(m_width / 2), (LONG)(m_height / 2) };
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);

        // Set pipeline
        commandList->SetPipelineState(m_bloomExtractPSO->GetD3D12PipelineState());
        commandList->SetGraphicsRootSignature(m_bloomRootSig->GetD3D12RootSignature());

        // Update constants (use bloom threshold)
        BloomBlurConstants constants;
        constants.texelSize = XMFLOAT2(1.0f / m_width, 1.0f / m_height);
        constants.direction = m_bloomThreshold;  // Repurpose as threshold for extract pass
        m_bloomBlurCB->UpdateData(&constants, sizeof(BloomBlurConstants));

        commandList->SetGraphicsRootConstantBufferView(0, m_bloomBlurCB->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(1, m_hdrSRV.gpu);

        // Draw fullscreen triangle
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);

        // Transition bloom texture to shader resource
        barrier.Transition.pResource = m_bloomTextures[0]->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);
    }

    // Step 2: Gaussian blur (horizontal then vertical)
    commandList->SetPipelineState(m_bloomBlurPSO->GetD3D12PipelineState());

    uint32_t blurWidth = m_width / 2;
    uint32_t blurHeight = m_height / 2;

    // Horizontal blur
    {
        // Transition blur texture to render target
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_bloomBlurTexture->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        commandList->OMSetRenderTargets(1, &m_bloomBlurRTV.cpu, FALSE, nullptr);

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)blurWidth, (float)blurHeight, 0.0f, 1.0f };
        D3D12_RECT scissor = { 0, 0, (LONG)blurWidth, (LONG)blurHeight };
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);

        BloomBlurConstants constants;
        constants.texelSize = XMFLOAT2(1.0f / blurWidth, 1.0f / blurHeight);
        constants.direction = 0.0f;  // Horizontal
        m_bloomBlurCB->UpdateData(&constants, sizeof(BloomBlurConstants));

        commandList->SetGraphicsRootConstantBufferView(0, m_bloomBlurCB->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(1, m_bloomSRVs[0].gpu);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);

        // Transition blur texture to shader resource
        barrier.Transition.pResource = m_bloomBlurTexture->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);
    }

    // Vertical blur (back to bloom texture 0)
    {
        // Transition bloom texture to render target
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_bloomTextures[0]->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        commandList->OMSetRenderTargets(1, &m_bloomRTVs[0].cpu, FALSE, nullptr);

        BloomBlurConstants constants;
        constants.texelSize = XMFLOAT2(1.0f / blurWidth, 1.0f / blurHeight);
        constants.direction = 1.0f;  // Vertical
        m_bloomBlurCB->UpdateData(&constants, sizeof(BloomBlurConstants));

        commandList->SetGraphicsRootConstantBufferView(0, m_bloomBlurCB->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(1, m_bloomBlurSRV.gpu);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);

        // Transition bloom texture to shader resource for final composite
        barrier.Transition.pResource = m_bloomTextures[0]->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);
    }
}

void PostProcess::RenderToneMap(
    ID3D12GraphicsCommandList* commandList,
    DescriptorHeap* srvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
    uint32_t outputWidth,
    uint32_t outputHeight
)
{
    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { srvHeap->GetD3D12DescriptorHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Set render target
    commandList->OMSetRenderTargets(1, &outputRTV, FALSE, nullptr);

    // Set viewport
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)outputWidth, (float)outputHeight, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, (LONG)outputWidth, (LONG)outputHeight };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    // Set pipeline
    commandList->SetPipelineState(m_toneMapPSO->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_toneMapRootSig->GetD3D12RootSignature());

    // Update constants
    PostProcessConstants constants;
    constants.exposure = m_exposure;
    constants.gamma = m_gamma;
    constants.toneMapMode = static_cast<uint32_t>(m_toneMapMode);
    constants.bloomIntensity = m_bloomEnabled ? m_bloomIntensity : 0.0f;
    constants.bloomThreshold = m_bloomThreshold;
    m_postProcessCB->UpdateData(&constants, sizeof(PostProcessConstants));

    // Bind resources
    commandList->SetGraphicsRootConstantBufferView(0, m_postProcessCB->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(1, m_hdrSRV.gpu);
    commandList->SetGraphicsRootDescriptorTable(2, m_bloomSRVs[0].gpu);

    // Draw fullscreen triangle
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
}

void PostProcess::Render(
    ID3D12GraphicsCommandList* commandList,
    DescriptorHeap* srvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
    uint32_t outputWidth,
    uint32_t outputHeight
)
{
    // Render bloom if enabled
    RenderBloom(commandList, srvHeap);

    // Transition HDR to shader resource if not done by bloom
    if (!m_bloomEnabled)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_hdrTexture->GetD3D12Resource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    // Render tone mapping to final output
    RenderToneMap(commandList, srvHeap, outputRTV, outputWidth, outputHeight);

    // Transition HDR back to render target for next frame
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_hdrTexture->GetD3D12Resource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);
}
