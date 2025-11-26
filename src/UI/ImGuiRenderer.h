#pragma once

#include "../RHI/Device.h"
#include "../RHI/DescriptorHeap.h"
#include "../RHI/CommandList.h"
#include <d3d12.h>

// Forward declarations
class Window;

class ImGuiRenderer
{
public:
    ImGuiRenderer(GraphicsDevice* device);
    ~ImGuiRenderer();

    // Initialize ImGui with D3D12 backend
    bool Initialize(Window* window, DescriptorHeap* srvHeap, int numFramesInFlight = 2);
    void Shutdown();

    // Frame handling
    void BeginFrame();
    void EndFrame(CommandList* commandList);

    // Check if ImGui wants to capture input
    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

private:
    GraphicsDevice* m_device = nullptr;
    DescriptorHeap* m_srvHeap = nullptr;
    DescriptorHandle m_fontSrvHandle;
    bool m_initialized = false;
};
