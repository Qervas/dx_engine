#include "RHI/Device.h"
#include "RHI/CommandQueue.h"
#include "RHI/CommandList.h"
#include "RHI/SwapChain.h"
#include "RHI/Buffer.h"
#include "RHI/Shader.h"
#include "RHI/RootSignature.h"
#include "RHI/PipelineState.h"
#include "Platform/Window.h"
#include <memory>

// Application constants
constexpr uint32_t WINDOW_WIDTH = 1280;
constexpr uint32_t WINDOW_HEIGHT = 720;

// Vertex structure
struct Vertex
{
    float position[3];
    float color[3];
};

class Application
{
public:
    Application()
        : m_window(L"DirectX 12 Engine - Triangle", WINDOW_WIDTH, WINDOW_HEIGHT)
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

        return true;
    }

    bool InitializeRenderingResources()
    {
        // Create vertex buffer
        Vertex vertices[] =
        {
            { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },  // Top (red)
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },  // Right (green)
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }   // Left (blue)
        };

        m_vertexBuffer = std::make_unique<Buffer>(m_device.get());
        if (!m_vertexBuffer->Create(sizeof(vertices), sizeof(Vertex), BufferUsage::Upload, vertices))
        {
            return false;
        }

        // Compile shaders
        m_vertexShader = std::make_unique<Shader>();
        if (!m_vertexShader->CompileFromFile(L"shaders/BasicVS.hlsl", "main", "vs_5_1"))
        {
            return false;
        }

        m_pixelShader = std::make_unique<Shader>();
        if (!m_pixelShader->CompileFromFile(L"shaders/BasicPS.hlsl", "main", "ps_5_1"))
        {
            return false;
        }

        // Create root signature
        m_rootSignature = std::make_unique<RootSignature>(m_device.get());
        if (!m_rootSignature->CreateEmpty())
        {
            return false;
        }

        // Define input layout
        D3D12_INPUT_ELEMENT_DESC inputElements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

    void Run()
    {
        while (m_window.ProcessMessages())
        {
            Render();
        }

        // Wait for GPU to finish before cleanup
        m_commandQueue->Flush();
    }

    void Shutdown()
    {
        // Shutdown in reverse order of initialization
        if (m_commandQueue)
        {
            m_commandQueue->Flush();
        }

        m_pipelineState.reset();
        m_rootSignature.reset();
        m_pixelShader.reset();
        m_vertexShader.reset();
        m_vertexBuffer.reset();
        m_commandList.reset();
        m_swapChain.reset();
        m_commandQueue.reset();
        m_device.reset();
        m_window.Shutdown();
    }

private:
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

        // Clear render target to dark blue
        const float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
        m_commandList->ClearRenderTargetView(rtv, clearColor);

        // Set pipeline state and draw triangle
        m_commandList->SetPipelineState(m_pipelineState->GetD3D12PipelineState());
        m_commandList->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature());
        m_commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VERTEX_BUFFER_VIEW vbv = m_vertexBuffer->GetVertexBufferView();
        m_commandList->SetVertexBuffers(0, 1, &vbv);
        m_commandList->Draw(3, 0);

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
        m_swapChain->Present(true);  // vsync enabled

        // Wait for this frame to complete
        m_commandQueue->Flush();
    }

    Window m_window;
    std::unique_ptr<GraphicsDevice> m_device;
    std::unique_ptr<CommandQueue> m_commandQueue;
    std::unique_ptr<SwapChain> m_swapChain;
    std::unique_ptr<CommandList> m_commandList;

    // Rendering resources
    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Shader> m_vertexShader;
    std::unique_ptr<Shader> m_pixelShader;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;
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
