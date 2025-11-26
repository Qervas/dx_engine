#pragma once

#include "../RHI/Device.h"
#include "../RHI/Buffer.h"
#include "../RHI/Shader.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include "../RHI/CommandList.h"
#include "../Renderer/Camera.h"
#include <DirectXMath.h>
#include <vector>
#include <memory>

using namespace DirectX;

// Debug line vertex
struct DebugVertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

// Debug renderer for immediate-mode debug drawing
// All draw calls are collected and rendered at the end of the frame
class DebugRenderer
{
public:
    DebugRenderer(GraphicsDevice* device);
    ~DebugRenderer();

    bool Initialize();
    void Shutdown();

    // Set camera for view/projection matrices
    void SetCamera(Camera* camera) { m_camera = camera; }

    // === Immediate mode drawing API ===

    // Basic primitives
    void DrawLine(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT4& color);
    void DrawLine(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT3& color);

    // Shapes
    void DrawBox(const XMFLOAT3& center, const XMFLOAT3& size, const XMFLOAT4& color);
    void DrawWireBox(const XMFLOAT3& min, const XMFLOAT3& max, const XMFLOAT4& color);
    void DrawSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color, int segments = 16);
    void DrawCircle(const XMFLOAT3& center, float radius, const XMFLOAT3& normal, const XMFLOAT4& color, int segments = 32);

    // Coordinate systems
    void DrawAxes(const XMFLOAT3& origin, float scale = 1.0f);
    void DrawAxes(const XMMATRIX& transform, float scale = 1.0f);
    void DrawGrid(float size, float step, const XMFLOAT4& color);

    // Camera/frustum visualization
    void DrawFrustum(const XMMATRIX& viewProj, const XMFLOAT4& color);

    // Direction/ray
    void DrawRay(const XMFLOAT3& origin, const XMFLOAT3& direction, float length, const XMFLOAT4& color);
    void DrawArrow(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT4& color, float headSize = 0.1f);

    // === Rendering ===

    // Render all collected debug primitives
    void Render(CommandList* commandList);

    // Clear all primitives (call at end of frame)
    void Clear();

    // Accessors
    RootSignature* GetRootSignature() const { return m_rootSignature.get(); }
    PipelineState* GetPipelineState() const { return m_pipelineState.get(); }
    bool HasPrimitives() const { return !m_vertices.empty(); }

private:
    bool CreatePipeline();
    void UpdateVertexBuffer();

    GraphicsDevice* m_device = nullptr;
    Camera* m_camera = nullptr;

    // Pipeline
    std::unique_ptr<Shader> m_vertexShader;
    std::unique_ptr<Shader> m_pixelShader;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;

    // Vertex data
    std::vector<DebugVertex> m_vertices;
    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Buffer> m_constantBuffer;

    static constexpr uint32_t MAX_DEBUG_VERTICES = 65536;
};

// Debug constant buffer
struct DebugConstants
{
    XMFLOAT4X4 viewProjection;
};
