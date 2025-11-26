#pragma once

#include "../Scene/Scene.h"
#include "../Scene/Transform.h"
#include "../Scene/MeshRenderer.h"
#include "Camera.h"
#include "Light.h"
#include "ShadowMap.h"
#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/Buffer.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include "../RHI/DescriptorHeap.h"
#include <vector>
#include <memory>

// MVP constant buffer structure
struct PerObjectConstants
{
    XMFLOAT4X4 model;
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
};

class SceneRenderer
{
public:
    SceneRenderer(GraphicsDevice* device);
    ~SceneRenderer();

    bool Initialize();

    // Set the scene to render
    void SetScene(Scene* scene) { m_scene = scene; }

    // Set camera
    void SetCamera(Camera* camera) { m_camera = camera; }

    // Set lights
    void SetLights(const std::vector<Light>& lights) { m_lights = lights; }

    // Set shadow map for rendering
    void SetShadowMap(ShadowMap* shadowMap) { m_shadowMap = shadowMap; }

    // Set shadow constant buffer for main pass
    void SetShadowConstantBuffer(Buffer* shadowCB) { m_shadowCB = shadowCB; }

    // Update the scene (transforms, etc.)
    void Update(float deltaTime);

    // Render shadow pass (depth only)
    void RenderShadowPass(CommandList* commandList);

    // Render all visible entities (main pass)
    void Render(CommandList* commandList, DescriptorHeap* srvHeap);

    // Set rendering pipeline
    void SetPipeline(RootSignature* rootSig, PipelineState* pso)
    {
        m_rootSignature = rootSig;
        m_pipelineState = pso;
    }

    // Get scene bounds for shadow map calculation
    void GetSceneBounds(XMFLOAT3& center, float& radius) const;

private:
    GraphicsDevice* m_device = nullptr;
    Scene* m_scene = nullptr;
    Camera* m_camera = nullptr;
    std::vector<Light> m_lights;

    // Shadow mapping
    ShadowMap* m_shadowMap = nullptr;
    Buffer* m_shadowCB = nullptr;

    // Rendering resources
    RootSignature* m_rootSignature = nullptr;
    PipelineState* m_pipelineState = nullptr;

    // Per-object constant buffer (dynamically sized)
    std::unique_ptr<Buffer> m_perObjectCB;

    // Scene lighting constant buffer
    std::unique_ptr<Buffer> m_lightingCB;

    // Shadow pass constant buffer (light view-proj per object)
    std::unique_ptr<Buffer> m_shadowPassCB;

    // Max entities we can render in one batch
    static constexpr uint32_t MAX_RENDER_OBJECTS = 1024;
};
