#pragma once

#include "../Core/Types.h"
#include <d3d12.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <string>

class GraphicsDevice;
class CommandQueue;
class SwapChain;

class D2DInterop
{
public:
    D2DInterop(GraphicsDevice* device, CommandQueue* commandQueue);
    ~D2DInterop();

    bool Initialize();
    void Shutdown();

    // Create wrapped render targets for each swap chain back buffer
    bool CreateWrappedRenderTargets(SwapChain* swapChain);
    void ReleaseWrappedRenderTargets();

    // Full recreation for resize (destroys and recreates D3D11On12 resources)
    void PrepareForResize();
    bool RecreateAfterResize(SwapChain* swapChain);

    // Begin/End frame for D2D rendering
    void BeginD2DDraw(uint32_t backBufferIndex);
    void EndD2DDraw();

    // Clear the render target
    void Clear(float r, float g, float b, float a = 1.0f);

    // Accessors
    ID2D1DeviceContext* GetD2DContext() const { return m_d2dContext.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }
    ID2D1SolidColorBrush* GetBrush() const { return m_brush.Get(); }

    // Set brush color
    void SetBrushColor(float r, float g, float b, float a = 1.0f);

    // Draw text helper
    void DrawText(const std::wstring& text, IDWriteTextFormat* format,
                  float x, float y, float width, float height);

    // Draw filled rectangle
    void FillRect(float x, float y, float width, float height);

    // Draw rounded rectangle
    void FillRoundedRect(float x, float y, float width, float height, float radius);

    // Draw rectangle outline
    void DrawRect(float x, float y, float width, float height, float strokeWidth = 1.0f);

    // Draw rounded rectangle outline
    void DrawRoundedRect(float x, float y, float width, float height, float radius, float strokeWidth = 1.0f);

    // Draw line
    void DrawLine(float x1, float y1, float x2, float y2, float strokeWidth = 1.0f);

    // Draw ellipse (filled)
    void FillEllipse(float centerX, float centerY, float radiusX, float radiusY);

    // Draw circle (filled) - convenience wrapper
    void FillCircle(float centerX, float centerY, float radius);

private:
    GraphicsDevice* m_device;
    CommandQueue* m_commandQueue;

    // D3D11on12 interop
    ComPtr<ID3D11On12Device> m_d3d11On12Device;
    ComPtr<ID3D11Device> m_d3d11Device;
    ComPtr<ID3D11DeviceContext> m_d3d11Context;

    // D2D resources
    ComPtr<ID2D1Factory3> m_d2dFactory;
    ComPtr<ID2D1Device2> m_d2dDevice;
    ComPtr<ID2D1DeviceContext> m_d2dContext;

    // DirectWrite resources
    ComPtr<IDWriteFactory> m_dwriteFactory;

    // Common brush
    ComPtr<ID2D1SolidColorBrush> m_brush;

    // Per-back-buffer resources (for swap chain)
    static constexpr uint32_t MAX_BACK_BUFFERS = 3;
    ComPtr<ID3D11Resource> m_wrappedBackBuffers[MAX_BACK_BUFFERS];
    ComPtr<ID2D1Bitmap1> m_d2dRenderTargets[MAX_BACK_BUFFERS];
    uint32_t m_backBufferCount = 0;
    uint32_t m_currentBackBuffer = 0;
};
