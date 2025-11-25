#pragma once

#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

// Maximum number of lights supported
constexpr uint32_t MAX_LIGHTS = 4;

enum class LightType : uint32_t
{
    Directional = 0,
    Point = 1,
    Spot = 2
};

// Light data structure (matches shader layout)
struct LightData
{
    XMFLOAT3 position;      // For point/spot lights
    uint32_t type;          // LightType

    XMFLOAT3 direction;     // For directional/spot lights
    float range;            // For point/spot lights

    XMFLOAT3 color;
    float intensity;

    float spotAngle;        // For spot lights (cosine of half-angle)
    float _padding[3];
};

// Scene lighting data (sent to GPU)
struct SceneLightingData
{
    XMFLOAT3 cameraPosition;
    uint32_t lightCount;

    LightData lights[MAX_LIGHTS];    // Support up to MAX_LIGHTS
};

class Light
{
public:
    Light(LightType type = LightType::Point)
        : m_type(type)
    {
    }

    // Setters
    void SetType(LightType type) { m_type = type; }
    void SetPosition(const XMFLOAT3& position) { m_position = position; }
    void SetDirection(const XMFLOAT3& direction) { m_direction = direction; }
    void SetColor(const XMFLOAT3& color) { m_color = color; }
    void SetIntensity(float intensity) { m_intensity = intensity; }
    void SetRange(float range) { m_range = range; }
    void SetSpotAngle(float angleRadians) { m_spotAngle = cosf(angleRadians * 0.5f); }

    // Getters
    LightType GetType() const { return m_type; }
    const XMFLOAT3& GetPosition() const { return m_position; }
    const XMFLOAT3& GetDirection() const { return m_direction; }
    const XMFLOAT3& GetColor() const { return m_color; }
    float GetIntensity() const { return m_intensity; }
    float GetRange() const { return m_range; }
    float GetSpotAngle() const { return m_spotAngle; }

    // Get light data for GPU
    LightData GetLightData() const
    {
        LightData data;
        data.position = m_position;
        data.type = static_cast<uint32_t>(m_type);
        data.direction = m_direction;
        data.range = m_range;
        data.color = m_color;
        data.intensity = m_intensity;
        data.spotAngle = m_spotAngle;
        return data;
    }

private:
    LightType m_type = LightType::Point;
    XMFLOAT3 m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
    XMFLOAT3 m_color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    float m_intensity = 1.0f;
    float m_range = 10.0f;
    float m_spotAngle = 0.866f;  // cos(30 degrees)
};
