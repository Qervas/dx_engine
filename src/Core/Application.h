#pragma once

#include "../RHI/Device.h"
#include "../RHI/CommandQueue.h"
#include "../RHI/CommandList.h"
#include "../RHI/SwapChain.h"
#include "../RHI/Buffer.h"
#include "../RHI/Texture.h"
#include "../RHI/Shader.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include "../RHI/DescriptorHeap.h"
#include "../RHI/D2DInterop.h"
#include "../RHI/CubemapTexture.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../Renderer/Light.h"
#include "../Renderer/SceneRenderer.h"
#include "../Renderer/ShadowMap.h"
#include "../Renderer/DebugRenderer.h"
#include "../Renderer/PostProcess.h"
#include "../Renderer/Skybox.h"
#include "../Renderer/IBL.h"
#include "../Renderer/GBuffer.h"
#include "../Renderer/SSAO.h"
#include "../Scene/Scene.h"
#include "../Platform/Window.h"
#include "../UI/UIManager.h"
#include "Timer.h"
#include "AppState.h"
#include "Config.h"
#include <memory>
#include <vector>

// Forward declarations
class RenderGraph;
class ShadowPass;
class MainPass;
class SkyboxPass;
class DebugPass;
class GBufferPass;
class SSAOPass;
class PostProcessPass;

class Application
{
public:
    Application();
    ~Application();

    bool Initialize();
    void Run();

private:
    // Initialization
    bool InitializeMinimal();
    bool InitializeGameResources();
    bool InitializeRenderGraph();
    bool InitializeScene();
    bool InitializeRenderingResources();
    void InitializeLights();
    bool InitializeSkybox();
    bool InitializeIBL();
    bool InitializeSSAO();
    bool InitializePostProcess();

    // UI callbacks
    void SetupUICallbacks();
    void SyncUISettings();

    // Config
    void LoadConfig();
    void SaveConfig();
    void ApplyConfigSettings();

    // Event handlers
    void OnResize(uint32_t width, uint32_t height);
    void OnMenuCommand(MenuCommand cmd);

    // State updates
    void UpdateMenu();
    void UpdateSettings();
    void UpdateLoading();
    void UpdatePaused();
    void Update();

    // Rendering
    void RenderMenu();
    void RenderSettings();
    void RenderLoading();
    void RenderPaused();
    void Render();

    void Shutdown();

private:
    Window m_window;
    Timer m_timer;
    std::unique_ptr<GraphicsDevice> m_device;
    std::unique_ptr<CommandQueue> m_commandQueue;
    std::unique_ptr<SwapChain> m_swapChain;
    std::unique_ptr<CommandList> m_commandList;

    // Application state
    AppState m_currentState = AppState::MainMenu;
    AppState m_settingsReturnState = AppState::MainMenu;  // Where to return after closing settings
    bool m_gameResourcesLoaded = false;

    // D2D/DirectWrite interop for UI rendering
    std::unique_ptr<D2DInterop> m_d2dInterop;

    // UI Manager (owns all menus)
    std::unique_ptr<UIManager> m_uiManager;

    // Camera
    std::unique_ptr<Camera> m_camera;

    // Scene management
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<SceneRenderer> m_sceneRenderer;
    Entity m_cubeEntity;

    // Rendering resources
    std::unique_ptr<DescriptorHeap> m_srvHeap;
    std::unique_ptr<Mesh> m_mesh;
    std::unique_ptr<Mesh> m_groundMesh;
    std::unique_ptr<Texture> m_albedoTexture;
    std::unique_ptr<Texture> m_normalTexture;
    std::unique_ptr<Material> m_material;
    std::unique_ptr<Material> m_groundMaterial;
    std::unique_ptr<Shader> m_vertexShader;
    std::unique_ptr<Shader> m_pixelShader;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;

    // Shadow mapping
    std::unique_ptr<ShadowMap> m_shadowMap;
    std::unique_ptr<Buffer> m_shadowConstantBuffer;

    // Render graph
    std::unique_ptr<RenderGraph> m_renderGraph;
    ShadowPass* m_shadowPass = nullptr;
    MainPass* m_mainPass = nullptr;
    SkyboxPass* m_skyboxPass = nullptr;
    DebugPass* m_debugPass = nullptr;

    // Skybox
    std::unique_ptr<CubemapTexture> m_skyCubemap;
    std::unique_ptr<Skybox> m_skybox;

    // Image-Based Lighting
    std::unique_ptr<IBL> m_ibl;

    // SSAO
    std::unique_ptr<GBuffer> m_gBuffer;
    std::unique_ptr<SSAO> m_ssao;
    std::unique_ptr<DescriptorHeap> m_rtvHeap;
    GBufferPass* m_gBufferPass = nullptr;
    SSAOPass* m_ssaoPass = nullptr;

    // Post-Processing
    std::unique_ptr<PostProcess> m_postProcess;
    PostProcessPass* m_postProcessPass = nullptr;

    // Debug renderer
    std::unique_ptr<DebugRenderer> m_debugRenderer;

    // Scene lights
    std::vector<Light> m_lights;

    // Render settings
    bool m_wireframeEnabled = false;
    bool m_debugRenderingEnabled = true;
    bool m_postProcessEnabled = true;
    bool m_bloomEnabled = true;
    bool m_ssaoEnabled = true;
    bool m_vsyncEnabled = true;
};
