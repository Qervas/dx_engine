#include "ShadowMap.h"
#include "../RHI/CommandList.h"
#include "../Scene/Scene.h"
#include <d3d12.h>
#include <d3dcompiler.h>

ShadowMap::ShadowMap(GraphicsDevice* device)
    : m_device(device)
{
    m_lightViewProj = XMMatrixIdentity();
    m_lightView = XMMatrixIdentity();
    m_lightProj = XMMatrixIdentity();
}

ShadowMap::~ShadowMap()
{
    Shutdown();
}

bool ShadowMap::Initialize(uint32_t width, uint32_t height, DescriptorHeap* srvHeap)
{
    m_width = width;
    m_height = height;
    m_srvHeap = srvHeap;

    if (!CreateShadowTexture())
        return false;

    if (!CreatePipeline())
        return false;

    // Create constant buffer for shadow pass
    m_constantBuffer = std::make_unique<Buffer>(m_device);
    if (!m_constantBuffer->Create(sizeof(ShadowConstants), 0, BufferUsage::Constant))
        return false;

    return true;
}

void ShadowMap::Shutdown()
{
    m_constantBuffer.reset();
    m_pipelineState.reset();
    m_rootSignature.reset();
    m_vertexShader.reset();
    m_dsvHeap.Reset();
    m_shadowTexture.reset();
}

bool ShadowMap::CreateShadowTexture()
{
    // Create shadow map texture (depth format)
    m_shadowTexture = std::make_unique<Texture>(m_device);

    // Create as depth stencil with shader resource capability
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;  // Typeless for dual use
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(m_shadowTexture->GetAddressOfResource())
    );

    if (FAILED(hr))
        return false;

#if defined(_DEBUG)
    m_shadowTexture->GetD3D12Resource()->SetName(L"ShadowMap");
#endif

    // Create DSV heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->GetD3D12Device()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
    if (FAILED(hr))
        return false;

    // Create DSV
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    m_device->GetD3D12Device()->CreateDepthStencilView(
        m_shadowTexture->GetD3D12Resource(),
        &dsvDesc,
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    // Create SRV for sampling in main pass
    m_srv = m_srvHeap->Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;  // Read as R32 float
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_shadowTexture->GetD3D12Resource(),
        &srvDesc,
        m_srv.cpu
    );

    m_shadowTexture->SetSRV(m_srv);

    return true;
}

bool ShadowMap::CreatePipeline()
{
    // Compile shadow vertex shader
    m_vertexShader = std::make_unique<Shader>();
    if (!m_vertexShader->CompileFromFile(L"shaders/ShadowVS.hlsl", "main", "vs_5_1"))
        return false;

    // Create root signature for shadow pass (just MVP matrix)
    m_rootSignature = std::make_unique<RootSignature>(m_device);

    // Simple root signature: 1 CBV for light view-projection
    D3D12_ROOT_PARAMETER rootParams[1] = {};

    // Root param 0: Shadow constants (b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
        return false;

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(m_rootSignature->GetAddressOf())
    );
    if (FAILED(hr))
        return false;

    // Create pipeline state for shadow pass (depth-only, no pixel shader)
    D3D12_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature->GetD3D12RootSignature();
    psoDesc.VS = m_vertexShader->GetBytecode();
    // No pixel shader for depth-only pass

    psoDesc.InputLayout.pInputElementDescs = inputElements;
    psoDesc.InputLayout.NumElements = _countof(inputElements);

    // Rasterizer - render back faces to reduce shadow acne
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;  // Cull front faces
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = 10000;  // Depth bias to reduce shadow acne
    psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // Blend state - disabled
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;  // No color output

    // Depth stencil - write depth
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    psoDesc.NumRenderTargets = 0;  // No render targets
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    m_pipelineState = std::make_unique<PipelineState>(m_device);
    hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(&psoDesc,
        IID_PPV_ARGS(m_pipelineState->GetAddressOf()));
    if (FAILED(hr))
        return false;

    return true;
}

void ShadowMap::UpdateLightMatrix(const Light& light, const XMFLOAT3& sceneCenter, float sceneRadius)
{
    // For directional light, create orthographic projection
    XMVECTOR lightDir;

    if (light.GetType() == LightType::Directional)
    {
        lightDir = XMLoadFloat3(&light.GetDirection());
    }
    else
    {
        // For point lights, use direction from light to scene center
        XMFLOAT3 lightPos = light.GetPosition();
        XMVECTOR lightPosVec = XMLoadFloat3(&lightPos);
        XMVECTOR sceneCenterVec = XMLoadFloat3(&sceneCenter);
        lightDir = XMVector3Normalize(XMVectorSubtract(sceneCenterVec, lightPosVec));
    }

    // Position light far enough to see entire scene
    XMVECTOR sceneCenterVec = XMLoadFloat3(&sceneCenter);
    XMVECTOR lightPos = XMVectorSubtract(sceneCenterVec, XMVectorScale(lightDir, sceneRadius * 2.0f));

    // Create view matrix looking at scene center
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // Handle case where light is pointing straight down
    if (fabsf(XMVectorGetX(XMVector3Dot(lightDir, up))) > 0.99f)
    {
        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    m_lightView = XMMatrixLookAtLH(lightPos, sceneCenterVec, up);

    // Create orthographic projection that encompasses the scene
    float orthoSize = sceneRadius * 2.0f;
    m_lightProj = XMMatrixOrthographicLH(orthoSize, orthoSize, 0.1f, sceneRadius * 4.0f);

    m_lightViewProj = m_lightView * m_lightProj;

    // Update constant buffer
    ShadowConstants constants;
    XMStoreFloat4x4(&constants.lightViewProj, XMMatrixTranspose(m_lightViewProj));
    m_constantBuffer->UpdateData(&constants, sizeof(ShadowConstants));
}

void ShadowMap::BeginShadowPass(CommandList* commandList)
{
    // Transition shadow map to depth write state
    commandList->TransitionBarrier(
        m_shadowTexture->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );

    // Set render target (depth only)
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSV();
    commandList->GetD3D12CommandList()->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

    // Set viewport and scissor
    commandList->SetViewport(0, 0, static_cast<float>(m_width), static_cast<float>(m_height));
    commandList->SetScissorRect(0, 0, m_width, m_height);

    // Clear depth buffer
    commandList->ClearDepthStencilView(dsv, 1.0f);

    // Set pipeline state
    commandList->SetPipelineState(m_pipelineState->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature());
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Bind shadow constants
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
}

void ShadowMap::EndShadowPass(CommandList* commandList)
{
    // Transition shadow map back to shader resource for sampling
    commandList->TransitionBarrier(
        m_shadowTexture->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDSV() const
{
    return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}
