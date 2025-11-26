#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
public:
    Camera();
    ~Camera();

    // Camera setup
    void SetPosition(const XMFLOAT3& position);
    void SetRotation(float pitch, float yaw);
    void SetPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);
    void SetOrthographic(float width, float height, float nearPlane, float farPlane);

    // Get matrices
    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix() const;
    XMMATRIX GetViewProjectionMatrix() const;

    // Camera movement
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);
    void Rotate(float deltaPitch, float deltaYaw);

    // FPS-style input processing
    void ProcessFPSInput(float deltaTime, float moveSpeed = 5.0f, float lookSensitivity = 0.002f);

    // Accessors
    XMFLOAT3 GetPosition() const { return m_position; }
    XMFLOAT3 GetForward() const;
    XMFLOAT3 GetRight() const;
    XMFLOAT3 GetUp() const;

private:
    void UpdateViewMatrix();

    XMFLOAT3 m_position;
    float m_pitch;  // Rotation around X axis (radians)
    float m_yaw;    // Rotation around Y axis (radians)

    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;
    bool m_viewDirty;
};
