#include "SceneRenderer.h"
#include "IBL.h"
#include "SSAO.h"
#include <cfloat>

// Structure for shadow pass per-object data
struct ShadowPassConstants
{
    XMFLOAT4X4 lightViewProj;
    XMFLOAT4X4 model;
};

// Structure for IBL constants
struct IBLConstants
{
    uint32_t prefilteredMipLevels;
    float padding[3];
};

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

    // Create shadow pass constant buffer
    m_shadowPassCB = std::make_unique<Buffer>(m_device);
    if (!m_shadowPassCB->Create(sizeof(ShadowPassConstants), 0, BufferUsage::Constant))
    {
        return false;
    }

    // Create IBL constant buffer
    m_iblCB = std::make_unique<Buffer>(m_device);
    if (!m_iblCB->Create(sizeof(IBLConstants), 0, BufferUsage::Constant))
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

        // Root param 3: Shadow constants (b3)
        if (m_shadowCB)
        {
            commandList->SetGraphicsRootConstantBufferView(3, m_shadowCB->GetGPUVirtualAddress());
        }

        // Check if we're using IBL (different root signature layout)
        if (m_ibl)
        {
            // IBL root signature layout
            // Root param 4: IBL constants (b4)
            IBLConstants iblConst;
            iblConst.prefilteredMipLevels = m_ibl->GetPrefilteredMipLevels();
            m_iblCB->UpdateData(&iblConst, sizeof(IBLConstants));
            commandList->SetGraphicsRootConstantBufferView(4, m_iblCB->GetGPUVirtualAddress());

            // Root param 5: Albedo texture (t0)
            if (material && material->GetAlbedoTexture())
            {
                commandList->SetGraphicsRootDescriptorTable(5, material->GetAlbedoSRV().gpu);
            }

            // Root param 6: Normal map (t1)
            if (material && material->GetNormalTexture())
            {
                commandList->SetGraphicsRootDescriptorTable(6, material->GetNormalSRV().gpu);
            }

            // Root param 7: Shadow map (t2)
            if (m_shadowMap)
            {
                commandList->SetGraphicsRootDescriptorTable(7, m_shadowMap->GetSRV().gpu);
            }

            // Root param 8: Environment cubemap (t3)
            if (m_environmentMap)
            {
                commandList->SetGraphicsRootDescriptorTable(8, m_environmentMap->GetSRV().gpu);
            }

            // Root param 9: Irradiance map (t4)
            if (m_ibl->GetIrradianceMap())
            {
                commandList->SetGraphicsRootDescriptorTable(9, m_ibl->GetIrradianceSRV().gpu);
            }

            // Root param 10: Prefiltered map (t5)
            if (m_ibl->GetPrefilteredMap())
            {
                commandList->SetGraphicsRootDescriptorTable(10, m_ibl->GetPrefilteredSRV().gpu);
            }

            // Root param 11: BRDF LUT (t6)
            if (m_ibl->GetBRDFLUT())
            {
                commandList->SetGraphicsRootDescriptorTable(11, m_ibl->GetBRDFLUTSRV().gpu);
            }

            // Root param 12: SSAO texture (t7) - only if SSAO is enabled
            if (m_ssao)
            {
                commandList->SetGraphicsRootDescriptorTable(12, m_ssao->GetSSAOSRV().gpu);
            }
        }
        else
        {
            // Non-IBL root signature layout (environment only)
            // Root param 4: Albedo texture (t0)
            if (material && material->GetAlbedoTexture())
            {
                commandList->SetGraphicsRootDescriptorTable(4, material->GetAlbedoSRV().gpu);
            }

            // Root param 5: Normal map (t1)
            if (material && material->GetNormalTexture())
            {
                commandList->SetGraphicsRootDescriptorTable(5, material->GetNormalSRV().gpu);
            }

            // Root param 6: Shadow map (t2)
            if (m_shadowMap)
            {
                commandList->SetGraphicsRootDescriptorTable(6, m_shadowMap->GetSRV().gpu);
            }

            // Root param 7: Environment cubemap (t3)
            if (m_environmentMap)
            {
                commandList->SetGraphicsRootDescriptorTable(7, m_environmentMap->GetSRV().gpu);
            }
        }

        // Draw mesh
        renderer->mesh->Draw(commandList);
    }
}

void SceneRenderer::RenderShadowPass(CommandList* commandList)
{
    if (!m_scene || !m_shadowMap || !m_shadowPassCB)
        return;

    // Get component pools
    auto* transformPool = m_scene->GetComponentPool<Transform>();
    auto* rendererPool = m_scene->GetComponentPool<MeshRenderer>();

    if (!transformPool || !rendererPool)
        return;

    // Get light view-projection matrix
    XMMATRIX lightViewProj = m_shadowMap->GetLightViewProjection();

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

        // Calculate final transform: model * lightViewProj
        XMMATRIX finalTransform = worldMatrix * lightViewProj;

        // Update shadow pass constants (just the combined MVP from light's view)
        ShadowConstants shadowConst;
        XMStoreFloat4x4(&shadowConst.lightViewProj, XMMatrixTranspose(finalTransform));
        m_shadowPassCB->UpdateData(&shadowConst, sizeof(ShadowConstants));

        // Bind shadow constant buffer (root param 0 for shadow pass)
        commandList->SetGraphicsRootConstantBufferView(0, m_shadowPassCB->GetGPUVirtualAddress());

        // Draw mesh
        renderer->mesh->Draw(commandList);
    }
}

void SceneRenderer::GetSceneBounds(XMFLOAT3& center, float& radius) const
{
    if (!m_scene)
    {
        center = XMFLOAT3(0.0f, 0.0f, 0.0f);
        radius = 10.0f;
        return;
    }

    auto* transformPool = m_scene->GetComponentPool<Transform>();
    if (!transformPool || transformPool->GetEntities().empty())
    {
        center = XMFLOAT3(0.0f, 0.0f, 0.0f);
        radius = 10.0f;
        return;
    }

    // Calculate bounding sphere of all transforms
    XMVECTOR minBounds = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
    XMVECTOR maxBounds = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

    const auto& entities = transformPool->GetEntities();
    for (Entity entity : entities)
    {
        Transform* transform = transformPool->Get(entity);
        if (transform)
        {
            XMFLOAT3 pos = transform->GetWorldPosition();
            XMVECTOR posVec = XMLoadFloat3(&pos);
            minBounds = XMVectorMin(minBounds, posVec);
            maxBounds = XMVectorMax(maxBounds, posVec);
        }
    }

    // Calculate center and radius
    XMVECTOR centerVec = XMVectorScale(XMVectorAdd(minBounds, maxBounds), 0.5f);
    XMStoreFloat3(&center, centerVec);

    // Radius is half diagonal plus some padding for object sizes
    XMVECTOR diagonal = XMVectorSubtract(maxBounds, minBounds);
    radius = XMVectorGetX(XMVector3Length(diagonal)) * 0.5f + 2.0f;  // +2 for object bounds
}

void SceneRenderer::RenderGBuffer(CommandList* commandList, RootSignature* rootSig, PipelineState* pso)
{
    if (!m_scene || !m_camera)
        return;

    // Set pipeline state
    commandList->SetPipelineState(pso->GetD3D12PipelineState());
    commandList->SetGraphicsRootSignature(rootSig->GetD3D12RootSignature());
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

        // Bind per-object constant buffer (root param 0 for G-Buffer pass)
        commandList->SetGraphicsRootConstantBufferView(0, m_perObjectCB->GetGPUVirtualAddress());

        // Draw mesh
        renderer->mesh->Draw(commandList);
    }
}
