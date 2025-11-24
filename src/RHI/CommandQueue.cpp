#include "CommandQueue.h"
#include "Device.h"
#include <windows.h>

CommandQueue::CommandQueue(GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type)
    : m_device(device)
    , m_type(type)
{
}

CommandQueue::~CommandQueue()
{
    Shutdown();
}

bool CommandQueue::Initialize()
{
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = m_type;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    HRESULT hr = m_device->GetD3D12Device()->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(&m_commandQueue)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create command queue", L"Error", MB_OK);
        return false;
    }

    // Set name for debugging
#if defined(_DEBUG)
    switch (m_type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        m_commandQueue->SetName(L"Graphics Command Queue");
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        m_commandQueue->SetName(L"Compute Command Queue");
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        m_commandQueue->SetName(L"Copy Command Queue");
        break;
    }
#endif

    // Create fence
    hr = m_device->GetD3D12Device()->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&m_fence)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create fence", L"Error", MB_OK);
        return false;
    }

    // Create fence event
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        MessageBox(nullptr, L"Failed to create fence event", L"Error", MB_OK);
        return false;
    }

    m_fenceValue = 1;

    return true;
}

void CommandQueue::Shutdown()
{
    // Flush any remaining work
    if (m_commandQueue)
    {
        Flush();
    }

    // Close fence event
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    m_fence.Reset();
    m_commandQueue.Reset();
}

void CommandQueue::ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count)
{
    m_commandQueue->ExecuteCommandLists(count, commandLists);
}

uint64_t CommandQueue::Signal()
{
    uint64_t fenceValueToSignal = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fenceValueToSignal);
    m_fenceValue++;
    return fenceValueToSignal;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if (m_fence->GetCompletedValue() < fenceValue)
    {
        m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    return m_fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::Flush()
{
    uint64_t fenceValueToWaitFor = Signal();
    WaitForFenceValue(fenceValueToWaitFor);
}
