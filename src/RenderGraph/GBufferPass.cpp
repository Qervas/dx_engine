#include "GBufferPass.h"
#include "../RHI/CommandList.h"
#include "../Scene/Scene.h"
#include "../Scene/Transform.h"
#include "../Scene/MeshRenderer.h"

GBufferPass::GBufferPass(GraphicsDevice* device, GBuffer* gBuffer, SceneRenderer* sceneRenderer, SwapChain* swapChain)
    : RenderPass("GBuffer Pass")
    , m_device(device)
    , m_gBuffer(gBuffer)
    , m_sceneRenderer(sceneRenderer)
    , m_swapChain(swapChain)
{
}

void GBufferPass::Setup(RenderGraph& graph)
{
    // G-Buffer pass doesn't have render graph resource dependencies for now
    // It writes to its own G-Buffer textures
}

bool GBufferPass::Initialize()
{
    // Compile G-Buffer shaders
    m_vertexShader = std::make_unique<Shader>();
    if (!m_vertexShader->CompileFromFile(L"shaders/GBufferVS.hlsl", "main", "vs_5_1"))
    {
        return false;
    }

    m_pixelShader = std::make_unique<Shader>();
    if (!m_pixelShader->CompileFromFile(L"shaders/GBufferPS.hlsl", "main", "ps_5_1"))
    {
        return false;
    }

    // Create root signature for G-Buffer pass
    // b0: Per-object constants (MVP matrices)
    D3D12_ROOT_PARAMETER rootParams[1] = {};

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

    if (FAILED(hr))
        return false;

    // Create pipeline state with MRT (2 render targets)
    D3D12_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature->GetD3D12RootSignature();
    psoDesc.VS = { m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize() };
    psoDesc.PS = { m_pixelShader->GetBufferPointer(), m_pixelShader->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.InputLayout.pInputElementDescs = inputElements;
    psoDesc.InputLayout.NumElements = _countof(inputElements);
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 2;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;  // Position
    psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;  // Normal
    psoDesc.DSVFormat = m_swapChain->GetDepthFormat();
    psoDesc.SampleDesc.Count = 1;

    m_pipelineState = std::make_unique<PipelineState>(m_device);
    hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(m_pipelineState->GetAddressOf())
    );

    return SUCCEEDED(hr);
}

void GBufferPass::Execute(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_gBuffer || !m_sceneRenderer)
        return;

    // Transition G-Buffer to render target
    m_gBuffer->TransitionToRenderTarget(commandList->GetD3D12CommandList());

    // Set render targets (G-Buffer + depth)
    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] = {
        m_gBuffer->GetPositionRTV(),
        m_gBuffer->GetNormalRTV()
    };

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_swapChain->GetDSV();
    commandList->GetD3D12CommandList()->OMSetRenderTargets(2, rtvs, FALSE, &dsv);

    // Clear G-Buffer
    m_gBuffer->Clear(commandList->GetD3D12CommandList());

    // Clear depth
    commandList->GetD3D12CommandList()->ClearDepthStencilView(
        dsv,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f, 0, 0, nullptr
    );

    // Set viewport and scissor
    D3D12_VIEWPORT viewport = {
        0.0f, 0.0f,
        (float)m_gBuffer->GetWidth(), (float)m_gBuffer->GetHeight(),
        0.0f, 1.0f
    };
    D3D12_RECT scissor = { 0, 0, (LONG)m_gBuffer->GetWidth(), (LONG)m_gBuffer->GetHeight() };
    commandList->GetD3D12CommandList()->RSSetViewports(1, &viewport);
    commandList->GetD3D12CommandList()->RSSetScissorRects(1, &scissor);

    // Set pipeline state
    commandList->SetPipelineState(m_pipelineState->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature());
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Render all entities (simplified version - no materials, just geometry)
    // We need to access the scene through SceneRenderer
    // For now, this pass shares the per-object constant buffer concept

    // Render scene geometry to G-Buffer
    m_sceneRenderer->RenderGBuffer(commandList, m_rootSignature.get(), m_pipelineState.get());

    // Transition G-Buffer to shader resource for SSAO
    m_gBuffer->TransitionToShaderResource(commandList->GetD3D12CommandList());
}
