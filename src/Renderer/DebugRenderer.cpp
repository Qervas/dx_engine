#include "DebugRenderer.h"
#include <d3dcompiler.h>

DebugRenderer::DebugRenderer(GraphicsDevice* device)
    : m_device(device)
{
}

DebugRenderer::~DebugRenderer()
{
    Shutdown();
}

bool DebugRenderer::Initialize()
{
    if (!CreatePipeline())
        return false;

    // Create vertex buffer (dynamic - use Upload heap for CPU updates every frame)
    m_vertexBuffer = std::make_unique<Buffer>(m_device);
    if (!m_vertexBuffer->Create(MAX_DEBUG_VERTICES * sizeof(DebugVertex), sizeof(DebugVertex), BufferUsage::Upload))
        return false;

    // Create constant buffer
    m_constantBuffer = std::make_unique<Buffer>(m_device);
    if (!m_constantBuffer->Create(sizeof(DebugConstants), 0, BufferUsage::Constant))
        return false;

    return true;
}

void DebugRenderer::Shutdown()
{
    m_constantBuffer.reset();
    m_vertexBuffer.reset();
    m_pipelineState.reset();
    m_rootSignature.reset();
    m_pixelShader.reset();
    m_vertexShader.reset();
    m_vertices.clear();
}

bool DebugRenderer::CreatePipeline()
{
    // Compile debug shaders
    m_vertexShader = std::make_unique<Shader>();
    if (!m_vertexShader->CompileFromFile(L"shaders/DebugVS.hlsl", "main", "vs_5_1"))
        return false;

    m_pixelShader = std::make_unique<Shader>();
    if (!m_pixelShader->CompileFromFile(L"shaders/DebugPS.hlsl", "main", "ps_5_1"))
        return false;

    // Create root signature (1 CBV for view-projection)
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.Descriptor.ShaderRegister = 0;
    rootParam.Descriptor.RegisterSpace = 0;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = &rootParam;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
        return false;

    m_rootSignature = std::make_unique<RootSignature>(m_device);
    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0, signature->GetBufferPointer(), signature->GetBufferSize(),
        IID_PPV_ARGS(m_rootSignature->GetAddressOf()));
    if (FAILED(hr))
        return false;

    // Create pipeline state for line rendering
    D3D12_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature->GetD3D12RootSignature();
    psoDesc.VS = m_vertexShader->GetBytecode();
    psoDesc.PS = m_pixelShader->GetBytecode();

    psoDesc.InputLayout.pInputElementDescs = inputElements;
    psoDesc.InputLayout.NumElements = _countof(inputElements);

    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = 0;
    psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;

    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;  // Don't write depth
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

    m_pipelineState = std::make_unique<PipelineState>(m_device);
    hr = m_device->GetD3D12Device()->CreateGraphicsPipelineState(&psoDesc,
        IID_PPV_ARGS(m_pipelineState->GetAddressOf()));
    if (FAILED(hr))
        return false;

    return true;
}

void DebugRenderer::DrawLine(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT4& color)
{
    if (m_vertices.size() + 2 > MAX_DEBUG_VERTICES)
        return;

    m_vertices.push_back({ start, color });
    m_vertices.push_back({ end, color });
}

void DebugRenderer::DrawLine(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT3& color)
{
    DrawLine(start, end, XMFLOAT4(color.x, color.y, color.z, 1.0f));
}

void DebugRenderer::DrawBox(const XMFLOAT3& center, const XMFLOAT3& size, const XMFLOAT4& color)
{
    XMFLOAT3 half(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
    XMFLOAT3 boxMin(center.x - half.x, center.y - half.y, center.z - half.z);
    XMFLOAT3 boxMax(center.x + half.x, center.y + half.y, center.z + half.z);
    DrawWireBox(boxMin, boxMax, color);
}

void DebugRenderer::DrawWireBox(const XMFLOAT3& boxMin, const XMFLOAT3& boxMax, const XMFLOAT4& color)
{
    // 8 corners of the box
    XMFLOAT3 corners[8] =
    {
        { boxMin.x, boxMin.y, boxMin.z },
        { boxMax.x, boxMin.y, boxMin.z },
        { boxMax.x, boxMax.y, boxMin.z },
        { boxMin.x, boxMax.y, boxMin.z },
        { boxMin.x, boxMin.y, boxMax.z },
        { boxMax.x, boxMin.y, boxMax.z },
        { boxMax.x, boxMax.y, boxMax.z },
        { boxMin.x, boxMax.y, boxMax.z }
    };

    // 12 edges
    DrawLine(corners[0], corners[1], color);
    DrawLine(corners[1], corners[2], color);
    DrawLine(corners[2], corners[3], color);
    DrawLine(corners[3], corners[0], color);

    DrawLine(corners[4], corners[5], color);
    DrawLine(corners[5], corners[6], color);
    DrawLine(corners[6], corners[7], color);
    DrawLine(corners[7], corners[4], color);

    DrawLine(corners[0], corners[4], color);
    DrawLine(corners[1], corners[5], color);
    DrawLine(corners[2], corners[6], color);
    DrawLine(corners[3], corners[7], color);
}

void DebugRenderer::DrawSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color, int segments)
{
    // Draw 3 circles (XY, XZ, YZ planes)
    DrawCircle(center, radius, XMFLOAT3(0, 0, 1), color, segments);  // XY plane
    DrawCircle(center, radius, XMFLOAT3(0, 1, 0), color, segments);  // XZ plane
    DrawCircle(center, radius, XMFLOAT3(1, 0, 0), color, segments);  // YZ plane
}

void DebugRenderer::DrawCircle(const XMFLOAT3& center, float radius, const XMFLOAT3& normal, const XMFLOAT4& color, int segments)
{
    // Build orthonormal basis
    XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&normal));
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (fabsf(XMVectorGetX(XMVector3Dot(n, up))) > 0.99f)
        up = XMVectorSet(1, 0, 0, 0);

    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, n));
    XMVECTOR forward = XMVector3Cross(n, right);

    XMVECTOR centerVec = XMLoadFloat3(&center);

    float angleStep = XM_2PI / static_cast<float>(segments);

    for (int i = 0; i < segments; ++i)
    {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        XMVECTOR p1 = XMVectorAdd(centerVec,
            XMVectorAdd(XMVectorScale(right, cosf(angle1) * radius),
                        XMVectorScale(forward, sinf(angle1) * radius)));
        XMVECTOR p2 = XMVectorAdd(centerVec,
            XMVectorAdd(XMVectorScale(right, cosf(angle2) * radius),
                        XMVectorScale(forward, sinf(angle2) * radius)));

        XMFLOAT3 start, end;
        XMStoreFloat3(&start, p1);
        XMStoreFloat3(&end, p2);
        DrawLine(start, end, color);
    }
}

void DebugRenderer::DrawAxes(const XMFLOAT3& origin, float scale)
{
    // X axis - Red
    DrawLine(origin, XMFLOAT3(origin.x + scale, origin.y, origin.z), XMFLOAT4(1, 0, 0, 1));
    // Y axis - Green
    DrawLine(origin, XMFLOAT3(origin.x, origin.y + scale, origin.z), XMFLOAT4(0, 1, 0, 1));
    // Z axis - Blue
    DrawLine(origin, XMFLOAT3(origin.x, origin.y, origin.z + scale), XMFLOAT4(0, 0, 1, 1));
}

void DebugRenderer::DrawAxes(const XMMATRIX& transform, float scale)
{
    XMVECTOR origin = transform.r[3];
    XMVECTOR right = XMVector3Normalize(transform.r[0]);
    XMVECTOR up = XMVector3Normalize(transform.r[1]);
    XMVECTOR forward = XMVector3Normalize(transform.r[2]);

    XMFLOAT3 o, x, y, z;
    XMStoreFloat3(&o, origin);
    XMStoreFloat3(&x, XMVectorAdd(origin, XMVectorScale(right, scale)));
    XMStoreFloat3(&y, XMVectorAdd(origin, XMVectorScale(up, scale)));
    XMStoreFloat3(&z, XMVectorAdd(origin, XMVectorScale(forward, scale)));

    DrawLine(o, x, XMFLOAT4(1, 0, 0, 1));
    DrawLine(o, y, XMFLOAT4(0, 1, 0, 1));
    DrawLine(o, z, XMFLOAT4(0, 0, 1, 1));
}

void DebugRenderer::DrawGrid(float size, float step, const XMFLOAT4& color)
{
    float halfSize = size * 0.5f;
    int lineCount = static_cast<int>(size / step) + 1;

    for (int i = 0; i < lineCount; ++i)
    {
        float offset = -halfSize + i * step;

        // Lines parallel to Z
        DrawLine(XMFLOAT3(offset, 0, -halfSize), XMFLOAT3(offset, 0, halfSize), color);
        // Lines parallel to X
        DrawLine(XMFLOAT3(-halfSize, 0, offset), XMFLOAT3(halfSize, 0, offset), color);
    }
}

void DebugRenderer::DrawFrustum(const XMMATRIX& viewProj, const XMFLOAT4& color)
{
    // Inverse view-projection to get frustum corners in world space
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    // NDC corners
    XMFLOAT3 ndcCorners[8] =
    {
        { -1, -1, 0 }, { 1, -1, 0 }, { 1, 1, 0 }, { -1, 1, 0 },  // Near plane
        { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 }   // Far plane
    };

    XMFLOAT3 worldCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        XMVECTOR corner = XMLoadFloat3(&ndcCorners[i]);
        corner = XMVector3TransformCoord(corner, invViewProj);
        XMStoreFloat3(&worldCorners[i], corner);
    }

    // Draw near plane
    DrawLine(worldCorners[0], worldCorners[1], color);
    DrawLine(worldCorners[1], worldCorners[2], color);
    DrawLine(worldCorners[2], worldCorners[3], color);
    DrawLine(worldCorners[3], worldCorners[0], color);

    // Draw far plane
    DrawLine(worldCorners[4], worldCorners[5], color);
    DrawLine(worldCorners[5], worldCorners[6], color);
    DrawLine(worldCorners[6], worldCorners[7], color);
    DrawLine(worldCorners[7], worldCorners[4], color);

    // Draw edges connecting near and far
    DrawLine(worldCorners[0], worldCorners[4], color);
    DrawLine(worldCorners[1], worldCorners[5], color);
    DrawLine(worldCorners[2], worldCorners[6], color);
    DrawLine(worldCorners[3], worldCorners[7], color);
}

void DebugRenderer::DrawRay(const XMFLOAT3& origin, const XMFLOAT3& direction, float length, const XMFLOAT4& color)
{
    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&direction));
    XMVECTOR end = XMVectorAdd(XMLoadFloat3(&origin), XMVectorScale(dir, length));

    XMFLOAT3 endPoint;
    XMStoreFloat3(&endPoint, end);

    DrawLine(origin, endPoint, color);
}

void DebugRenderer::DrawArrow(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT4& color, float headSize)
{
    DrawLine(start, end, color);

    // Calculate arrow head
    XMVECTOR s = XMLoadFloat3(&start);
    XMVECTOR e = XMLoadFloat3(&end);
    XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(e, s));

    // Perpendicular vectors
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (fabsf(XMVectorGetX(XMVector3Dot(dir, up))) > 0.99f)
        up = XMVectorSet(1, 0, 0, 0);

    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, dir));
    XMVECTOR actualUp = XMVector3Cross(dir, right);

    XMVECTOR headBase = XMVectorSubtract(e, XMVectorScale(dir, headSize));

    XMFLOAT3 h1, h2, h3, h4;
    XMStoreFloat3(&h1, XMVectorAdd(headBase, XMVectorScale(right, headSize * 0.3f)));
    XMStoreFloat3(&h2, XMVectorSubtract(headBase, XMVectorScale(right, headSize * 0.3f)));
    XMStoreFloat3(&h3, XMVectorAdd(headBase, XMVectorScale(actualUp, headSize * 0.3f)));
    XMStoreFloat3(&h4, XMVectorSubtract(headBase, XMVectorScale(actualUp, headSize * 0.3f)));

    DrawLine(end, h1, color);
    DrawLine(end, h2, color);
    DrawLine(end, h3, color);
    DrawLine(end, h4, color);
}

void DebugRenderer::UpdateVertexBuffer()
{
    if (m_vertices.empty())
        return;

    size_t dataSize = m_vertices.size() * sizeof(DebugVertex);
    if (dataSize > MAX_DEBUG_VERTICES * sizeof(DebugVertex))
        dataSize = MAX_DEBUG_VERTICES * sizeof(DebugVertex);

    m_vertexBuffer->UpdateData(m_vertices.data(), dataSize);
}

void DebugRenderer::Render(CommandList* commandList)
{
    if (m_vertices.empty() || !m_camera)
        return;

    // Update vertex buffer
    UpdateVertexBuffer();

    // Update constant buffer
    DebugConstants constants;
    XMMATRIX viewProj = m_camera->GetViewMatrix() * m_camera->GetProjectionMatrix();
    XMStoreFloat4x4(&constants.viewProjection, XMMatrixTranspose(viewProj));
    m_constantBuffer->UpdateData(&constants, sizeof(DebugConstants));

    // Set pipeline
    commandList->SetPipelineState(m_pipelineState->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature());
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    // Bind constant buffer
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

    // Bind vertex buffer and draw
    D3D12_VERTEX_BUFFER_VIEW vbv = m_vertexBuffer->GetVertexBufferView();
    commandList->SetVertexBuffers(0, 1, &vbv);
    commandList->GetD3D12CommandList()->DrawInstanced(
        static_cast<UINT>(m_vertices.size()), 1, 0, 0);
}

void DebugRenderer::Clear()
{
    m_vertices.clear();
}
