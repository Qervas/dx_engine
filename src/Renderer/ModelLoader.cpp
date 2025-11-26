#include "ModelLoader.h"
#include "../RHI/Device.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;

ModelLoader::ModelLoader(GraphicsDevice* device)
    : m_device(device)
{
}

std::unique_ptr<ModelData> ModelLoader::LoadFromFile(const std::string& filepath)
{
    // Get file extension
    size_t dotPos = filepath.rfind('.');
    if (dotPos == std::string::npos)
    {
        m_lastError = "No file extension found";
        return nullptr;
    }

    std::string ext = filepath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".obj")
    {
        return LoadOBJ(filepath);
    }
    else
    {
        m_lastError = "Unsupported file format: " + ext;
        return nullptr;
    }
}

ModelLoader::OBJVertex ModelLoader::ParseFaceVertex(const std::string& token)
{
    OBJVertex vertex;

    // Parse formats: "v", "v/vt", "v/vt/vn", "v//vn"
    std::stringstream ss(token);
    std::string part;

    // Position index (always present)
    if (std::getline(ss, part, '/'))
    {
        vertex.posIndex = std::stoi(part) - 1;  // OBJ indices are 1-based
    }

    // Texture coordinate index (optional)
    if (std::getline(ss, part, '/'))
    {
        if (!part.empty())
        {
            vertex.uvIndex = std::stoi(part) - 1;
        }
    }

    // Normal index (optional)
    if (std::getline(ss, part, '/'))
    {
        if (!part.empty())
        {
            vertex.normalIndex = std::stoi(part) - 1;
        }
    }

    return vertex;
}

std::unique_ptr<ModelData> ModelLoader::LoadOBJ(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        m_lastError = "Failed to open file: " + filepath;
        return nullptr;
    }

    // Raw data from OBJ file
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT2> texCoords;
    std::vector<XMFLOAT3> normals;

    // Current mesh data
    std::vector<VertexPosUVNormalTangent> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<OBJVertex, uint32_t, OBJVertexHash> vertexMap;

    // Result
    auto modelData = std::make_unique<ModelData>();
    std::string currentMaterial;

    std::string line;
    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v")
        {
            // Vertex position
            XMFLOAT3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (prefix == "vt")
        {
            // Texture coordinate
            XMFLOAT2 uv;
            ss >> uv.x >> uv.y;
            // Flip V coordinate (OBJ uses bottom-left origin)
            uv.y = 1.0f - uv.y;
            texCoords.push_back(uv);
        }
        else if (prefix == "vn")
        {
            // Vertex normal
            XMFLOAT3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "f")
        {
            // Face (triangle or polygon)
            std::vector<std::string> faceTokens;
            std::string token;
            while (ss >> token)
            {
                faceTokens.push_back(token);
            }

            // Triangulate polygon (fan triangulation)
            for (size_t i = 1; i + 1 < faceTokens.size(); ++i)
            {
                OBJVertex faceVertices[3] = {
                    ParseFaceVertex(faceTokens[0]),
                    ParseFaceVertex(faceTokens[i]),
                    ParseFaceVertex(faceTokens[i + 1])
                };

                for (int j = 0; j < 3; ++j)
                {
                    const OBJVertex& objVert = faceVertices[j];

                    // Check if this vertex combination already exists
                    auto it = vertexMap.find(objVert);
                    if (it != vertexMap.end())
                    {
                        indices.push_back(it->second);
                    }
                    else
                    {
                        // Create new vertex
                        VertexPosUVNormalTangent vertex = {};

                        // Position
                        if (objVert.posIndex >= 0 && objVert.posIndex < static_cast<int>(positions.size()))
                        {
                            vertex.position = positions[objVert.posIndex];
                        }

                        // Texture coordinate
                        if (objVert.uvIndex >= 0 && objVert.uvIndex < static_cast<int>(texCoords.size()))
                        {
                            vertex.texCoord = texCoords[objVert.uvIndex];
                        }

                        // Normal
                        if (objVert.normalIndex >= 0 && objVert.normalIndex < static_cast<int>(normals.size()))
                        {
                            vertex.normal = normals[objVert.normalIndex];
                        }

                        // Tangent will be calculated later
                        vertex.tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);

                        uint32_t newIndex = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                        vertexMap[objVert] = newIndex;
                        indices.push_back(newIndex);
                    }
                }
            }
        }
        else if (prefix == "usemtl")
        {
            // Material change - if we have accumulated geometry, create a mesh
            if (!indices.empty())
            {
                // Calculate tangents before creating mesh
                CalculateTangents(vertices, indices);

                auto mesh = std::make_unique<Mesh>(m_device);
                if (mesh->CreateWithTangents(vertices.data(), static_cast<uint32_t>(vertices.size()),
                                             indices.data(), static_cast<uint32_t>(indices.size())))
                {
                    modelData->meshes.push_back(std::move(mesh));
                    modelData->materialNames.push_back(currentMaterial);
                }

                vertices.clear();
                indices.clear();
                vertexMap.clear();
            }

            ss >> currentMaterial;
        }
        else if (prefix == "o" || prefix == "g")
        {
            // Object or group - for now, treat as material boundary
            // Could be used for sub-mesh separation
        }
    }

    // Create final mesh with remaining geometry
    if (!indices.empty())
    {
        // Calculate tangents
        CalculateTangents(vertices, indices);

        auto mesh = std::make_unique<Mesh>(m_device);
        if (mesh->CreateWithTangents(vertices.data(), static_cast<uint32_t>(vertices.size()),
                                     indices.data(), static_cast<uint32_t>(indices.size())))
        {
            modelData->meshes.push_back(std::move(mesh));
            modelData->materialNames.push_back(currentMaterial);
        }
    }

    if (modelData->meshes.empty())
    {
        m_lastError = "No valid meshes found in file";
        return nullptr;
    }

    return modelData;
}

void ModelLoader::CalculateTangents(
    std::vector<VertexPosUVNormalTangent>& vertices,
    const std::vector<uint32_t>& indices)
{
    // Initialize tangent accumulation
    std::vector<XMFLOAT3> tangents(vertices.size(), XMFLOAT3(0, 0, 0));
    std::vector<XMFLOAT3> bitangents(vertices.size(), XMFLOAT3(0, 0, 0));

    // Calculate tangent and bitangent for each triangle
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        const XMFLOAT3& p0 = vertices[i0].position;
        const XMFLOAT3& p1 = vertices[i1].position;
        const XMFLOAT3& p2 = vertices[i2].position;

        const XMFLOAT2& uv0 = vertices[i0].texCoord;
        const XMFLOAT2& uv1 = vertices[i1].texCoord;
        const XMFLOAT2& uv2 = vertices[i2].texCoord;

        // Edge vectors
        XMFLOAT3 e1(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
        XMFLOAT3 e2(p2.x - p0.x, p2.y - p0.y, p2.z - p0.z);

        // UV delta
        float du1 = uv1.x - uv0.x;
        float dv1 = uv1.y - uv0.y;
        float du2 = uv2.x - uv0.x;
        float dv2 = uv2.y - uv0.y;

        float det = du1 * dv2 - du2 * dv1;
        if (fabsf(det) < 1e-6f)
            det = 1.0f;

        float invDet = 1.0f / det;

        XMFLOAT3 tangent(
            invDet * (dv2 * e1.x - dv1 * e2.x),
            invDet * (dv2 * e1.y - dv1 * e2.y),
            invDet * (dv2 * e1.z - dv1 * e2.z)
        );

        XMFLOAT3 bitangent(
            invDet * (-du2 * e1.x + du1 * e2.x),
            invDet * (-du2 * e1.y + du1 * e2.y),
            invDet * (-du2 * e1.z + du1 * e2.z)
        );

        // Accumulate
        tangents[i0].x += tangent.x; tangents[i0].y += tangent.y; tangents[i0].z += tangent.z;
        tangents[i1].x += tangent.x; tangents[i1].y += tangent.y; tangents[i1].z += tangent.z;
        tangents[i2].x += tangent.x; tangents[i2].y += tangent.y; tangents[i2].z += tangent.z;

        bitangents[i0].x += bitangent.x; bitangents[i0].y += bitangent.y; bitangents[i0].z += bitangent.z;
        bitangents[i1].x += bitangent.x; bitangents[i1].y += bitangent.y; bitangents[i1].z += bitangent.z;
        bitangents[i2].x += bitangent.x; bitangents[i2].y += bitangent.y; bitangents[i2].z += bitangent.z;
    }

    // Orthonormalize and store tangents
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        XMVECTOR n = XMLoadFloat3(&vertices[i].normal);
        XMVECTOR t = XMLoadFloat3(&tangents[i]);

        // Gram-Schmidt orthogonalize
        t = XMVector3Normalize(XMVectorSubtract(t, XMVectorScale(n, XMVectorGetX(XMVector3Dot(n, t)))));

        XMStoreFloat3(&vertices[i].tangent, t);
    }
}
