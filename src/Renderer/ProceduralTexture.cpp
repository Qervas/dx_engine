#include "ProceduralTexture.h"

std::vector<uint8_t> ProceduralTexture::GenerateCheckerboard(uint32_t width, uint32_t height, uint32_t checkSize)
{
    std::vector<uint8_t> pixels(width * height * 4); // RGBA

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint32_t checkX = x / checkSize;
            uint32_t checkY = y / checkSize;
            bool isWhite = (checkX + checkY) % 2 == 0;

            uint32_t index = (y * width + x) * 4;
            if (isWhite)
            {
                pixels[index + 0] = 255; // R
                pixels[index + 1] = 255; // G
                pixels[index + 2] = 255; // B
                pixels[index + 3] = 255; // A
            }
            else
            {
                pixels[index + 0] = 50;  // R
                pixels[index + 1] = 50;  // G
                pixels[index + 2] = 50;  // B
                pixels[index + 3] = 255; // A
            }
        }
    }

    return pixels;
}

std::vector<uint8_t> ProceduralTexture::GenerateFlatNormalMap(uint32_t width, uint32_t height)
{
    std::vector<uint8_t> pixels(width * height * 4); // RGBA

    // Flat normal map: normal pointing straight up (0, 0, 1) in world space
    // In tangent space this is stored as RGB (128, 128, 255) or (0.5, 0.5, 1.0) normalized
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint32_t index = (y * width + x) * 4;
            pixels[index + 0] = 128; // R: X component (0.0 in [-1, 1])
            pixels[index + 1] = 128; // G: Y component (0.0 in [-1, 1])
            pixels[index + 2] = 255; // B: Z component (1.0 in [-1, 1])
            pixels[index + 3] = 255; // A
        }
    }

    return pixels;
}

std::vector<uint8_t> ProceduralTexture::GenerateBrickNormalMap(uint32_t width, uint32_t height)
{
    std::vector<uint8_t> pixels(width * height * 4); // RGBA

    const uint32_t brickWidth = 64;
    const uint32_t brickHeight = 32;
    const uint32_t mortarSize = 4;

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint32_t brickY = y / brickHeight;
            uint32_t localY = y % brickHeight;

            // Offset every other row for brick pattern
            uint32_t offsetX = (brickY % 2) * (brickWidth / 2);
            uint32_t localX = (x + offsetX) % brickWidth;

            uint32_t index = (y * width + x) * 4;

            // Determine if we're in mortar or brick
            bool isMortar = (localX < mortarSize) || (localY < mortarSize);

            if (isMortar)
            {
                // Mortar: recessed, normal points slightly outward from edges
                pixels[index + 0] = 128;  // R: 0.0
                pixels[index + 1] = 128;  // G: 0.0
                pixels[index + 2] = 200;  // B: slightly recessed
                pixels[index + 3] = 255;  // A
            }
            else
            {
                // Brick: slightly raised in center
                float centerX = brickWidth / 2.0f;
                float centerY = brickHeight / 2.0f;

                float dx = (localX - centerX) / centerX;
                float dy = (localY - centerY) / centerY;

                // Create subtle bump in the middle of each brick
                float bump = 1.0f - (dx * dx + dy * dy) * 0.1f;
                bump = (bump < 0.9f) ? 0.9f : bump;

                pixels[index + 0] = 128;  // R: 0.0
                pixels[index + 1] = 128;  // G: 0.0
                pixels[index + 2] = static_cast<uint8_t>(128 + bump * 127);  // B: raised
                pixels[index + 3] = 255;  // A
            }
        }
    }

    return pixels;
}
