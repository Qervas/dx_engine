#pragma once

#include "../Core/Types.h"
#include <d3d12.h>

class GraphicsDevice;

class CommandList
{
public:
    CommandList(GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandList();

    bool Initialize();
    void Shutdown();

    // Lifecycle
    void Reset();
    void Close();

    // Drawing
    void Draw(uint32_t vertexCount, uint32_t startVertex = 0);
    void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0);
    void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex = 0, uint32_t startInstance = 0);
    void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);

    // Compute
    void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);

    // State setting
    void SetPipelineState(ID3D12PipelineState* pso);
    void SetGraphicsRootSignature(ID3D12RootSignature* rootSig);
    void SetComputeRootSignature(ID3D12RootSignature* rootSig);
    void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);

    // Viewport and scissor
    void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
    void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    // Resource barriers
    void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    void UAVBarrier(ID3D12Resource* resource);

    // Render targets
    void SetRenderTargets(uint32_t numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, const D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr);
    void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* color);
    void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth, uint8_t stencil = 0);

    // Descriptor heaps
    void SetDescriptorHeaps(uint32_t numHeaps, ID3D12DescriptorHeap* const* heaps);

    // Root parameters
    void SetGraphicsRoot32BitConstant(uint32_t rootIndex, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0);
    void SetGraphicsRoot32BitConstants(uint32_t rootIndex, uint32_t num32BitValues, const void* srcData, uint32_t destOffsetIn32BitValues = 0);
    void SetGraphicsRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetGraphicsRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);

    void SetComputeRoot32BitConstant(uint32_t rootIndex, uint32_t srcData, uint32_t destOffsetIn32BitValues = 0);
    void SetComputeRoot32BitConstants(uint32_t rootIndex, uint32_t num32BitValues, const void* srcData, uint32_t destOffsetIn32BitValues = 0);
    void SetComputeRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
    void SetComputeRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);

    // Resource binding
    void SetVertexBuffers(uint32_t startSlot, uint32_t numViews, const D3D12_VERTEX_BUFFER_VIEW* views);
    void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view);

    // Accessors
    ID3D12GraphicsCommandList* GetD3D12CommandList() const { return m_commandList.Get(); }
    ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocator.Get(); }

private:
    GraphicsDevice* m_device;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    D3D12_COMMAND_LIST_TYPE m_type;
};
