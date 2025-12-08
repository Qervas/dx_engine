#pragma once

#include "../Core/Types.h"
#include <d3d12.h>
#include <dxgi1_6.h>

class GraphicsDevice;
class CommandQueue;
class Window;

class SwapChain
{
public:
    SwapChain(GraphicsDevice* device, CommandQueue* commandQueue, Window* window);
    ~SwapChain();

    bool Initialize();
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);

    void Present(bool vsync = true);
    uint32_t GetCurrentBackBufferIndex() const;

    // Accessors
    IDXGISwapChain3* GetDXGISwapChain() const { return m_swapChain.Get(); }
    ID3D12Resource* GetBackBuffer(uint32_t index) const { return m_renderTargets[index].Get(); }
    ID3D12Resource* GetDepthStencilBuffer() const { return m_depthStencilBuffer.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(uint32_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetBackBufferCount() const { return FRAME_COUNT; }
    DXGI_FORMAT GetDepthFormat() const { return DXGI_FORMAT_D32_FLOAT; }

private:
    GraphicsDevice* m_device;
    CommandQueue* m_commandQueue;
    Window* m_window;

    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[FRAME_COUNT];
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;

    // Depth stencil resources
    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_rtvDescriptorSize;
    uint32_t m_frameIndex = 0;
};
