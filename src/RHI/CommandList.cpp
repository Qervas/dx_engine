#include "CommandList.h"
#include "Device.h"
#include <windows.h>

CommandList::CommandList(GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type)
    : m_device(device)
    , m_type(type)
{
}

CommandList::~CommandList()
{
    Shutdown();
}

bool CommandList::Initialize()
{
    // Create command allocator
    HRESULT hr = m_device->GetD3D12Device()->CreateCommandAllocator(
        m_type,
        IID_PPV_ARGS(&m_commandAllocator)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create command allocator", L"Error", MB_OK);
        return false;
    }

    // Create command list
    hr = m_device->GetD3D12Device()->CreateCommandList(
        0,
        m_type,
        m_commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create command list", L"Error", MB_OK);
        return false;
    }

    // Command lists are created in recording state, close it
    m_commandList->Close();

    return true;
}

void CommandList::Shutdown()
{
    m_commandList.Reset();
    m_commandAllocator.Reset();
}

void CommandList::Reset()
{
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);
}

void CommandList::Close()
{
    m_commandList->Close();
}

// Drawing
void CommandList::Draw(uint32_t vertexCount, uint32_t startVertex)
{
    m_commandList->DrawInstanced(vertexCount, 1, startVertex, 0);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex)
{
    m_commandList->DrawIndexedInstanced(indexCount, 1, startIndex, baseVertex, 0);
}

void CommandList::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
    m_commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    m_commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

// Compute
void CommandList::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    m_commandList->Dispatch(groupsX, groupsY, groupsZ);
}

// State setting
void CommandList::SetPipelineState(ID3D12PipelineState* pso)
{
    m_commandList->SetPipelineState(pso);
}

void CommandList::SetGraphicsRootSignature(ID3D12RootSignature* rootSig)
{
    m_commandList->SetGraphicsRootSignature(rootSig);
}

void CommandList::SetComputeRootSignature(ID3D12RootSignature* rootSig)
{
    m_commandList->SetComputeRootSignature(rootSig);
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
    m_commandList->IASetPrimitiveTopology(topology);
}

// Viewport and scissor
void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = minDepth;
    viewport.MaxDepth = maxDepth;
    m_commandList->RSSetViewports(1, &viewport);
}

void CommandList::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    D3D12_RECT scissorRect;
    scissorRect.left = left;
    scissorRect.top = top;
    scissorRect.right = right;
    scissorRect.bottom = bottom;
    m_commandList->RSSetScissorRects(1, &scissorRect);
}

// Resource barriers
void CommandList::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);
}

void CommandList::UAVBarrier(ID3D12Resource* resource)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource;
    m_commandList->ResourceBarrier(1, &barrier);
}

// Render targets
void CommandList::SetRenderTargets(uint32_t numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, const D3D12_CPU_DESCRIPTOR_HANDLE* dsv)
{
    m_commandList->OMSetRenderTargets(numRenderTargets, rtvs, FALSE, dsv);
}

void CommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* color)
{
    m_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
}

void CommandList::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth, uint8_t stencil)
{
    m_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

// Descriptor heaps
void CommandList::SetDescriptorHeaps(uint32_t numHeaps, ID3D12DescriptorHeap* const* heaps)
{
    m_commandList->SetDescriptorHeaps(numHeaps, heaps);
}

// Root parameters - Graphics
void CommandList::SetGraphicsRoot32BitConstant(uint32_t rootIndex, uint32_t srcData, uint32_t destOffsetIn32BitValues)
{
    m_commandList->SetGraphicsRoot32BitConstant(rootIndex, srcData, destOffsetIn32BitValues);
}

void CommandList::SetGraphicsRoot32BitConstants(uint32_t rootIndex, uint32_t num32BitValues, const void* srcData, uint32_t destOffsetIn32BitValues)
{
    m_commandList->SetGraphicsRoot32BitConstants(rootIndex, num32BitValues, srcData, destOffsetIn32BitValues);
}

void CommandList::SetGraphicsRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation)
{
    m_commandList->SetGraphicsRootConstantBufferView(rootIndex, bufferLocation);
}

void CommandList::SetGraphicsRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor)
{
    m_commandList->SetGraphicsRootDescriptorTable(rootIndex, baseDescriptor);
}

// Root parameters - Compute
void CommandList::SetComputeRoot32BitConstant(uint32_t rootIndex, uint32_t srcData, uint32_t destOffsetIn32BitValues)
{
    m_commandList->SetComputeRoot32BitConstant(rootIndex, srcData, destOffsetIn32BitValues);
}

void CommandList::SetComputeRoot32BitConstants(uint32_t rootIndex, uint32_t num32BitValues, const void* srcData, uint32_t destOffsetIn32BitValues)
{
    m_commandList->SetComputeRoot32BitConstants(rootIndex, num32BitValues, srcData, destOffsetIn32BitValues);
}

void CommandList::SetComputeRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation)
{
    m_commandList->SetComputeRootConstantBufferView(rootIndex, bufferLocation);
}

void CommandList::SetComputeRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor)
{
    m_commandList->SetComputeRootDescriptorTable(rootIndex, baseDescriptor);
}

// Resource binding
void CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t numViews, const D3D12_VERTEX_BUFFER_VIEW* views)
{
    m_commandList->IASetVertexBuffers(startSlot, numViews, views);
}

void CommandList::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view)
{
    m_commandList->IASetIndexBuffer(view);
}
