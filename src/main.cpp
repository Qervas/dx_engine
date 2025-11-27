#include "RHI/Device.h"
#include "RHI/CommandQueue.h"
#include "RHI/CommandList.h"
#include "RHI/SwapChain.h"
#include "RHI/Buffer.h"
#include "RHI/Texture.h"
#include "RHI/Shader.h"
#include "RHI/RootSignature.h"
#include "RHI/PipelineState.h"
#include "RHI/DescriptorHeap.h"
#include "Renderer/Camera.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/Light.h"
#include "Renderer/ProceduralTexture.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/ShadowMap.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Scene/MeshRenderer.h"
#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Core/Timer.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/ShadowPass.h"
#include "RenderGraph/MainPass.h"
#include "RenderGraph/DebugPass.h"
#include "RenderGraph/SkyboxPass.h"
#include "Renderer/DebugRenderer.h"
#include "Renderer/Skybox.h"
#include "Renderer/CubemapLoader.h"
#include "Renderer/IBL.h"
#include "RHI/CubemapTexture.h"
#include "UI/ImGuiRenderer.h"
#include <imgui.h>
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

// Application constants
constexpr uint32_t WINDOW_WIDTH = 1280;
constexpr uint32_t WINDOW_HEIGHT = 720;
constexpr float CAMERA_FOV = 60.0f;
constexpr float CAMERA_NEAR = 0.1f;
constexpr float CAMERA_FAR = 100.0f;

class Application
{
public:
    Application()
        : m_window(L"DirectX 12 Engine - PBR with Normal Mapping", WINDOW_WIDTH, WINDOW_HEIGHT)
    {
    }

    ~Application()
    {
        Shutdown();
    }

    bool Initialize()
    {
        // Initialize window
        if (!m_window.Initialize())
        {
            return false;
        }

        // Initialize graphics device
        m_device = std::make_unique<GraphicsDevice>();
        if (!m_device->Initialize())
        {
            return false;
        }

        // Create command queue
        m_commandQueue = std::make_unique<CommandQueue>(m_device.get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
        if (!m_commandQueue->Initialize())
        {
            return false;
        }

        // Create swap chain
        m_swapChain = std::make_unique<SwapChain>(m_device.get(), m_commandQueue.get(), &m_window);
        if (!m_swapChain->Initialize())
        {
            return false;
        }

        // Create command list
        m_commandList = std::make_unique<CommandList>(m_device.get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
        if (!m_commandList->Initialize())
        {
            return false;
        }

        // Initialize rendering resources
        if (!InitializeRenderingResources())
        {
            return false;
        }

        // Initialize camera - positioned to see ground and shadows
        m_camera = std::make_unique<Camera>();
        m_camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -10.0f));
        m_camera->SetPerspective(CAMERA_FOV, (float)m_window.GetWidth() / (float)m_window.GetHeight(), CAMERA_NEAR, CAMERA_FAR);

        // Initialize scene lights
        InitializeLights();

        // Initialize scene system
        if (!InitializeScene())
        {
            return false;
        }

        // Initialize debug renderer
        m_debugRenderer = std::make_unique<DebugRenderer>(m_device.get());
        if (!m_debugRenderer->Initialize())
        {
            return false;
        }
        m_debugRenderer->SetCamera(m_camera.get());

        // Initialize ImGui
        m_imguiRenderer = std::make_unique<ImGuiRenderer>(m_device.get());
        if (!m_imguiRenderer->Initialize(&m_window, m_srvHeap.get(), 2))
        {
            return false;
        }

        // Initialize skybox
        if (!InitializeSkybox())
        {
            return false;
        }

        // Initialize IBL (must be after skybox since it uses the environment cubemap)
        if (!InitializeIBL())
        {
            return false;
        }

        // Set environment map and IBL for scene reflections (after skybox and IBL are created)
        m_sceneRenderer->SetEnvironmentMap(m_skyCubemap.get());
        m_sceneRenderer->SetIBL(m_ibl.get());

        // Initialize render graph
        if (!InitializeRenderGraph())
        {
            return false;
        }

        return true;
    }

    bool InitializeRenderGraph()
    {
        // Create render graph
        m_renderGraph = std::make_unique<RenderGraph>(m_device.get());

        // Create shadow pass
        m_shadowPass = m_renderGraph->AddPass<ShadowPass>(m_shadowMap.get(), m_sceneRenderer.get());

        // Create main pass
        m_mainPass = m_renderGraph->AddPass<MainPass>(m_sceneRenderer.get(), m_swapChain.get());
        m_mainPass->SetShadowResources(m_shadowMap.get(), m_shadowConstantBuffer.get());

        // Create skybox pass (renders after main scene to use depth buffer for occlusion)
        m_skyboxPass = m_renderGraph->AddPass<SkyboxPass>(m_skybox.get(), m_swapChain.get(), m_camera.get());

        // Create debug pass (renders after skybox)
        m_debugPass = m_renderGraph->AddPass<DebugPass>(m_debugRenderer.get());

        // Compile the graph
        m_renderGraph->Compile();

        return true;
    }

    bool InitializeScene()
    {
        // Create the scene
        m_scene = std::make_unique<Scene>();

        // Create scene renderer
        m_sceneRenderer = std::make_unique<SceneRenderer>(m_device.get());
        if (!m_sceneRenderer->Initialize())
        {
            return false;
        }

        m_sceneRenderer->SetScene(m_scene.get());
        m_sceneRenderer->SetCamera(m_camera.get());
        m_sceneRenderer->SetLights(m_lights);
        m_sceneRenderer->SetPipeline(m_rootSignature.get(), m_pipelineState.get());
        m_sceneRenderer->SetShadowMap(m_shadowMap.get());
        m_sceneRenderer->SetShadowConstantBuffer(m_shadowConstantBuffer.get());
        // Environment map set after InitializeSkybox

        // Create ground plane
        Entity groundEntity = m_scene->CreateEntity("Ground");
        Transform* groundTransform = m_scene->AddComponent<Transform>(groundEntity);
        groundTransform->SetLocalPosition(0.0f, -2.0f, 0.0f);  // Below the cubes

        MeshRenderer* groundRenderer = m_scene->AddComponent<MeshRenderer>(groundEntity);
        groundRenderer->mesh = m_groundMesh.get();
        groundRenderer->material = m_groundMaterial.get();

        // Create a grid of cubes above the ground
        const int gridSize = 3;  // Reduced for clearer shadows
        const float spacing = 3.0f;
        const float offset = (gridSize - 1) * spacing * 0.5f;

        for (int x = 0; x < gridSize; ++x)
        {
            for (int z = 0; z < gridSize; ++z)
            {
                Entity cubeEntity = m_scene->CreateEntity("Cube");

                // Add Transform component - cubes floating above ground
                Transform* transform = m_scene->AddComponent<Transform>(cubeEntity);
                transform->SetLocalPosition(
                    x * spacing - offset,
                    1.0f + (x + z) * 0.3f,  // Varying heights for interesting shadows
                    z * spacing - offset
                );

                // Add MeshRenderer component
                MeshRenderer* meshRenderer = m_scene->AddComponent<MeshRenderer>(cubeEntity);
                meshRenderer->mesh = m_mesh.get();
                meshRenderer->material = m_material.get();

                // Store center cube for reference
                if (x == gridSize / 2 && z == gridSize / 2)
                {
                    m_cubeEntity = cubeEntity;
                }
            }
        }

        return true;
    }

    bool InitializeRenderingResources()
    {
        // Create descriptor heap for SRVs
        m_srvHeap = std::make_unique<DescriptorHeap>(m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);
        if (!m_srvHeap->Initialize())
        {
            return false;
        }

        // Create cube mesh with tangents for normal mapping
        m_mesh = std::unique_ptr<Mesh>(Mesh::CreateCubeWithTangents(m_device.get()));
        if (!m_mesh)
        {
            return false;
        }

        // Create ground plane mesh (large, tiled UVs)
        m_groundMesh = std::unique_ptr<Mesh>(Mesh::CreatePlaneWithTangents(m_device.get(), 30.0f, 10.0f));
        if (!m_groundMesh)
        {
            return false;
        }

        // Create albedo (checkerboard) texture
        const uint32_t texWidth = 256;
        const uint32_t texHeight = 256;
        auto checkerboardData = ProceduralTexture::GenerateCheckerboard(texWidth, texHeight, 32);

        m_albedoTexture = std::make_unique<Texture>(m_device.get());
        if (!m_albedoTexture->Create(texWidth, texHeight, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource))
        {
            return false;
        }

        // Create normal map (brick pattern)
        auto normalMapData = ProceduralTexture::GenerateBrickNormalMap(texWidth, texHeight);

        m_normalTexture = std::make_unique<Texture>(m_device.get());
        if (!m_normalTexture->Create(texWidth, texHeight, DXGI_FORMAT_R8G8B8A8_UNORM, TextureUsage::ShaderResource))
        {
            return false;
        }

        // Upload both textures
        // IMPORTANT: Upload buffers must stay alive until GPU finishes copying!
        m_commandList->Reset();

        // Create upload buffers OUTSIDE the command recording so they live until Flush()
        Buffer albedoUploadBuffer(m_device.get());
        if (!albedoUploadBuffer.Create(checkerboardData.size(), 0, BufferUsage::Upload, checkerboardData.data()))
        {
            return false;
        }

        Buffer normalUploadBuffer(m_device.get());
        if (!normalUploadBuffer.Create(normalMapData.size(), 0, BufferUsage::Upload, normalMapData.data()))
        {
            return false;
        }

        // Upload albedo texture
        m_commandList->TransitionBarrier(
            m_albedoTexture->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST
        );

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

        m_commandList->TransitionBarrier(
            m_albedoTexture->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        // Upload normal map
        m_commandList->TransitionBarrier(
            m_normalTexture->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST
        );

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

        m_commandList->TransitionBarrier(
            m_normalTexture->GetD3D12Resource(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        m_commandList->Close();

        ID3D12CommandList* commandLists[] = { m_commandList->GetD3D12CommandList() };
        m_commandQueue->ExecuteCommandLists(commandLists, 1);
        m_commandQueue->Flush();
        // Upload buffers are destroyed HERE, AFTER Flush() ensures GPU is done

        // Create SRV for albedo texture
        DescriptorHandle albedoSrvHandle = m_srvHeap->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        m_device->GetD3D12Device()->CreateShaderResourceView(
            m_albedoTexture->GetD3D12Resource(),
            &srvDesc,
            albedoSrvHandle.cpu
        );

        m_albedoTexture->SetSRV(albedoSrvHandle);

        // Create SRV for normal map
        DescriptorHandle normalSrvHandle = m_srvHeap->Allocate();
        m_device->GetD3D12Device()->CreateShaderResourceView(
            m_normalTexture->GetD3D12Resource(),
            &srvDesc,
            normalSrvHandle.cpu
        );

        m_normalTexture->SetSRV(normalSrvHandle);

        // Create material for cubes (more metallic to show reflections)
        m_material = std::make_unique<Material>(m_device.get());
        m_material->SetAlbedo(XMFLOAT3(0.9f, 0.9f, 0.95f));
        m_material->SetMetallic(0.8f);   // More metallic for visible reflections
        m_material->SetRoughness(0.2f);  // Smoother for clearer reflections
        m_material->SetAO(1.0f);
        m_material->SetAlbedoTexture(m_albedoTexture.get(), albedoSrvHandle);
        m_material->SetNormalTexture(m_normalTexture.get(), normalSrvHandle);

        // Create ground material (gray floor)
        m_groundMaterial = std::make_unique<Material>(m_device.get());
        m_groundMaterial->SetAlbedo(XMFLOAT3(0.5f, 0.5f, 0.5f));
        m_groundMaterial->SetMetallic(0.0f);
        m_groundMaterial->SetRoughness(0.9f);
        m_groundMaterial->SetAO(1.0f);
        m_groundMaterial->SetAlbedoTexture(m_albedoTexture.get(), albedoSrvHandle);
        m_groundMaterial->SetNormalTexture(m_normalTexture.get(), normalSrvHandle);

        // Create shadow map
        m_shadowMap = std::make_unique<ShadowMap>(m_device.get());
        if (!m_shadowMap->Initialize(2048, 2048, m_srvHeap.get()))
        {
            return false;
        }

        // Create shadow constant buffer for main pass
        m_shadowConstantBuffer = std::make_unique<Buffer>(m_device.get());
        if (!m_shadowConstantBuffer->Create(sizeof(ShadowConstants), 0, BufferUsage::Constant))
        {
            return false;
        }

        // Compile PBR shaders with shadows and IBL
        m_vertexShader = std::make_unique<Shader>();
        if (!m_vertexShader->CompileFromFile(L"shaders/PBRShadowVS.hlsl", "main", "vs_5_1"))
        {
            return false;
        }

        m_pixelShader = std::make_unique<Shader>();
        if (!m_pixelShader->CompileFromFile(L"shaders/PBRIblPS.hlsl", "main", "ps_5_1"))
        {
            return false;
        }

        // Create PBR root signature with IBL
        m_rootSignature = std::make_unique<RootSignature>(m_device.get());
        if (!m_rootSignature->CreateForPBRWithIBL())
        {
            return false;
        }

        // Define input layout with tangents
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

        // Create pipeline state with depth testing enabled
        m_pipelineState = std::make_unique<PipelineState>(m_device.get());
        if (!m_pipelineState->CreateGraphics(
            m_rootSignature.get(),
            m_vertexShader.get(),
            m_pixelShader.get(),
            inputLayout,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            m_swapChain->GetDepthFormat()))  // Enable depth testing
        {
            return false;
        }

        return true;
    }

    void InitializeLights()
    {
        m_lights.resize(2);

        // Light 1: Directional sun light (casts shadows)
        m_lights[0].SetType(LightType::Directional);
        m_lights[0].SetDirection(XMFLOAT3(0.5f, -1.0f, 0.5f));  // Sun angle
        m_lights[0].SetColor(XMFLOAT3(1.0f, 0.95f, 0.8f));      // Warm sunlight
        m_lights[0].SetIntensity(1.5f);

        // Light 2: Fill light (point light, no shadows)
        m_lights[1].SetType(LightType::Point);
        m_lights[1].SetPosition(XMFLOAT3(-3.0f, 2.0f, -3.0f));
        m_lights[1].SetColor(XMFLOAT3(0.4f, 0.5f, 0.7f));  // Cool fill
        m_lights[1].SetIntensity(0.8f);
        m_lights[1].SetRange(20.0f);
    }

    bool InitializeSkybox()
    {
        // Generate a procedural gradient sky cubemap
        m_skyCubemap = std::unique_ptr<CubemapTexture>(
            CubemapLoader::GenerateGradientSky(
                m_device.get(),
                m_commandList.get(),
                m_commandQueue.get(),
                m_srvHeap.get(),
                512  // Cubemap face size
            )
        );

        if (!m_skyCubemap)
        {
            return false;
        }

        // Create skybox
        m_skybox = std::make_unique<Skybox>(m_device.get());
        if (!m_skybox->Initialize(
            m_skyCubemap.get(),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            m_swapChain->GetDepthFormat()))
        {
            return false;
        }

        return true;
    }

    bool InitializeIBL()
    {
        // Create IBL resource manager
        m_ibl = std::make_unique<IBL>(m_device.get());

        // Generate all IBL textures from the environment cubemap
        if (!m_ibl->Generate(
            m_skyCubemap.get(),
            m_commandList.get(),
            m_commandQueue.get(),
            m_srvHeap.get()))
        {
            return false;
        }

        return true;
    }

    void OnResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;

        // Wait for GPU to finish all pending work
        m_commandQueue->Flush();

        // Resize swap chain buffers
        m_swapChain->Resize(width, height);

        // Update camera aspect ratio
        float aspectRatio = (float)width / (float)height;
        m_camera->SetPerspective(CAMERA_FOV, aspectRatio, CAMERA_NEAR, CAMERA_FAR);
    }

    void Run()
    {
        m_timer.Reset();

        while (m_window.ProcessMessages())
        {
            // Handle window resize
            if (m_window.WasResized())
            {
                OnResize(m_window.GetWidth(), m_window.GetHeight());
                m_window.ClearResizeFlag();
            }

            m_timer.Tick();
            Input::Get().Update();

            Update();
            Render();
        }

        // Wait for GPU to finish before cleanup
        m_commandQueue->Flush();
    }

    void Update()
    {
        float deltaTime = m_timer.GetDeltaTime();

        // Begin ImGui frame
        m_imguiRenderer->BeginFrame();

        // Show ImGui demo window for testing
        ImGui::ShowDemoWindow();

        // Simple stats window
        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", m_timer.GetFPS());
        ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
        ImGui::End();

        // Process camera FPS controls (only if ImGui doesn't want input)
        if (!m_imguiRenderer->WantCaptureMouse())
        {
            m_camera->ProcessFPSInput(deltaTime, 5.0f, 0.003f);
        }

        // Scene is static - no rotation

        // Update scene (propagates transform hierarchy)
        m_sceneRenderer->Update(deltaTime);

        // === Debug Rendering ===
        // Clear previous frame's debug primitives
        m_debugRenderer->Clear();

        // Draw world axes at origin
        m_debugRenderer->DrawAxes(XMFLOAT3(0.0f, 0.0f, 0.0f), 2.0f);

        // Draw a grid on the ground plane
        m_debugRenderer->DrawGrid(20.0f, 1.0f, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));

        // Draw bounding boxes around cubes (skip ground at y=-2)
        const auto& entities = m_scene->GetEntitiesWithComponent<Transform>();
        for (Entity entity : entities)
        {
            Transform* transform = m_scene->GetComponent<Transform>(entity);
            if (transform)
            {
                XMFLOAT3 pos = transform->GetWorldPosition();
                // Skip ground (positioned at y=-2)
                if (pos.y < -1.0f)
                    continue;

                m_debugRenderer->DrawWireBox(
                    XMFLOAT3(pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f),
                    XMFLOAT3(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f),
                    XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
                );
            }
        }

        // Draw light direction indicator
        XMFLOAT3 lightDir = m_lights[0].GetDirection();
        m_debugRenderer->DrawArrow(
            XMFLOAT3(0.0f, 5.0f, 0.0f),
            XMFLOAT3(-lightDir.x * 3.0f, 5.0f - lightDir.y * 3.0f, -lightDir.z * 3.0f),
            XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f),
            0.2f
        );

        // Update window title with FPS
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

    void Render()
    {
        uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        ID3D12Resource* backBuffer = m_swapChain->GetBackBuffer(backBufferIndex);

        // Reset command list
        m_commandList->Reset();

        // Begin frame for render graph
        m_renderGraph->BeginFrame();

        // Update pass parameters for this frame
        XMFLOAT3 sceneCenter;
        float sceneRadius;
        m_sceneRenderer->GetSceneBounds(sceneCenter, sceneRadius);

        // Configure shadow pass
        m_shadowPass->SetLightInfo(&m_lights[0], sceneCenter, sceneRadius);

        // Configure main pass
        m_mainPass->SetBackBufferIndex(backBufferIndex);

        // Configure skybox pass
        m_skyboxPass->SetBackBufferIndex(backBufferIndex);

        // Transition back buffer to render target state
        m_commandList->TransitionBarrier(
            backBuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        // Execute render graph (shadow pass + main pass)
        m_renderGraph->Execute(m_commandList.get(), m_srvHeap.get());

        // End frame for render graph
        m_renderGraph->EndFrame();

        // Render ImGui (after scene, before present)
        m_imguiRenderer->EndFrame(m_commandList.get());

        // Transition back buffer to present state
        m_commandList->TransitionBarrier(
            backBuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        // Close command list
        m_commandList->Close();

        // Execute command list
        ID3D12CommandList* commandLists[] = { m_commandList->GetD3D12CommandList() };
        m_commandQueue->ExecuteCommandLists(commandLists, 1);

        // Present
        m_swapChain->Present(true);

        // Wait for this frame to complete
        m_commandQueue->Flush();
    }

    void Shutdown()
    {
        // Shutdown in reverse order of initialization
        if (m_commandQueue)
        {
            m_commandQueue->Flush();
        }

        // Render graph (owns passes, must be destroyed first)
        m_renderGraph.reset();
        m_shadowPass = nullptr;
        m_mainPass = nullptr;
        m_skyboxPass = nullptr;
        m_debugPass = nullptr;

        // Skybox and IBL
        m_ibl.reset();
        m_skybox.reset();
        m_skyCubemap.reset();

        // Debug renderer
        m_debugRenderer.reset();

        // ImGui
        m_imguiRenderer.reset();

        // Scene resources
        m_sceneRenderer.reset();
        m_scene.reset();

        // Shadow mapping resources
        m_shadowConstantBuffer.reset();
        m_shadowMap.reset();

        // Rendering resources
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

private:
    Window m_window;
    Timer m_timer;
    std::unique_ptr<GraphicsDevice> m_device;
    std::unique_ptr<CommandQueue> m_commandQueue;
    std::unique_ptr<SwapChain> m_swapChain;
    std::unique_ptr<CommandList> m_commandList;

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
    ShadowPass* m_shadowPass = nullptr;  // Owned by render graph
    MainPass* m_mainPass = nullptr;      // Owned by render graph
    SkyboxPass* m_skyboxPass = nullptr;  // Owned by render graph
    DebugPass* m_debugPass = nullptr;    // Owned by render graph

    // Skybox
    std::unique_ptr<CubemapTexture> m_skyCubemap;
    std::unique_ptr<Skybox> m_skybox;

    // Image-Based Lighting
    std::unique_ptr<IBL> m_ibl;

    // Debug renderer
    std::unique_ptr<DebugRenderer> m_debugRenderer;

    // ImGui
    std::unique_ptr<ImGuiRenderer> m_imguiRenderer;

    // Scene lights
    std::vector<Light> m_lights;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Application app;

    if (!app.Initialize())
    {
        MessageBox(nullptr, L"Failed to initialize application", L"Error", MB_OK);
        return 1;
    }

    app.Run();

    return 0;
}
