#include "ImGuiRenderer.h"
#include "../Platform/Window.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

ImGuiRenderer::ImGuiRenderer(GraphicsDevice* device)
    : m_device(device)
{
}

ImGuiRenderer::~ImGuiRenderer()
{
    Shutdown();
}

bool ImGuiRenderer::Initialize(Window* window, DescriptorHeap* srvHeap, int numFramesInFlight)
{
    if (m_initialized)
        return true;

    m_srvHeap = srvHeap;

    // Allocate descriptor for font texture
    m_fontSrvHandle = srvHeap->Allocate();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window->GetHandle());
    ImGui_ImplDX12_Init(
        m_device->GetD3D12Device(),
        numFramesInFlight,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        srvHeap->GetD3D12DescriptorHeap(),
        m_fontSrvHandle.cpu,
        m_fontSrvHandle.gpu
    );

    m_initialized = true;
    return true;
}

void ImGuiRenderer::Shutdown()
{
    if (!m_initialized)
        return;

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
}

void ImGuiRenderer::BeginFrame()
{
    if (!m_initialized)
        return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderer::EndFrame(CommandList* commandList)
{
    if (!m_initialized)
        return;

    // Render ImGui
    ImGui::Render();

    // Set descriptor heap before rendering
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap->GetD3D12DescriptorHeap() };
    commandList->GetD3D12CommandList()->SetDescriptorHeaps(1, heaps);

    // Render draw data
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList->GetD3D12CommandList());
}

bool ImGuiRenderer::WantCaptureMouse() const
{
    if (!m_initialized)
        return false;
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiRenderer::WantCaptureKeyboard() const
{
    if (!m_initialized)
        return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}
