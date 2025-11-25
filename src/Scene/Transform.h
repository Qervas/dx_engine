#pragma once

#include "Entity.h"
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

// Forward declaration
class Scene;

// Transform component with hierarchical parent-child support
struct Transform
{
    // Local transform (relative to parent)
    XMFLOAT3 localPosition = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4 localRotation = { 0.0f, 0.0f, 0.0f, 1.0f };  // Quaternion (x, y, z, w)
    XMFLOAT3 localScale = { 1.0f, 1.0f, 1.0f };

    // Hierarchy
    Entity parent = NULL_ENTITY;
    std::vector<Entity> children;

    // Cached world transform
    XMFLOAT4X4 worldMatrix;
    bool dirty = true;

    Transform()
    {
        XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
    }

    // Set local position
    void SetLocalPosition(const XMFLOAT3& pos)
    {
        localPosition = pos;
        MarkDirty();
    }

    void SetLocalPosition(float x, float y, float z)
    {
        localPosition = XMFLOAT3(x, y, z);
        MarkDirty();
    }

    // Set local rotation from euler angles (in radians)
    void SetLocalRotationEuler(float pitch, float yaw, float roll)
    {
        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
        XMStoreFloat4(&localRotation, quat);
        MarkDirty();
    }

    // Set local rotation from quaternion
    void SetLocalRotation(const XMFLOAT4& quat)
    {
        localRotation = quat;
        MarkDirty();
    }

    // Set local scale
    void SetLocalScale(const XMFLOAT3& scale)
    {
        localScale = scale;
        MarkDirty();
    }

    void SetLocalScale(float uniformScale)
    {
        localScale = XMFLOAT3(uniformScale, uniformScale, uniformScale);
        MarkDirty();
    }

    // Rotate by euler angles (additive)
    void Rotate(float deltaPitch, float deltaYaw, float deltaRoll)
    {
        XMVECTOR currentQuat = XMLoadFloat4(&localRotation);
        XMVECTOR deltaQuat = XMQuaternionRotationRollPitchYaw(deltaPitch, deltaYaw, deltaRoll);
        XMVECTOR newQuat = XMQuaternionMultiply(currentQuat, deltaQuat);
        XMStoreFloat4(&localRotation, XMQuaternionNormalize(newQuat));
        MarkDirty();
    }

    // Translate (additive)
    void Translate(float dx, float dy, float dz)
    {
        localPosition.x += dx;
        localPosition.y += dy;
        localPosition.z += dz;
        MarkDirty();
    }

    void Translate(const XMFLOAT3& delta)
    {
        Translate(delta.x, delta.y, delta.z);
    }

    // Get local transform matrix
    XMMATRIX GetLocalMatrix() const
    {
        XMVECTOR pos = XMLoadFloat3(&localPosition);
        XMVECTOR rot = XMLoadFloat4(&localRotation);
        XMVECTOR scale = XMLoadFloat3(&localScale);

        return XMMatrixScalingFromVector(scale) *
               XMMatrixRotationQuaternion(rot) *
               XMMatrixTranslationFromVector(pos);
    }

    // Get cached world matrix
    XMMATRIX GetWorldMatrix() const
    {
        return XMLoadFloat4x4(&worldMatrix);
    }

    // Get world position (extracted from world matrix)
    XMFLOAT3 GetWorldPosition() const
    {
        return XMFLOAT3(worldMatrix._41, worldMatrix._42, worldMatrix._43);
    }

    // Update world matrix (called by Scene::UpdateTransforms)
    void UpdateWorldMatrix(Scene* scene);

    // Set parent entity
    void SetParent(Entity newParent, Scene* scene);

    // Add child entity
    void AddChild(Entity child);

    // Remove child entity
    void RemoveChild(Entity child);

    // Mark this transform and all children as dirty
    void MarkDirty();

private:
    void MarkChildrenDirty(Scene* scene);
};
