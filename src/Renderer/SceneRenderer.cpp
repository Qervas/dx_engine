#include "SceneRenderer.h"

SceneRenderer::SceneRenderer(GraphicsDevice* device)
    : m_device(device)
{
}

SceneRenderer::~SceneRenderer()
{
}

bool SceneRenderer::Initialize()
{
    // Create per-object constant buffer (for now, one object at a time)
    m_perObjectCB = std::make_unique<Buffer>(m_device);
    if (!m_perObjectCB->Create(sizeof(PerObjectConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    // Create lighting constant buffer
    m_lightingCB = std::make_unique<Buffer>(m_device);
    if (!m_lightingCB->Create(sizeof(SceneLightingData), 0, BufferUsage::Constant))
    {
        return false;
    }

    return true;
}

void SceneRenderer::Update(float deltaTime)
{
    if (!m_scene)
        return;

    // Update the scene (propagates transform hierarchy)
    m_scene->Update(deltaTime);

    // Update lighting constant buffer
    SceneLightingData lightingData = {};
    if (m_camera)
    {
        lightingData.cameraPosition = m_camera->GetPosition();
    }
    lightingData.lightCount = static_cast<uint32_t>(m_lights.size());
    for (uint32_t i = 0; i < m_lights.size() && i < MAX_LIGHTS; ++i)
    {
        lightingData.lights[i] = m_lights[i].GetLightData();
    }

    m_lightingCB->UpdateData(&lightingData, sizeof(SceneLightingData));
}

void SceneRenderer::Render(CommandList* commandList, DescriptorHeap* srvHeap)
{
    if (!m_scene || !m_camera || !m_rootSignature || !m_pipelineState)
        return;

    // Set pipeline state
    commandList->SetPipelineState(m_pipelineState->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature());
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { srvHeap->GetD3D12DescriptorHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Get view and projection matrices
    XMMATRIX viewMatrix = m_camera->GetViewMatrix();
    XMMATRIX projMatrix = m_camera->GetProjectionMatrix();

    // Get component pools
    auto* transformPool = m_scene->GetComponentPool<Transform>();
    auto* rendererPool = m_scene->GetComponentPool<MeshRenderer>();

    if (!transformPool || !rendererPool)
        return;

    // Iterate over all entities with MeshRenderer
    const auto& renderableEntities = rendererPool->GetEntities();
    for (Entity entity : renderableEntities)
    {
        MeshRenderer* renderer = rendererPool->Get(entity);
        if (!renderer || !renderer->IsValid() || !renderer->visible)
            continue;

        Transform* transform = m_scene->GetComponent<Transform>(entity);
        if (!transform)
            continue;

        // Get world matrix from transform
        XMMATRIX worldMatrix = transform->GetWorldMatrix();

        // Update per-object constants
        PerObjectConstants constants;
        XMStoreFloat4x4(&constants.model, XMMatrixTranspose(worldMatrix));
        XMStoreFloat4x4(&constants.view, XMMatrixTranspose(viewMatrix));
        XMStoreFloat4x4(&constants.projection, XMMatrixTranspose(projMatrix));

        m_perObjectCB->UpdateData(&constants, sizeof(PerObjectConstants));

        // Bind resources
        // Root param 0: Per-object constant buffer (b0)
        commandList->SetGraphicsRootConstantBufferView(0, m_perObjectCB->GetGPUVirtualAddress());

        // Root param 1: Material constant buffer (b1)
        Material* material = renderer->material;
        if (material && material->GetConstantBuffer())
        {
            material->UpdateConstants();
            commandList->SetGraphicsRootConstantBufferView(1, material->GetConstantBuffer()->GetGPUVirtualAddress());
        }

        // Root param 2: Lighting constant buffer (b2)
        commandList->SetGraphicsRootConstantBufferView(2, m_lightingCB->GetGPUVirtualAddress());

        // Root param 3: Albedo texture (t0)
        if (material && material->GetAlbedoTexture())
        {
            commandList->SetGraphicsRootDescriptorTable(3, material->GetAlbedoSRV().gpu);
        }

        // Root param 4: Normal map (t1)
        if (material && material->GetNormalTexture())
        {
            commandList->SetGraphicsRootDescriptorTable(4, material->GetNormalSRV().gpu);
        }

        // Draw mesh
        renderer->mesh->Draw(commandList);
    }
}
