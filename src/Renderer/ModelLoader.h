#pragma once

#include "Mesh.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations
class GraphicsDevice;

// Loaded model data (can contain multiple meshes)
struct ModelData
{
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::string> materialNames;  // Material name per mesh
};

// OBJ file loader
class ModelLoader
{
public:
    ModelLoader(GraphicsDevice* device);
    ~ModelLoader() = default;

    // Load model from file (auto-detects format by extension)
    std::unique_ptr<ModelData> LoadFromFile(const std::string& filepath);

    // Load specific formats
    std::unique_ptr<ModelData> LoadOBJ(const std::string& filepath);

    // Get last error message
    const std::string& GetLastError() const { return m_lastError; }

private:
    struct OBJVertex
    {
        int posIndex = -1;
        int uvIndex = -1;
        int normalIndex = -1;

        bool operator==(const OBJVertex& other) const
        {
            return posIndex == other.posIndex &&
                   uvIndex == other.uvIndex &&
                   normalIndex == other.normalIndex;
        }
    };

    struct OBJVertexHash
    {
        size_t operator()(const OBJVertex& v) const
        {
            return std::hash<int>()(v.posIndex) ^
                   (std::hash<int>()(v.uvIndex) << 1) ^
                   (std::hash<int>()(v.normalIndex) << 2);
        }
    };

    // Parse OBJ face element (e.g., "1/2/3" or "1//3" or "1")
    OBJVertex ParseFaceVertex(const std::string& token);

    // Calculate tangent vectors for normal mapping
    void CalculateTangents(
        std::vector<VertexPosUVNormalTangent>& vertices,
        const std::vector<uint32_t>& indices);

    GraphicsDevice* m_device;
    std::string m_lastError;
};
