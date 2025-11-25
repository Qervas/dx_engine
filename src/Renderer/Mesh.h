#pragma once

#include "../RHI/Buffer.h"
#include <DirectXMath.h>
#include <vector>
#include <memory>

using namespace DirectX;

// Forward declarations
class CommandList;

struct VertexPosUV
{
    XMFLOAT3 position;
    XMFLOAT2 texCoord;
};

struct VertexPosUVNormal
{
    XMFLOAT3 position;
    XMFLOAT2 texCoord;
    XMFLOAT3 normal;
};

struct VertexPosUVNormalTangent
{
    XMFLOAT3 position;
    XMFLOAT2 texCoord;
    XMFLOAT3 normal;
    XMFLOAT3 tangent;
};

class Mesh
{
public:
    Mesh(GraphicsDevice* device);
    ~Mesh();

    // Create mesh from vertex/index data (legacy - no normals)
    bool Create(const VertexPosUV* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount);

    // Create mesh from vertex/index data with normals
    bool CreateWithNormals(const VertexPosUVNormal* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount);

    // Create mesh from vertex/index data with normals and tangents
    bool CreateWithTangents(const VertexPosUVNormalTangent* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount);

    // Generate common shapes
    static Mesh* CreateCube(GraphicsDevice* device);
    static Mesh* CreateCubeWithTangents(GraphicsDevice* device);

    // Drawing
    void Draw(CommandList* commandList);

    // Accessors
    uint32_t GetIndexCount() const { return m_indexCount; }

private:
    GraphicsDevice* m_device;
    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Buffer> m_indexBuffer;
    uint32_t m_indexCount = 0;
};
