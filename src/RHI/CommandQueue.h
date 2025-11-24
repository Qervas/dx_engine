#pragma once

#include "../Core/Types.h"
#include <d3d12.h>

class GraphicsDevice;

class CommandQueue
{
public:
    CommandQueue(GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandQueue();

    bool Initialize();
    void Shutdown();

    // Command list execution
    void ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count);

    // Synchronization
    uint64_t Signal();
    void WaitForFenceValue(uint64_t fenceValue);
    bool IsFenceComplete(uint64_t fenceValue);
    void Flush();  // Wait for all work to complete

    // Accessors
    ID3D12CommandQueue* GetD3D12CommandQueue() const { return m_commandQueue.Get(); }
    D3D12_COMMAND_LIST_TYPE GetType() const { return m_type; }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent = nullptr;
    uint64_t m_fenceValue = 0;
    D3D12_COMMAND_LIST_TYPE m_type;
};
