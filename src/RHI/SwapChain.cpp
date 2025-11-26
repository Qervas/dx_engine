#include "SwapChain.h"
#include "Device.h"
#include "CommandQueue.h"
#include "../Platform/Window.h"
#include <windows.h>

SwapChain::SwapChain(GraphicsDevice* device, CommandQueue* commandQueue, Window* window)
    : m_device(device)
    , m_commandQueue(commandQueue)
    , m_window(window)
{
    m_width = window->GetWidth();
    m_height = window->GetHeight();
}

SwapChain::~SwapChain()
{
    Shutdown();
}

bool SwapChain::Initialize()
{
    // Create swap chain description
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    // Create swap chain
    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = m_device->GetDXGIFactory()->CreateSwapChainForHwnd(
        m_commandQueue->GetD3D12CommandQueue(),
        m_window->GetHandle(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create swap chain", L"Error", MB_OK);
        return false;
    }

    // Query IDXGISwapChain3 interface
    hr = swapChain1.As(&m_swapChain);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to query SwapChain3 interface", L"Error", MB_OK);
        return false;
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FRAME_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->GetD3D12Device()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create RTV descriptor heap", L"Error", MB_OK);
        return false;
    }

    m_rtvDescriptorSize = m_device->GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create render target views
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < FRAME_COUNT; i++)
    {
        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to get swap chain buffer", L"Error", MB_OK);
            return false;
        }

        m_device->GetD3D12Device()->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;

#if defined(_DEBUG)
        wchar_t name[32];
        swprintf_s(name, L"RenderTarget[%u]", i);
        m_renderTargets[i]->SetName(name);
#endif
    }

    // Create depth stencil buffer
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = m_width;
    depthDesc.Height = m_height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClearValue = {};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    hr = m_device->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&m_depthStencilBuffer)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create depth stencil buffer", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_depthStencilBuffer->SetName(L"DepthStencilBuffer");
#endif

    // Create DSV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->GetD3D12Device()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create DSV descriptor heap", L"Error", MB_OK);
        return false;
    }

    // Create depth stencil view
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    m_device->GetD3D12Device()->CreateDepthStencilView(
        m_depthStencilBuffer.Get(),
        &dsvDesc,
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    return true;
}

void SwapChain::Shutdown()
{
    m_depthStencilBuffer.Reset();
    m_dsvHeap.Reset();
    for (auto& rt : m_renderTargets)
    {
        rt.Reset();
    }
    m_rtvHeap.Reset();
    m_swapChain.Reset();
}

void SwapChain::Present(bool vsync)
{
    m_swapChain->Present(vsync ? 1 : 0, 0);
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

uint32_t SwapChain::GetCurrentBackBufferIndex() const
{
    return m_frameIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetRTV(uint32_t index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += index * m_rtvDescriptorSize;
    return rtvHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetDSV() const
{
    return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}
