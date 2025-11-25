#pragma once

#include <vector>
#include <cstdint>

class ProceduralTexture
{
public:
    // Generate a simple checkerboard pattern
    static std::vector<uint8_t> GenerateCheckerboard(uint32_t width, uint32_t height, uint32_t checkSize = 32);

    // Generate a flat normal map (all normals pointing up: 0.5, 0.5, 1.0 in RGB)
    static std::vector<uint8_t> GenerateFlatNormalMap(uint32_t width, uint32_t height);

    // Generate a simple brick normal map pattern
    static std::vector<uint8_t> GenerateBrickNormalMap(uint32_t width, uint32_t height);
};
