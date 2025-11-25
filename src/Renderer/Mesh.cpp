#include "Mesh.h"
#include "../RHI/Device.h"
#include "../RHI/CommandList.h"

Mesh::Mesh(GraphicsDevice* device)
    : m_device(device)
{
}

Mesh::~Mesh()
{
}

bool Mesh::Create(const VertexPosUV* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
{
    m_indexCount = indexCount;

    // Create vertex buffer
    m_vertexBuffer = std::make_unique<Buffer>(m_device);
    if (!m_vertexBuffer->Create(vertexCount * sizeof(VertexPosUV), sizeof(VertexPosUV), BufferUsage::Upload, vertices))
    {
        return false;
    }

    // Create index buffer
    m_indexBuffer = std::make_unique<Buffer>(m_device);
    if (!m_indexBuffer->Create(indexCount * sizeof(uint32_t), sizeof(uint32_t), BufferUsage::Upload, indices))
    {
        return false;
    }

    return true;
}

bool Mesh::CreateWithNormals(const VertexPosUVNormal* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
{
    m_indexCount = indexCount;

    // Create vertex buffer
    m_vertexBuffer = std::make_unique<Buffer>(m_device);
    if (!m_vertexBuffer->Create(vertexCount * sizeof(VertexPosUVNormal), sizeof(VertexPosUVNormal), BufferUsage::Upload, vertices))
    {
        return false;
    }

    // Create index buffer
    m_indexBuffer = std::make_unique<Buffer>(m_device);
    if (!m_indexBuffer->Create(indexCount * sizeof(uint32_t), sizeof(uint32_t), BufferUsage::Upload, indices))
    {
        return false;
    }

    return true;
}

bool Mesh::CreateWithTangents(const VertexPosUVNormalTangent* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
{
    m_indexCount = indexCount;

    // Create vertex buffer
    m_vertexBuffer = std::make_unique<Buffer>(m_device);
    if (!m_vertexBuffer->Create(vertexCount * sizeof(VertexPosUVNormalTangent), sizeof(VertexPosUVNormalTangent), BufferUsage::Upload, vertices))
    {
        return false;
    }

    // Create index buffer
    m_indexBuffer = std::make_unique<Buffer>(m_device);
    if (!m_indexBuffer->Create(indexCount * sizeof(uint32_t), sizeof(uint32_t), BufferUsage::Upload, indices))
    {
        return false;
    }

    return true;
}

Mesh* Mesh::CreateCube(GraphicsDevice* device)
{
    // Cube vertices with texture coordinates and normals
    VertexPosUVNormal vertices[] =
    {
        // Front face (normal: 0, 0, -1)
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

        // Back face (normal: 0, 0, 1)
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },

        // Top face (normal: 0, 1, 0)
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

        // Bottom face (normal: 0, -1, 0)
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

        // Left face (normal: -1, 0, 0)
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

        // Right face (normal: 1, 0, 0)
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
    };

    // Cube indices
    uint32_t indices[] =
    {
        // Front
        0, 1, 2,  0, 2, 3,
        // Back
        4, 5, 6,  4, 6, 7,
        // Top
        8, 9, 10,  8, 10, 11,
        // Bottom
        12, 13, 14,  12, 14, 15,
        // Left
        16, 17, 18,  16, 18, 19,
        // Right
        20, 21, 22,  20, 22, 23
    };

    Mesh* mesh = new Mesh(device);
    if (!mesh->CreateWithNormals(vertices, _countof(vertices), indices, _countof(indices)))
    {
        delete mesh;
        return nullptr;
    }

    return mesh;
}

Mesh* Mesh::CreateCubeWithTangents(GraphicsDevice* device)
{
    // Cube vertices with texture coordinates, normals, and tangents
    VertexPosUVNormalTangent vertices[] =
    {
        // Front face (normal: 0, 0, -1, tangent: 1, 0, 0)
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

        // Back face (normal: 0, 0, 1, tangent: -1, 0, 0)
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

        // Top face (normal: 0, 1, 0, tangent: 1, 0, 0)
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

        // Bottom face (normal: 0, -1, 0, tangent: 1, 0, 0)
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

        // Left face (normal: -1, 0, 0, tangent: 0, 0, 1)
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },

        // Right face (normal: 1, 0, 0, tangent: 0, 0, -1)
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
    };

    // Cube indices
    uint32_t indices[] =
    {
        // Front
        0, 1, 2,  0, 2, 3,
        // Back
        4, 5, 6,  4, 6, 7,
        // Top
        8, 9, 10,  8, 10, 11,
        // Bottom
        12, 13, 14,  12, 14, 15,
        // Left
        16, 17, 18,  16, 18, 19,
        // Right
        20, 21, 22,  20, 22, 23
    };

    Mesh* mesh = new Mesh(device);
    if (!mesh->CreateWithTangents(vertices, _countof(vertices), indices, _countof(indices)))
    {
        delete mesh;
        return nullptr;
    }

    return mesh;
}

void Mesh::Draw(CommandList* commandList)
{
    D3D12_VERTEX_BUFFER_VIEW vbv = m_vertexBuffer->GetVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW ibv = m_indexBuffer->GetIndexBufferView();

    commandList->SetVertexBuffers(0, 1, &vbv);
    commandList->SetIndexBuffer(&ibv);
    commandList->DrawIndexed(m_indexCount, 0, 0);
}
