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
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Scene/MeshRenderer.h"
#include "Platform/Window.h"
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

// Application constants
constexpr uint32_t WINDOW_WIDTH = 1280;
constexpr uint32_t WINDOW_HEIGHT = 720;

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

        // Initialize camera
        m_camera = std::make_unique<Camera>();
        m_camera->SetPosition(XMFLOAT3(0.0f, 0.0f, -3.0f));
        m_camera->SetPerspective(60.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

        // Initialize scene lights
        InitializeLights();

        // Initialize scene system
        if (!InitializeScene())
        {
            return false;
        }

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

        // Create a cube entity
        Entity cubeEntity = m_scene->CreateEntity("PBR Cube");

        // Add Transform component
        Transform* transform = m_scene->AddComponent<Transform>(cubeEntity);
        transform->SetLocalPosition(0.0f, 0.0f, 0.0f);

        // Add MeshRenderer component
        MeshRenderer* meshRenderer = m_scene->AddComponent<MeshRenderer>(cubeEntity);
        meshRenderer->mesh = m_mesh.get();
        meshRenderer->material = m_material.get();

        // Store the cube entity for animation
        m_cubeEntity = cubeEntity;

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

        // Create material
        m_material = std::make_unique<Material>(m_device.get());
        m_material->SetAlbedo(XMFLOAT3(1.0f, 1.0f, 1.0f));
        m_material->SetMetallic(0.3f);
        m_material->SetRoughness(0.7f);
        m_material->SetAO(1.0f);
        m_material->SetAlbedoTexture(m_albedoTexture.get(), albedoSrvHandle);
        m_material->SetNormalTexture(m_normalTexture.get(), normalSrvHandle);

        // Compile PBR shaders with normal mapping
        m_vertexShader = std::make_unique<Shader>();
        if (!m_vertexShader->CompileFromFile(L"shaders/PBRNormalVS.hlsl", "main", "vs_5_1"))
        {
            return false;
        }

        m_pixelShader = std::make_unique<Shader>();
        if (!m_pixelShader->CompileFromFile(L"shaders/PBRNormalPS.hlsl", "main", "ps_5_1"))
        {
            return false;
        }

        // Create PBR root signature with normal mapping
        m_rootSignature = std::make_unique<RootSignature>(m_device.get());
        if (!m_rootSignature->CreateForPBRWithNormalMap())
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

        // Create pipeline state
        m_pipelineState = std::make_unique<PipelineState>(m_device.get());
        if (!m_pipelineState->CreateGraphics(
            m_rootSignature.get(),
            m_vertexShader.get(),
            m_pixelShader.get(),
            inputLayout,
            DXGI_FORMAT_R8G8B8A8_UNORM))
        {
            return false;
        }

        return true;
    }

    void InitializeLights()
    {
        // Create 2 point lights
        m_lights.resize(2);

        // Light 1: White light on the right (reduced intensity)
        m_lights[0].SetType(LightType::Point);
        m_lights[0].SetPosition(XMFLOAT3(2.0f, 1.0f, -2.0f));
        m_lights[0].SetColor(XMFLOAT3(1.0f, 1.0f, 1.0f));
        m_lights[0].SetIntensity(2.0f);  // Reduced from 10.0
        m_lights[0].SetRange(10.0f);

        // Light 2: Blue light on the left (reduced intensity)
        m_lights[1].SetType(LightType::Point);
        m_lights[1].SetPosition(XMFLOAT3(-2.0f, 1.0f, -2.0f));
        m_lights[1].SetColor(XMFLOAT3(0.3f, 0.5f, 1.0f));
        m_lights[1].SetIntensity(1.5f);  // Reduced from 8.0
        m_lights[1].SetRange(10.0f);
    }

    void Run()
    {
        while (m_window.ProcessMessages())
        {
            Update();
            Render();
        }

        // Wait for GPU to finish before cleanup
        m_commandQueue->Flush();
    }

    void Update()
    {
        static float rotation = 0.0f;
        rotation += 0.01f;

        // Update cube entity transform
        Transform* transform = m_scene->GetComponent<Transform>(m_cubeEntity);
        if (transform)
        {
            transform->SetLocalRotationEuler(rotation * 0.5f, rotation, 0.0f);
        }

        // Update scene (propagates transform hierarchy)
        m_sceneRenderer->Update(0.016f);  // ~60fps delta time
    }

    void Render()
    {
        uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        ID3D12Resource* backBuffer = m_swapChain->GetBackBuffer(backBufferIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetRTV(backBufferIndex);

        // Reset command list
        m_commandList->Reset();

        // Transition back buffer to render target state
        m_commandList->TransitionBarrier(
            backBuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        // Set render target
        m_commandList->SetRenderTargets(1, &rtv, nullptr);

        // Set viewport and scissor
        m_commandList->SetViewport(0, 0,
            static_cast<float>(m_swapChain->GetWidth()),
            static_cast<float>(m_swapChain->GetHeight()));
        m_commandList->SetScissorRect(0, 0,
            m_swapChain->GetWidth(),
            m_swapChain->GetHeight());

        // Clear render target
        const float clearColor[] = { 0.02f, 0.02f, 0.02f, 1.0f };
        m_commandList->ClearRenderTargetView(rtv, clearColor);

        // Render scene using SceneRenderer
        m_sceneRenderer->Render(m_commandList.get(), m_srvHeap.get());

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

        // Scene resources
        m_sceneRenderer.reset();
        m_scene.reset();

        // Rendering resources
        m_pipelineState.reset();
        m_rootSignature.reset();
        m_pixelShader.reset();
        m_vertexShader.reset();
        m_material.reset();
        m_normalTexture.reset();
        m_albedoTexture.reset();
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
    std::unique_ptr<Texture> m_albedoTexture;
    std::unique_ptr<Texture> m_normalTexture;
    std::unique_ptr<Material> m_material;
    std::unique_ptr<Shader> m_vertexShader;
    std::unique_ptr<Shader> m_pixelShader;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;

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
