#include "Camera.h"
#include "../Platform/Input.h"

Camera::Camera()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_pitch(0.0f)
    , m_yaw(0.0f)
    , m_viewDirty(true)
{
    m_viewMatrix = XMMatrixIdentity();
    m_projectionMatrix = XMMatrixIdentity();
}

Camera::~Camera()
{
}

void Camera::SetPosition(const XMFLOAT3& position)
{
    m_position = position;
    m_viewDirty = true;
}

void Camera::SetRotation(float pitch, float yaw)
{
    m_pitch = pitch;
    m_yaw = yaw;
    m_viewDirty = true;
}

void Camera::SetPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane)
{
    float fovRadians = XMConvertToRadians(fovDegrees);
    m_projectionMatrix = XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, nearPlane, farPlane);
}

void Camera::SetOrthographic(float width, float height, float nearPlane, float farPlane)
{
    m_projectionMatrix = XMMatrixOrthographicLH(width, height, nearPlane, farPlane);
}

XMMATRIX Camera::GetViewMatrix() const
{
    if (m_viewDirty)
    {
        const_cast<Camera*>(this)->UpdateViewMatrix();
    }
    return m_viewMatrix;
}

XMMATRIX Camera::GetProjectionMatrix() const
{
    return m_projectionMatrix;
}

XMMATRIX Camera::GetViewProjectionMatrix() const
{
    return GetViewMatrix() * m_projectionMatrix;
}

void Camera::MoveForward(float distance)
{
    XMFLOAT3 forward = GetForward();
    m_position.x += forward.x * distance;
    m_position.y += forward.y * distance;
    m_position.z += forward.z * distance;
    m_viewDirty = true;
}

void Camera::MoveRight(float distance)
{
    XMFLOAT3 right = GetRight();
    m_position.x += right.x * distance;
    m_position.y += right.y * distance;
    m_position.z += right.z * distance;
    m_viewDirty = true;
}

void Camera::MoveUp(float distance)
{
    m_position.y += distance;
    m_viewDirty = true;
}

void Camera::Rotate(float deltaPitch, float deltaYaw)
{
    m_pitch += deltaPitch;
    m_yaw += deltaYaw;

    // Clamp pitch to avoid gimbal lock
    const float maxPitch = XM_PIDIV2 - 0.1f;
    if (m_pitch > maxPitch) m_pitch = maxPitch;
    if (m_pitch < -maxPitch) m_pitch = -maxPitch;

    m_viewDirty = true;
}

XMFLOAT3 Camera::GetForward() const
{
    // Calculate forward vector from pitch and yaw
    float cosPitch = cosf(m_pitch);
    float sinPitch = sinf(m_pitch);
    float cosYaw = cosf(m_yaw);
    float sinYaw = sinf(m_yaw);

    return XMFLOAT3(
        cosPitch * sinYaw,
        sinPitch,
        cosPitch * cosYaw
    );
}

XMFLOAT3 Camera::GetRight() const
{
    // Right vector is perpendicular to forward and up
    float cosYaw = cosf(m_yaw);
    float sinYaw = sinf(m_yaw);

    return XMFLOAT3(
        cosYaw,
        0.0f,
        -sinYaw
    );
}

XMFLOAT3 Camera::GetUp() const
{
    // Up vector: cross(forward, right) in left-handed system
    XMFLOAT3 forward = GetForward();
    XMFLOAT3 right = GetRight();

    XMVECTOR f = XMLoadFloat3(&forward);
    XMVECTOR r = XMLoadFloat3(&right);
    XMVECTOR u = XMVector3Cross(f, r);  // Fixed: was (r, f) which gave down

    XMFLOAT3 up;
    XMStoreFloat3(&up, u);
    return up;
}

void Camera::UpdateViewMatrix()
{
    XMFLOAT3 forward = GetForward();
    XMFLOAT3 up = GetUp();

    XMVECTOR posVec = XMLoadFloat3(&m_position);
    XMVECTOR forwardVec = XMLoadFloat3(&forward);
    XMVECTOR upVec = XMLoadFloat3(&up);

    XMVECTOR targetVec = XMVectorAdd(posVec, forwardVec);

    m_viewMatrix = XMMatrixLookAtLH(posVec, targetVec, upVec);
    m_viewDirty = false;
}

void Camera::ProcessFPSInput(float deltaTime, float moveSpeed, float lookSensitivity)
{
    Input& input = Input::Get();

    // Movement (WASD + QE for up/down)
    float speed = moveSpeed * deltaTime;

    // Sprint with Shift
    if (input.IsKeyDown(Key::Shift))
    {
        speed *= 2.0f;
    }

    if (input.IsKeyDown(Key::W))
    {
        MoveForward(speed);
    }
    if (input.IsKeyDown(Key::S))
    {
        MoveForward(-speed);
    }
    if (input.IsKeyDown(Key::A))
    {
        MoveRight(-speed);  // Left
    }
    if (input.IsKeyDown(Key::D))
    {
        MoveRight(speed);   // Right
    }
    if (input.IsKeyDown(Key::E) || input.IsKeyDown(Key::Space))
    {
        MoveUp(speed);      // Up
    }
    if (input.IsKeyDown(Key::Q) || input.IsKeyDown(Key::Control))
    {
        MoveUp(-speed);     // Down
    }

    // Mouse look (only when captured)
    if (input.IsMouseCaptured())
    {
        float deltaX = static_cast<float>(input.GetMouseDeltaX());
        float deltaY = static_cast<float>(input.GetMouseDeltaY());

        Rotate(-deltaY * lookSensitivity, deltaX * lookSensitivity);  // Standard FPS controls
    }
}
