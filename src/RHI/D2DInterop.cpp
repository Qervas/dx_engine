#include "D2DInterop.h"
#include "Device.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include <windows.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")

D2DInterop::D2DInterop(GraphicsDevice* device, CommandQueue* commandQueue)
    : m_device(device)
    , m_commandQueue(commandQueue)
{
}

D2DInterop::~D2DInterop()
{
    Shutdown();
}

bool D2DInterop::Initialize()
{
    HRESULT hr;

    // 1. Create D3D11On12 device
    UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<ID3D11Device> d3d11Device;
    ID3D12CommandQueue* queues[] = { m_commandQueue->GetD3D12CommandQueue() };

    hr = D3D11On12CreateDevice(
        m_device->GetD3D12Device(),
        d3d11DeviceFlags,
        nullptr, 0,
        reinterpret_cast<IUnknown**>(queues), 1,
        0,
        &d3d11Device,
        &m_d3d11Context,
        nullptr
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create D3D11On12 device", L"Error", MB_OK);
        return false;
    }

    hr = d3d11Device.As(&m_d3d11On12Device);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to query ID3D11On12Device", L"Error", MB_OK);
        return false;
    }

    m_d3d11Device = d3d11Device;

    // 2. Create D2D factory
    D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory3),
        &options,
        reinterpret_cast<void**>(m_d2dFactory.GetAddressOf())
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create D2D factory", L"Error", MB_OK);
        return false;
    }

    // 3. Get DXGI device from D3D11 device
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_d3d11Device.As(&dxgiDevice);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to get DXGI device", L"Error", MB_OK);
        return false;
    }

    // 4. Create D2D device
    hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create D2D device", L"Error", MB_OK);
        return false;
    }

    // 5. Create D2D device context
    hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create D2D device context", L"Error", MB_OK);
        return false;
    }

    // 6. Create DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create DirectWrite factory", L"Error", MB_OK);
        return false;
    }

    // 7. Create default brush
    hr = m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create brush", L"Error", MB_OK);
        return false;
    }

    return true;
}

void D2DInterop::Shutdown()
{
    ReleaseWrappedRenderTargets();

    m_brush.Reset();
    m_dwriteFactory.Reset();
    m_d2dContext.Reset();
    m_d2dDevice.Reset();
    m_d2dFactory.Reset();
    m_d3d11Context.Reset();
    m_d3d11On12Device.Reset();
    m_d3d11Device.Reset();
}

bool D2DInterop::CreateWrappedRenderTargets(SwapChain* swapChain)
{
    HRESULT hr;

    m_backBufferCount = swapChain->GetBackBufferCount();

    for (uint32_t i = 0; i < m_backBufferCount; ++i)
    {
        ID3D12Resource* backBuffer = swapChain->GetBackBuffer(i);

        // Wrap the D3D12 resource for D3D11 use
        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };

        hr = m_d3d11On12Device->CreateWrappedResource(
            backBuffer,
            &d3d11Flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            IID_PPV_ARGS(&m_wrappedBackBuffers[i])
        );

        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to create wrapped resource", L"Error", MB_OK);
            return false;
        }

        // Get DXGI surface from wrapped resource
        ComPtr<IDXGISurface> dxgiSurface;
        hr = m_wrappedBackBuffers[i].As(&dxgiSurface);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to get DXGI surface", L"Error", MB_OK);
            return false;
        }

        // Create D2D bitmap from DXGI surface
        D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        hr = m_d2dContext->CreateBitmapFromDxgiSurface(
            dxgiSurface.Get(),
            &bitmapProps,
            &m_d2dRenderTargets[i]
        );

        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to create D2D bitmap", L"Error", MB_OK);
            return false;
        }
    }

    return true;
}

void D2DInterop::ReleaseWrappedRenderTargets()
{
    // Flush D3D11 context before releasing
    if (m_d3d11Context)
    {
        m_d3d11Context->Flush();
    }

    for (uint32_t i = 0; i < MAX_BACK_BUFFERS; ++i)
    {
        m_d2dRenderTargets[i].Reset();
        m_wrappedBackBuffers[i].Reset();
    }

    m_backBufferCount = 0;
}

void D2DInterop::BeginD2DDraw(uint32_t backBufferIndex)
{
    m_currentBackBuffer = backBufferIndex;

    // Acquire wrapped resource for D3D11 use
    ID3D11Resource* resources[] = { m_wrappedBackBuffers[backBufferIndex].Get() };
    m_d3d11On12Device->AcquireWrappedResources(resources, 1);

    // Set render target
    m_d2dContext->SetTarget(m_d2dRenderTargets[backBufferIndex].Get());

    m_d2dContext->BeginDraw();
}

void D2DInterop::EndD2DDraw()
{
    m_d2dContext->EndDraw();

    // Release wrapped resource back to D3D12
    ID3D11Resource* resources[] = { m_wrappedBackBuffers[m_currentBackBuffer].Get() };
    m_d3d11On12Device->ReleaseWrappedResources(resources, 1);

    // Flush D3D11 context to submit commands
    m_d3d11Context->Flush();
}

void D2DInterop::Clear(float r, float g, float b, float a)
{
    m_d2dContext->Clear(D2D1::ColorF(r, g, b, a));
}

void D2DInterop::SetBrushColor(float r, float g, float b, float a)
{
    m_brush->SetColor(D2D1::ColorF(r, g, b, a));
}

void D2DInterop::DrawText(const std::wstring& text, IDWriteTextFormat* format,
                          float x, float y, float width, float height)
{
    D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
    m_d2dContext->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        rect,
        m_brush.Get()
    );
}

void D2DInterop::FillRect(float x, float y, float width, float height)
{
    D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
    m_d2dContext->FillRectangle(rect, m_brush.Get());
}

void D2DInterop::FillRoundedRect(float x, float y, float width, float height, float radius)
{
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
        D2D1::RectF(x, y, x + width, y + height),
        radius,
        radius
    );
    m_d2dContext->FillRoundedRectangle(roundedRect, m_brush.Get());
}
