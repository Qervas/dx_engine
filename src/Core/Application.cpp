#include "Application.h"
#include "../Renderer/ProceduralTexture.h"
#include "../Renderer/CubemapLoader.h"
#include "../Scene/Transform.h"
#include "../Scene/MeshRenderer.h"
#include "../Platform/Input.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/ShadowPass.h"
#include "../RenderGraph/MainPass.h"
#include "../RenderGraph/DebugPass.h"
#include "../RenderGraph/SkyboxPass.h"
#include "../RenderGraph/GBufferPass.h"
#include "../RenderGraph/SSAOPass.h"
#include "../RenderGraph/PostProcessPass.h"
#include <DirectXMath.h>

using namespace DirectX;

// Application constants
constexpr uint32_t WINDOW_WIDTH = 1280;
constexpr uint32_t WINDOW_HEIGHT = 720;
constexpr float CAMERA_FOV = 60.0f;
constexpr float CAMERA_NEAR = 0.1f;
constexpr float CAMERA_FAR = 100.0f;

Application::Application()
    : m_window(L"DirectX 12 Engine - PBR with Normal Mapping", WINDOW_WIDTH, WINDOW_HEIGHT)
{
}

Application::~Application()
{
    Shutdown();
}

bool Application::Initialize()
{
    // Load config first
    LoadConfig();

    // Apply initial window size from config BEFORE window creation
    ApplyInitialDisplaySettings();

    if (!InitializeMinimal())
    {
        return false;
    }

    m_currentState = AppState::MainMenu;
    m_window.SetMouseCaptureEnabled(false);

    return true;
}

bool Application::InitializeMinimal()
{
    if (!m_window.Initialize())
    {
        return false;
    }

    m_window.SetMenuCallback([this](MenuCommand cmd) {
        OnMenuCommand(cmd);
    });

    m_device = std::make_unique<GraphicsDevice>();
    if (!m_device->Initialize())
    {
        return false;
    }

    m_commandQueue = std::make_unique<CommandQueue>(m_device.get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!m_commandQueue->Initialize())
    {
        return false;
    }

    m_swapChain = std::make_unique<SwapChain>(m_device.get(), m_commandQueue.get(), &m_window);
    if (!m_swapChain->Initialize())
    {
        return false;
    }

    m_commandList = std::make_unique<CommandList>(m_device.get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!m_commandList->Initialize())
    {
        return false;
    }

    m_d2dInterop = std::make_unique<D2DInterop>(m_device.get(), m_commandQueue.get());
    if (!m_d2dInterop->Initialize())
    {
        return false;
    }

    if (!m_d2dInterop->CreateWrappedRenderTargets(m_swapChain.get()))
    {
        return false;
    }

    m_uiManager = std::make_unique<UIManager>();
    if (!m_uiManager->Initialize(m_d2dInterop.get(), m_window.GetWidth(), m_window.GetHeight()))
    {
        return false;
    }
    SetupUICallbacks();

    // Create loading screen text format
    IDWriteFactory* dwrite = m_d2dInterop->GetDWriteFactory();
    HRESULT hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_LIGHT,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        36.0f,
        L"en-us",
        &m_loadingTextFormat
    );
    if (FAILED(hr)) return false;

    m_loadingTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_loadingTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return true;
}

bool Application::InitializeGameResources()
{
    if (m_gameResourcesLoaded)
        return true;

    if (!InitializeRenderingResources())
        return false;

    m_camera = std::make_unique<Camera>();
    m_camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -10.0f));
    m_camera->SetPerspective(CAMERA_FOV, (float)m_window.GetWidth() / (float)m_window.GetHeight(), CAMERA_NEAR, CAMERA_FAR);

    InitializeLights();

    if (!InitializeScene())
        return false;

    m_debugRenderer = std::make_unique<DebugRenderer>(m_device.get());
    if (!m_debugRenderer->Initialize())
        return false;
    m_debugRenderer->SetCamera(m_camera.get());

    if (!InitializeSkybox())
        return false;

    if (!InitializeIBL())
        return false;

    m_sceneRenderer->SetEnvironmentMap(m_skyCubemap.get());
    m_sceneRenderer->SetIBL(m_ibl.get());

    if (!InitializeSSAO())
        return false;

    m_sceneRenderer->SetSSAO(m_ssao.get());

    if (!InitializePostProcess())
        return false;

    if (!InitializeRenderGraph())
        return false;

    // Apply loaded config settings to all systems
    ApplyConfigSettings();

    m_gameResourcesLoaded = true;
    return true;
}

bool Application::InitializeRenderGraph()
{
    m_renderGraph = std::make_unique<RenderGraph>(m_device.get());

    m_shadowPass = m_renderGraph->AddPass<ShadowPass>(m_shadowMap.get(), m_sceneRenderer.get());

    m_gBufferPass = m_renderGraph->AddPass<GBufferPass>(m_device.get(), m_gBuffer.get(), m_sceneRenderer.get(), m_swapChain.get());
    if (!m_gBufferPass->Initialize())
        return false;

    m_ssaoPass = m_renderGraph->AddPass<SSAOPass>(m_ssao.get(), m_gBuffer.get(), m_camera.get());

    m_mainPass = m_renderGraph->AddPass<MainPass>(m_sceneRenderer.get(), m_swapChain.get());
    m_mainPass->SetShadowResources(m_shadowMap.get(), m_shadowConstantBuffer.get());

    m_skyboxPass = m_renderGraph->AddPass<SkyboxPass>(m_skybox.get(), m_swapChain.get(), m_camera.get());

    m_postProcessPass = m_renderGraph->AddPass<PostProcessPass>(m_postProcess.get(), m_swapChain.get());

    m_debugPass = m_renderGraph->AddPass<DebugPass>(m_debugRenderer.get(), m_swapChain.get());

    if (m_postProcess)
    {
        m_mainPass->SetCustomRTV(m_postProcess->GetHDRRTV(), m_window.GetWidth(), m_window.GetHeight());
        m_skyboxPass->SetCustomRTV(m_postProcess->GetHDRRTV(), m_window.GetWidth(), m_window.GetHeight());
    }

    m_renderGraph->Compile();

    return true;
}

bool Application::InitializeScene()
{
    m_scene = std::make_unique<Scene>();

    m_sceneRenderer = std::make_unique<SceneRenderer>(m_device.get());
    if (!m_sceneRenderer->Initialize())
        return false;

    m_sceneRenderer->SetScene(m_scene.get());
    m_sceneRenderer->SetCamera(m_camera.get());
    m_sceneRenderer->SetLights(m_lights);
    m_sceneRenderer->SetPipeline(m_rootSignature.get(), m_pipelineState.get());
    m_sceneRenderer->SetShadowMap(m_shadowMap.get());
    m_sceneRenderer->SetShadowConstantBuffer(m_shadowConstantBuffer.get());

    Entity groundEntity = m_scene->CreateEntity("Ground");
    Transform* groundTransform = m_scene->AddComponent<Transform>(groundEntity);
    groundTransform->SetLocalPosition(0.0f, -2.0f, 0.0f);

    MeshRenderer* groundRenderer = m_scene->AddComponent<MeshRenderer>(groundEntity);
    groundRenderer->mesh = m_groundMesh.get();
    groundRenderer->material = m_groundMaterial.get();

    const int gridSize = 3;
    const float spacing = 3.0f;
    const float offset = (gridSize - 1) * spacing * 0.5f;

    for (int x = 0; x < gridSize; ++x)
    {
        for (int z = 0; z < gridSize; ++z)
        {
            Entity cubeEntity = m_scene->CreateEntity("Cube");

            Transform* transform = m_scene->AddComponent<Transform>(cubeEntity);
            transform->SetLocalPosition(
                x * spacing - offset,
                1.0f + (x + z) * 0.3f,
                z * spacing - offset
            );

            MeshRenderer* meshRenderer = m_scene->AddComponent<MeshRenderer>(cubeEntity);
            meshRenderer->mesh = m_mesh.get();
            meshRenderer->material = m_material.get();

            if (x == gridSize / 2 && z == gridSize / 2)
            {
                m_cubeEntity = cubeEntity;
            }
        }
    }

    return true;
}

bool Application::InitializeRenderingResources()
{
    m_srvHeap = std::make_unique<DescriptorHeap>(m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512, true);
    if (!m_srvHeap->Initialize())
        return false;

    m_mesh = std::unique_ptr<Mesh>(Mesh::CreateCubeWithTangents(m_device.get()));
    if (!m_mesh)
        return false;

    m_groundMesh = std::unique_ptr<Mesh>(Mesh::CreatePlaneWithTangents(m_device.get(), 30.0f, 10.0f));
    if (!m_groundMesh)
        return false;

    const uint32_t texWidth = 256;
    const uint32_t texHeight = 256;
    auto checkerboardData = ProceduralTexture::GenerateCheckerboard(texWidth, texHeight, 32);

    m_albedoTexture = std::make_unique<Texture>(m_device.get());
    if (!m_albedoTexture->Create(texWidth, texHeight, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource))
        return false;

    auto normalMapData = ProceduralTexture::GenerateBrickNormalMap(texWidth, texHeight);

    m_normalTexture = std::make_unique<Texture>(m_device.get());
    if (!m_normalTexture->Create(texWidth, texHeight, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource))
        return false;

    m_commandList->Reset();

    Buffer albedoUploadBuffer(m_device.get());
    if (!albedoUploadBuffer.Create(checkerboardData.size(), 0, BufferUsage::Upload, checkerboardData.data()))
        return false;

    Buffer normalUploadBuffer(m_device.get());
    if (!normalUploadBuffer.Create(normalMapData.size(), 0, BufferUsage::Upload, normalMapData.data()))
        return false;

    m_commandList->TransitionBarrier(m_albedoTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_TEXTURE_COPY_LOCATION albedoSrc = {};
    albedoSrc.pResource = albedoUploadBuffer.GetD3D12Resource();
    albedoSrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    albedoSrc.PlacedFootprint.Offset = 0;
    albedoSrc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    albedoSrc.PlacedFootprint.Footprint.Width = texWidth;
    albedoSrc.PlacedFootprint.Footprint.Height = texHeight;
    albedoSrc.PlacedFootprint.Footprint.Depth = 1;
    albedoSrc.PlacedFootprint.Footprint.RowPitch = texWidth * 4;

    D3D12_TEXTURE_COPY_LOCATION albedoDst = {};
    albedoDst.pResource = m_albedoTexture->GetD3D12Resource();
    albedoDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    albedoDst.SubresourceIndex = 0;

    m_commandList->GetD3D12CommandList()->CopyTextureRegion(&albedoDst, 0, 0, 0, &albedoSrc, nullptr);

    m_commandList->TransitionBarrier(m_albedoTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_commandList->TransitionBarrier(m_normalTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_TEXTURE_COPY_LOCATION normalSrc = {};
    normalSrc.pResource = normalUploadBuffer.GetD3D12Resource();
    normalSrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    normalSrc.PlacedFootprint.Offset = 0;
    normalSrc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    normalSrc.PlacedFootprint.Footprint.Width = texWidth;
    normalSrc.PlacedFootprint.Footprint.Height = texHeight;
    normalSrc.PlacedFootprint.Footprint.Depth = 1;
    normalSrc.PlacedFootprint.Footprint.RowPitch = texWidth * 4;

    D3D12_TEXTURE_COPY_LOCATION normalDst = {};
    normalDst.pResource = m_normalTexture->GetD3D12Resource();
    normalDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    normalDst.SubresourceIndex = 0;

    m_commandList->GetD3D12CommandList()->CopyTextureRegion(&normalDst, 0, 0, 0, &normalSrc, nullptr);

    m_commandList->TransitionBarrier(m_normalTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_commandList->Close();

    ID3D12CommandList* commandLists[] = { m_commandList->GetD3D12CommandList() };
    m_commandQueue->ExecuteCommandLists(commandLists, 1);
    m_commandQueue->Flush();

    DescriptorHandle albedoSrvHandle = m_srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_device->GetD3D12Device()->CreateShaderResourceView(m_albedoTexture->GetD3D12Resource(), &srvDesc, albedoSrvHandle.cpu);
    m_albedoTexture->SetSRV(albedoSrvHandle);

    DescriptorHandle normalSrvHandle = m_srvHeap->Allocate();
    m_device->GetD3D12Device()->CreateShaderResourceView(m_normalTexture->GetD3D12Resource(), &srvDesc, normalSrvHandle.cpu);
    m_normalTexture->SetSRV(normalSrvHandle);

    m_material = std::make_unique<Material>(m_device.get());
    m_material->SetAlbedo(XMFLOAT3(0.9f, 0.9f, 0.95f));
    m_material->SetMetallic(0.8f);
    m_material->SetRoughness(0.2f);
    m_material->SetAO(1.0f);
    m_material->SetAlbedoTexture(m_albedoTexture.get(), albedoSrvHandle);
    m_material->SetNormalTexture(m_normalTexture.get(), normalSrvHandle);

    m_groundMaterial = std::make_unique<Material>(m_device.get());
    m_groundMaterial->SetAlbedo(XMFLOAT3(0.5f, 0.5f, 0.5f));
    m_groundMaterial->SetMetallic(0.0f);
    m_groundMaterial->SetRoughness(0.9f);
    m_groundMaterial->SetAO(1.0f);
    m_groundMaterial->SetAlbedoTexture(m_albedoTexture.get(), albedoSrvHandle);
    m_groundMaterial->SetNormalTexture(m_normalTexture.get(), normalSrvHandle);

    m_shadowMap = std::make_unique<ShadowMap>(m_device.get());
    if (!m_shadowMap->Initialize(2048, 2048, m_srvHeap.get()))
        return false;

    m_shadowConstantBuffer = std::make_unique<Buffer>(m_device.get());
    if (!m_shadowConstantBuffer->Create(sizeof(ShadowConstants), 0, BufferUsage::Constant))
        return false;

    m_vertexShader = std::make_unique<Shader>();
    if (!m_vertexShader->CompileFromFile(L"shaders/PBRShadowVS.hlsl", "main", "vs_5_1"))
        return false;

    m_pixelShader = std::make_unique<Shader>();
    if (!m_pixelShader->CompileFromFile(L"shaders/PBRIblSSAOPS.hlsl", "main", "ps_5_1"))
        return false;

    m_rootSignature = std::make_unique<RootSignature>(m_device.get());
    if (!m_rootSignature->CreateForPBRWithSSAO())
        return false;

    D3D12_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_INPUT_LAYOUT_DESC inputLayout = {};
    inputLayout.pInputElementDescs = inputElements;
    inputLayout.NumElements = _countof(inputElements);

    m_pipelineState = std::make_unique<PipelineState>(m_device.get());
    if (!m_pipelineState->CreateGraphics(m_rootSignature.get(), m_vertexShader.get(), m_pixelShader.get(), inputLayout, DXGI_FORMAT_R8G8B8A8_UNORM, m_swapChain->GetDepthFormat()))
        return false;

    return true;
}

void Application::InitializeLights()
{
    m_lights.resize(2);

    m_lights[0].SetType(LightType::Directional);
    m_lights[0].SetDirection(XMFLOAT3(0.5f, -1.0f, 0.5f));
    m_lights[0].SetColor(XMFLOAT3(1.0f, 0.95f, 0.8f));
    m_lights[0].SetIntensity(1.5f);

    m_lights[1].SetType(LightType::Point);
    m_lights[1].SetPosition(XMFLOAT3(-3.0f, 2.0f, -3.0f));
    m_lights[1].SetColor(XMFLOAT3(0.4f, 0.5f, 0.7f));
    m_lights[1].SetIntensity(0.8f);
    m_lights[1].SetRange(20.0f);
}

bool Application::InitializeSkybox()
{
    m_skyCubemap = std::unique_ptr<CubemapTexture>(
        CubemapLoader::GenerateGradientSky(m_device.get(), m_commandList.get(), m_commandQueue.get(), m_srvHeap.get(), 512)
    );

    if (!m_skyCubemap)
        return false;

    m_skybox = std::make_unique<Skybox>(m_device.get());
    if (!m_skybox->Initialize(m_skyCubemap.get(), DXGI_FORMAT_R8G8B8A8_UNORM, m_swapChain->GetDepthFormat()))
        return false;

    return true;
}

bool Application::InitializeIBL()
{
    m_ibl = std::make_unique<IBL>(m_device.get());

    if (!m_ibl->Generate(m_skyCubemap.get(), m_commandList.get(), m_commandQueue.get(), m_srvHeap.get()))
        return false;

    return true;
}

bool Application::InitializeSSAO()
{
    m_rtvHeap = std::make_unique<DescriptorHeap>(m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64, false);
    if (!m_rtvHeap->Initialize())
        return false;

    m_gBuffer = std::make_unique<GBuffer>(m_device.get());
    if (!m_gBuffer->Initialize(m_window.GetWidth(), m_window.GetHeight(), m_srvHeap.get(), m_rtvHeap.get()))
        return false;

    m_ssao = std::make_unique<SSAO>(m_device.get());
    if (!m_ssao->Initialize(m_window.GetWidth(), m_window.GetHeight(), m_srvHeap.get(), m_rtvHeap.get()))
        return false;

    return true;
}

bool Application::InitializePostProcess()
{
    m_postProcess = std::make_unique<PostProcess>(m_device.get());
    if (!m_postProcess->Initialize(m_window.GetWidth(), m_window.GetHeight(), m_srvHeap.get(), m_rtvHeap.get()))
        return false;

    m_postProcess->SetExposure(1.0f);
    m_postProcess->SetGamma(2.2f);
    m_postProcess->SetToneMapMode(ToneMapMode::ACES);
    m_postProcess->SetBloomEnabled(true);
    m_postProcess->SetBloomIntensity(0.3f);
    m_postProcess->SetBloomThreshold(1.5f);

    return true;
}

void Application::SetupUICallbacks()
{
    UISettingsCallbacks callbacks;

    // Graphics callbacks - update Config (single source of truth) and apply to systems
    callbacks.graphics.onPostProcessChanged = [this](bool enabled) {
        Config::Get().Graphics().postProcessEnabled = enabled;
        Config::Get().MarkDirty();
        if (m_postProcessPass) m_postProcessPass->SetEnabled(enabled);
        m_window.SetMenuChecked(MenuCommand::SettingsPostProcess, enabled);
        if (enabled && m_postProcess) {
            m_mainPass->SetCustomRTV(m_postProcess->GetHDRRTV(), m_window.GetWidth(), m_window.GetHeight());
            m_skyboxPass->SetCustomRTV(m_postProcess->GetHDRRTV(), m_window.GetWidth(), m_window.GetHeight());
        } else if (m_mainPass && m_skyboxPass) {
            m_mainPass->ClearCustomRTV();
            m_skyboxPass->ClearCustomRTV();
        }
    };

    callbacks.graphics.onBloomChanged = [this](bool enabled) {
        Config::Get().Graphics().bloomEnabled = enabled;
        Config::Get().MarkDirty();
        if (m_postProcess) m_postProcess->SetBloomEnabled(enabled);
        m_window.SetMenuChecked(MenuCommand::SettingsBloom, enabled);
    };

    callbacks.graphics.onBloomIntensityChanged = [this](float value) {
        Config::Get().Graphics().bloomIntensity = value;
        Config::Get().MarkDirty();
        if (m_postProcess) m_postProcess->SetBloomIntensity(value);
    };

    callbacks.graphics.onBloomThresholdChanged = [this](float value) {
        Config::Get().Graphics().bloomThreshold = value;
        Config::Get().MarkDirty();
        if (m_postProcess) m_postProcess->SetBloomThreshold(value);
    };

    callbacks.graphics.onToneMappingChanged = [this](int mode) {
        Config::Get().Graphics().toneMappingMode = mode;
        Config::Get().MarkDirty();
        if (m_postProcess) m_postProcess->SetToneMapMode(static_cast<ToneMapMode>(mode));
    };

    callbacks.graphics.onExposureChanged = [this](float value) {
        Config::Get().Graphics().exposure = value;
        Config::Get().MarkDirty();
        if (m_postProcess) m_postProcess->SetExposure(value);
    };

    callbacks.graphics.onGammaChanged = [this](float value) {
        Config::Get().Graphics().gamma = value;
        Config::Get().MarkDirty();
        if (m_postProcess) m_postProcess->SetGamma(value);
    };

    callbacks.graphics.onSSAOChanged = [this](bool enabled) {
        Config::Get().Graphics().ssaoEnabled = enabled;
        Config::Get().MarkDirty();
        if (m_ssaoPass) m_ssaoPass->SetEnabled(enabled);
        m_window.SetMenuChecked(MenuCommand::SettingsSSAO, enabled);
    };

    callbacks.graphics.onSSAORadiusChanged = [this](float value) {
        Config::Get().Graphics().ssaoRadius = value;
        Config::Get().MarkDirty();
        if (m_ssao) m_ssao->SetRadius(value);
    };

    callbacks.graphics.onSSAOIntensityChanged = [this](float value) {
        Config::Get().Graphics().ssaoIntensity = value;
        Config::Get().MarkDirty();
        if (m_ssao) m_ssao->SetIntensity(value);
    };

    // Display callbacks
    callbacks.display.onDisplayModeChanged = [this](int mode) {
        Config::Get().Display().displayMode = mode;
        Config::Get().MarkDirty();
        m_window.SetDisplayMode(static_cast<WindowDisplayMode>(mode));
    };

    callbacks.display.onResolutionChanged = [this](int index) {
        Config::Get().Display().resolutionIndex = index;
        Config::Get().MarkDirty();
        auto resolutions = Window::GetAvailableResolutions();
        if (index >= 0 && index < static_cast<int>(resolutions.size()))
        {
            m_window.SetResolution(resolutions[index].width, resolutions[index].height);
        }
    };

    callbacks.display.onVSyncChanged = [this](bool enabled) {
        Config::Get().Display().vsyncEnabled = enabled;
        Config::Get().MarkDirty();
        m_window.SetMenuChecked(MenuCommand::SettingsVSync, enabled);
    };

    callbacks.display.onWireframeChanged = [this](bool enabled) {
        Config::Get().Display().wireframeEnabled = enabled;
        Config::Get().MarkDirty();
        m_window.SetMenuChecked(MenuCommand::ViewWireframe, enabled);
    };

    callbacks.display.onDebugRenderingChanged = [this](bool enabled) {
        Config::Get().Display().debugRenderingEnabled = enabled;
        Config::Get().MarkDirty();
        if (m_debugPass) m_debugPass->SetEnabled(enabled);
        m_window.SetMenuChecked(MenuCommand::ViewDebugRendering, enabled);
    };

    m_uiManager->SetSettingsCallbacks(callbacks);
}

void Application::SyncUISettings()
{
    // Read from Config as the single source of truth
    auto& cfg = Config::Get();
    UISettingsValues values;

    // Graphics values from Config
    values.graphics.postProcessEnabled = cfg.Graphics().postProcessEnabled;
    values.graphics.bloomEnabled = cfg.Graphics().bloomEnabled;
    values.graphics.bloomIntensity = cfg.Graphics().bloomIntensity;
    values.graphics.bloomThreshold = cfg.Graphics().bloomThreshold;
    values.graphics.toneMappingMode = cfg.Graphics().toneMappingMode;
    values.graphics.exposure = cfg.Graphics().exposure;
    values.graphics.gamma = cfg.Graphics().gamma;
    values.graphics.ssaoEnabled = cfg.Graphics().ssaoEnabled;
    values.graphics.ssaoRadius = cfg.Graphics().ssaoRadius;
    values.graphics.ssaoIntensity = cfg.Graphics().ssaoIntensity;

    // Display values from Config
    values.display.displayMode = cfg.Display().displayMode;
    values.display.resolutionIndex = cfg.Display().resolutionIndex;
    values.display.vsyncEnabled = cfg.Display().vsyncEnabled;
    values.display.wireframeEnabled = cfg.Display().wireframeEnabled;
    values.display.debugRenderingEnabled = cfg.Display().debugRenderingEnabled;

    m_uiManager->SyncSettingsValues(values);
}

int Application::GetCurrentResolutionIndex() const
{
    auto resolutions = Window::GetAvailableResolutions();
    uint32_t w = m_window.GetWidth();
    uint32_t h = m_window.GetHeight();

    for (int i = 0; i < static_cast<int>(resolutions.size()); i++)
    {
        if (resolutions[i].width == w && resolutions[i].height == h)
        {
            return i;
        }
    }

    // Default to first resolution if no match
    return 0;
}

void Application::OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    // Don't resize if core resources aren't ready yet
    if (!m_commandQueue || !m_swapChain)
        return;

    // Full D2D interop destruction before resize (required for D3D11On12)
    if (m_d2dInterop)
    {
        m_d2dInterop->PrepareForResize();
    }

    // Now flush to wait for all GPU work to complete
    m_commandQueue->Flush();

    m_swapChain->Resize(width, height);

    // Recreate D2D interop with new swapchain
    if (m_d2dInterop)
    {
        m_d2dInterop->RecreateAfterResize(m_swapChain.get());
    }

    if (m_uiManager)
    {
        m_uiManager->OnResize(width, height);
    }

    if (m_gBuffer)
    {
        m_gBuffer->Resize(width, height, m_srvHeap.get(), m_rtvHeap.get());
    }
    if (m_ssao)
    {
        m_ssao->Resize(width, height, m_srvHeap.get(), m_rtvHeap.get());
    }

    if (m_postProcess)
    {
        m_postProcess->Resize(width, height, m_srvHeap.get(), m_rtvHeap.get());

        if (m_mainPass)
        {
            m_mainPass->SetCustomRTV(m_postProcess->GetHDRRTV(), width, height);
        }
        if (m_skyboxPass)
        {
            m_skyboxPass->SetCustomRTV(m_postProcess->GetHDRRTV(), width, height);
        }
    }

    if (m_camera)
    {
        float aspectRatio = (float)width / (float)height;
        m_camera->SetPerspective(CAMERA_FOV, aspectRatio, CAMERA_NEAR, CAMERA_FAR);
    }
}

void Application::OnMenuCommand(MenuCommand cmd)
{
    auto& cfg = Config::Get();
    bool checked = m_window.IsMenuChecked(cmd);

    switch (cmd)
    {
    case MenuCommand::ViewWireframe:
        cfg.Display().wireframeEnabled = checked;
        cfg.MarkDirty();
        break;

    case MenuCommand::ViewDebugRendering:
        cfg.Display().debugRenderingEnabled = checked;
        cfg.MarkDirty();
        if (m_debugPass)
            m_debugPass->SetEnabled(checked);
        break;

    case MenuCommand::SettingsPostProcess:
        cfg.Graphics().postProcessEnabled = checked;
        cfg.MarkDirty();
        if (m_postProcessPass)
            m_postProcessPass->SetEnabled(checked);
        if (checked)
        {
            m_mainPass->SetCustomRTV(m_postProcess->GetHDRRTV(), m_window.GetWidth(), m_window.GetHeight());
            m_skyboxPass->SetCustomRTV(m_postProcess->GetHDRRTV(), m_window.GetWidth(), m_window.GetHeight());
        }
        else
        {
            m_mainPass->ClearCustomRTV();
            m_skyboxPass->ClearCustomRTV();
        }
        break;

    case MenuCommand::SettingsBloom:
        cfg.Graphics().bloomEnabled = checked;
        cfg.MarkDirty();
        if (m_postProcess)
            m_postProcess->SetBloomEnabled(checked);
        break;

    case MenuCommand::SettingsSSAO:
        cfg.Graphics().ssaoEnabled = checked;
        cfg.MarkDirty();
        if (m_ssaoPass)
            m_ssaoPass->SetEnabled(checked);
        break;

    case MenuCommand::SettingsVSync:
        cfg.Display().vsyncEnabled = checked;
        cfg.MarkDirty();
        break;

    case MenuCommand::ViewFullscreen:
        break;
    }
}

void Application::Run()
{
    m_timer.Reset();

    while (m_window.ProcessMessages())
    {
        if (m_window.WasResized())
        {
            OnResize(m_window.GetWidth(), m_window.GetHeight());
            m_window.ClearResizeFlag();
        }

        m_timer.Tick();
        Input::Get().Update();

        switch (m_currentState)
        {
        case AppState::MainMenu:
            UpdateMenu();
            RenderMenu();
            break;

        case AppState::Settings:
            UpdateSettings();
            RenderSettings();
            break;

        case AppState::Loading:
            UpdateLoading();
            RenderLoading();
            break;

        case AppState::InGame:
            Update();
            Render();
            break;

        case AppState::Paused:
            UpdatePaused();
            RenderPaused();
            break;
        }
    }

    m_commandQueue->Flush();
}

void Application::UpdateMenu()
{
    auto& input = Input::Get();
    float mouseX = static_cast<float>(input.GetMouseX());
    float mouseY = static_cast<float>(input.GetMouseY());
    bool mouseClicked = input.IsMouseButtonPressed(MouseButton::Left);

    m_uiManager->UpdateMainMenu(m_timer.GetDeltaTime(), mouseX, mouseY, mouseClicked);

    if (m_uiManager->ShouldStartGame())
    {
        m_currentState = AppState::Loading;
    }
    else if (m_uiManager->ShouldOpenSettings())
    {
        m_settingsReturnState = AppState::MainMenu;
        m_currentState = AppState::Settings;
        SyncUISettings();
    }
    else if (m_uiManager->ShouldExit())
    {
        PostQuitMessage(0);
    }
    m_uiManager->ClearTransitionFlags();
}

void Application::RenderMenu()
{
    uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    m_d2dInterop->BeginD2DDraw(backBufferIndex);
    m_uiManager->RenderMainMenu(m_d2dInterop.get());
    m_d2dInterop->EndD2DDraw();
    m_swapChain->Present(Config::Get().Display().vsyncEnabled);
}

void Application::UpdateLoading()
{
    // If already loaded, skip to game immediately
    if (m_gameResourcesLoaded)
    {
        m_loadingStage = 0;
        m_currentState = AppState::InGame;
        m_window.SetMouseCaptureEnabled(true);
        Input::Get().SetMouseCaptured(false);
        return;
    }

    // Staged loading - each frame processes one stage and renders progress
    const int TOTAL_STAGES = 9;
    bool success = true;

    switch (m_loadingStage)
    {
    case 0:
        m_loadingStatus = L"Initializing...";
        break;

    case 1:
        m_loadingStatus = L"Loading meshes and textures...";
        success = InitializeRenderingResources();
        break;

    case 2:
        m_loadingStatus = L"Setting up camera and lights...";
        m_camera = std::make_unique<Camera>();
        m_camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -10.0f));
        m_camera->SetPerspective(CAMERA_FOV, (float)m_window.GetWidth() / (float)m_window.GetHeight(), CAMERA_NEAR, CAMERA_FAR);
        InitializeLights();
        success = InitializeScene();
        break;

    case 3:
        m_loadingStatus = L"Creating debug renderer...";
        m_debugRenderer = std::make_unique<DebugRenderer>(m_device.get());
        success = m_debugRenderer->Initialize();
        if (success) m_debugRenderer->SetCamera(m_camera.get());
        break;

    case 4:
        m_loadingStatus = L"Generating skybox...";
        success = InitializeSkybox();
        break;

    case 5:
        m_loadingStatus = L"Computing IBL maps...";
        success = InitializeIBL();
        if (success)
        {
            m_sceneRenderer->SetEnvironmentMap(m_skyCubemap.get());
            m_sceneRenderer->SetIBL(m_ibl.get());
        }
        break;

    case 6:
        m_loadingStatus = L"Initializing SSAO...";
        success = InitializeSSAO();
        if (success) m_sceneRenderer->SetSSAO(m_ssao.get());
        break;

    case 7:
        m_loadingStatus = L"Setting up post-processing...";
        success = InitializePostProcess();
        break;

    case 8:
        m_loadingStatus = L"Building render graph...";
        success = InitializeRenderGraph();
        if (success) ApplyConfigSettings();
        break;

    case 9:
        // Loading complete
        m_loadingStage = 0;
        m_gameResourcesLoaded = true;
        m_currentState = AppState::InGame;
        m_window.SetMouseCaptureEnabled(true);
        Input::Get().SetMouseCaptured(false);
        return;
    }

    if (!success)
    {
        m_loadingStage = 0;
        MessageBox(m_window.GetHandle(), L"Failed to load game resources", L"Error", MB_OK);
        m_currentState = AppState::MainMenu;
        m_window.SetMouseCaptureEnabled(false);
        return;
    }

    m_loadingStage++;
}

void Application::RenderLoading()
{
    uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    m_d2dInterop->BeginD2DDraw(backBufferIndex);
    m_d2dInterop->Clear(0.02f, 0.02f, 0.05f, 1.0f);

    float screenW = static_cast<float>(m_window.GetWidth());
    float screenH = static_cast<float>(m_window.GetHeight());

    const int TOTAL_STAGES = 9;
    float progress = static_cast<float>(m_loadingStage) / static_cast<float>(TOTAL_STAGES);

    // Draw "Loading" title
    m_d2dInterop->SetBrushColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_d2dInterop->DrawText(L"Loading", m_loadingTextFormat.Get(), 0.0f, -30.0f, screenW, screenH);

    // Draw current status text (smaller, below title)
    m_d2dInterop->SetBrushColor(0.7f, 0.7f, 0.8f, 1.0f);
    m_d2dInterop->DrawText(m_loadingStatus, m_loadingTextFormat.Get(), 0.0f, 20.0f, screenW, screenH);

    // Draw progress bar background
    float barWidth = 400.0f;
    float barHeight = 8.0f;
    float barX = (screenW - barWidth) / 2.0f;
    float barY = screenH / 2.0f + 60.0f;

    m_d2dInterop->SetBrushColor(0.2f, 0.2f, 0.25f, 1.0f);
    m_d2dInterop->FillRoundedRect(barX, barY, barWidth, barHeight, 4.0f);

    // Draw progress bar fill
    float fillWidth = barWidth * progress;
    if (fillWidth > 0)
    {
        m_d2dInterop->SetBrushColor(0.3f, 0.6f, 1.0f, 1.0f);
        m_d2dInterop->FillRoundedRect(barX, barY, fillWidth, barHeight, 4.0f);
    }

    // Draw percentage text
    wchar_t percentText[16];
    swprintf_s(percentText, L"%d%%", static_cast<int>(progress * 100));
    m_d2dInterop->SetBrushColor(0.6f, 0.6f, 0.7f, 1.0f);
    m_d2dInterop->DrawText(percentText, m_loadingTextFormat.Get(), 0.0f, 90.0f, screenW, screenH);

    m_d2dInterop->EndD2DDraw();
    m_swapChain->Present(Config::Get().Display().vsyncEnabled);
}

void Application::UpdatePaused()
{
    auto& input = Input::Get();
    float mouseX = static_cast<float>(input.GetMouseX());
    float mouseY = static_cast<float>(input.GetMouseY());
    bool mouseClicked = input.IsMouseButtonPressed(MouseButton::Left);

    m_uiManager->UpdatePaused(m_timer.GetDeltaTime(), mouseX, mouseY, mouseClicked);

    if (m_uiManager->ShouldResume())
    {
        m_currentState = AppState::InGame;
        m_window.SetMouseCaptureEnabled(true);
    }
    else if (m_uiManager->ShouldOpenSettings())
    {
        m_settingsReturnState = AppState::Paused;
        m_currentState = AppState::Settings;
        SyncUISettings();
    }
    else if (m_uiManager->ShouldReturnToMenu())
    {
        m_currentState = AppState::MainMenu;
        m_window.SetMouseCaptureEnabled(false);
    }
    else if (m_uiManager->ShouldExit())
    {
        PostQuitMessage(0);
    }
    m_uiManager->ClearTransitionFlags();
}

void Application::RenderPaused()
{
    RenderScene();
    m_commandQueue->Flush();

    uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    m_d2dInterop->BeginD2DDraw(backBufferIndex);
    m_uiManager->RenderPaused(m_d2dInterop.get());
    m_d2dInterop->EndD2DDraw();

    m_swapChain->Present(Config::Get().Display().vsyncEnabled);
    m_commandQueue->Flush();
}

void Application::UpdateSettings()
{
    auto& input = Input::Get();
    float mouseX = static_cast<float>(input.GetMouseX());
    float mouseY = static_cast<float>(input.GetMouseY());
    bool mouseDown = input.IsMouseButtonDown(MouseButton::Left);
    bool mouseClicked = input.IsMouseButtonPressed(MouseButton::Left);

    m_uiManager->UpdateSettings(m_timer.GetDeltaTime(), mouseX, mouseY, mouseDown, mouseClicked);

    if (m_uiManager->ShouldCloseSettings())
    {
        // Save any dirty settings when closing
        Config::Get().SaveIfDirty();
        m_currentState = m_settingsReturnState;
    }
    m_uiManager->ClearTransitionFlags();
}

void Application::RenderSettings()
{
    uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    m_d2dInterop->BeginD2DDraw(backBufferIndex);
    m_uiManager->RenderSettings(m_d2dInterop.get());
    m_d2dInterop->EndD2DDraw();
    m_swapChain->Present(Config::Get().Display().vsyncEnabled);
}

void Application::Update()
{
    float deltaTime = m_timer.GetDeltaTime();

    if (Input::Get().IsKeyPressed(Key::Escape))
    {
        if (!Input::Get().IsMouseCaptured())
        {
            m_currentState = AppState::Paused;
            m_window.SetMouseCaptureEnabled(false);
            return;
        }
    }

    m_camera->ProcessFPSInput(deltaTime, 5.0f, 0.003f);

    m_sceneRenderer->Update(deltaTime);

    if (Config::Get().Display().debugRenderingEnabled)
    {
        m_debugRenderer->Clear();

        m_debugRenderer->DrawAxes(XMFLOAT3(0.0f, 0.0f, 0.0f), 2.0f);
        m_debugRenderer->DrawGrid(20.0f, 1.0f, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));

        const auto& entities = m_scene->GetEntitiesWithComponent<Transform>();
        for (Entity entity : entities)
        {
            Transform* transform = m_scene->GetComponent<Transform>(entity);
            if (transform)
            {
                XMFLOAT3 pos = transform->GetWorldPosition();
                if (pos.y < -1.0f)
                    continue;

                m_debugRenderer->DrawWireBox(
                    XMFLOAT3(pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f),
                    XMFLOAT3(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f),
                    XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
                );
            }
        }

        XMFLOAT3 lightDir = m_lights[0].GetDirection();
        m_debugRenderer->DrawArrow(
            XMFLOAT3(0.0f, 5.0f, 0.0f),
            XMFLOAT3(-lightDir.x * 3.0f, 5.0f - lightDir.y * 3.0f, -lightDir.z * 3.0f),
            XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f),
            0.2f
        );
    }

    static float fpsUpdateTimer = 0.0f;
    fpsUpdateTimer += deltaTime;
    if (fpsUpdateTimer >= 0.5f)
    {
        fpsUpdateTimer = 0.0f;
        wchar_t title[128];
        swprintf_s(title, L"DX12 Engine - FPS: %.1f | Click to capture mouse, ESC to release", m_timer.GetFPS());
        SetWindowText(m_window.GetHandle(), title);
    }
}

void Application::RenderScene()
{
    uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    ID3D12Resource* backBuffer = m_swapChain->GetBackBuffer(backBufferIndex);

    m_commandList->Reset();

    m_renderGraph->BeginFrame();

    XMFLOAT3 sceneCenter;
    float sceneRadius;
    m_sceneRenderer->GetSceneBounds(sceneCenter, sceneRadius);

    m_shadowPass->SetLightInfo(&m_lights[0], sceneCenter, sceneRadius);
    m_mainPass->SetBackBufferIndex(backBufferIndex);
    m_skyboxPass->SetBackBufferIndex(backBufferIndex);
    m_debugPass->SetBackBufferIndex(backBufferIndex);

    m_commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    m_renderGraph->Execute(m_commandList.get(), m_srvHeap.get());

    m_renderGraph->EndFrame();

    m_commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    m_commandList->Close();

    ID3D12CommandList* commandLists[] = { m_commandList->GetD3D12CommandList() };
    m_commandQueue->ExecuteCommandLists(commandLists, 1);
}

void Application::Render()
{
    RenderScene();

    m_swapChain->Present(Config::Get().Display().vsyncEnabled);
    m_commandQueue->Flush();
}

void Application::LoadConfig()
{
    // Get executable directory for settings file
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
        exeDir = exeDir.substr(0, lastSlash + 1);
    exeDir += L"settings.json";

    // Convert wide string to narrow string
    int size = WideCharToMultiByte(CP_UTF8, 0, exeDir.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string configPath(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, exeDir.c_str(), -1, &configPath[0], size, nullptr, nullptr);

    // Load config - Config singleton is now the single source of truth
    Config::Get().Load(configPath);
}

void Application::SaveConfig()
{
    // Config is already updated via callbacks when settings change
    // Just save whatever is currently in Config (single source of truth)
    Config::Get().Save();
}

void Application::ApplyConfigSettings()
{
    auto& cfg = Config::Get();

    // Apply settings to systems
    if (m_postProcess)
    {
        m_postProcess->SetBloomEnabled(cfg.Graphics().bloomEnabled);
        m_postProcess->SetBloomIntensity(cfg.Graphics().bloomIntensity);
        m_postProcess->SetBloomThreshold(cfg.Graphics().bloomThreshold);
        m_postProcess->SetToneMapMode(static_cast<ToneMapMode>(cfg.Graphics().toneMappingMode));
        m_postProcess->SetExposure(cfg.Graphics().exposure);
        m_postProcess->SetGamma(cfg.Graphics().gamma);
    }
    if (m_postProcessPass)
    {
        m_postProcessPass->SetEnabled(cfg.Graphics().postProcessEnabled);
    }
    if (m_ssao)
    {
        m_ssao->SetRadius(cfg.Graphics().ssaoRadius);
        m_ssao->SetIntensity(cfg.Graphics().ssaoIntensity);
    }
    if (m_ssaoPass)
    {
        m_ssaoPass->SetEnabled(cfg.Graphics().ssaoEnabled);
    }
    if (m_debugPass)
    {
        m_debugPass->SetEnabled(cfg.Display().debugRenderingEnabled);
    }
}

void Application::ApplyInitialDisplaySettings()
{
    // Apply display settings from config BEFORE window/swapchain creation
    // This sets the initial window dimensions and display mode to avoid
    // triggering a resize after swapchain is created
    auto& cfg = Config::Get();
    auto resolutions = Window::GetAvailableResolutions();
    int resIdx = cfg.Display().resolutionIndex;

    if (resIdx >= 0 && resIdx < static_cast<int>(resolutions.size()))
    {
        m_window.SetInitialDimensions(resolutions[resIdx].width, resolutions[resIdx].height);
    }

    m_window.SetInitialDisplayMode(static_cast<WindowDisplayMode>(cfg.Display().displayMode));
}

void Application::Shutdown()
{
    // Save settings before shutdown
    SaveConfig();

    if (m_commandQueue)
    {
        m_commandQueue->Flush();
    }

    m_uiManager.reset();
    m_d2dInterop.reset();

    m_renderGraph.reset();
    m_shadowPass = nullptr;
    m_mainPass = nullptr;
    m_skyboxPass = nullptr;
    m_debugPass = nullptr;
    m_gBufferPass = nullptr;
    m_ssaoPass = nullptr;
    m_postProcessPass = nullptr;

    m_postProcess.reset();
    m_ssao.reset();
    m_gBuffer.reset();
    m_rtvHeap.reset();
    m_ibl.reset();
    m_skybox.reset();
    m_skyCubemap.reset();
    m_debugRenderer.reset();
    m_sceneRenderer.reset();
    m_scene.reset();
    m_shadowConstantBuffer.reset();
    m_shadowMap.reset();
    m_pipelineState.reset();
    m_rootSignature.reset();
    m_pixelShader.reset();
    m_vertexShader.reset();
    m_groundMaterial.reset();
    m_material.reset();
    m_normalTexture.reset();
    m_albedoTexture.reset();
    m_groundMesh.reset();
    m_mesh.reset();
    m_srvHeap.reset();
    m_camera.reset();
    m_commandList.reset();
    m_swapChain.reset();
    m_commandQueue.reset();
    m_device.reset();
    m_window.Shutdown();
}
