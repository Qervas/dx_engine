#include "Transform.h"
#include "Scene.h"

void Transform::UpdateWorldMatrix(Scene* scene)
{
    if (!dirty)
        return;

    XMMATRIX localMatrix = GetLocalMatrix();

    if (parent.IsValid() && scene)
    {
        // Get parent's world matrix
        Transform* parentTransform = scene->GetComponent<Transform>(parent);
        if (parentTransform)
        {
            // Ensure parent is updated first
            if (parentTransform->dirty)
            {
                parentTransform->UpdateWorldMatrix(scene);
            }

            // Combine with parent's world matrix
            XMMATRIX parentWorld = parentTransform->GetWorldMatrix();
            XMMATRIX worldMatrix = localMatrix * parentWorld;
            XMStoreFloat4x4(&this->worldMatrix, worldMatrix);
        }
        else
        {
            // Parent doesn't have transform, use local as world
            XMStoreFloat4x4(&worldMatrix, localMatrix);
        }
    }
    else
    {
        // No parent, local is world
        XMStoreFloat4x4(&worldMatrix, localMatrix);
    }

    dirty = false;
}

void Transform::SetParent(Entity newParent, Scene* scene)
{
    if (!scene)
        return;

    // Get our own entity by finding it in the scene
    // This is a bit awkward - in practice you'd call scene->SetParent(childEntity, parentEntity)
    // For now, we need the scene to manage the hierarchy properly

    Entity thisEntity = NULL_ENTITY;

    // Find our entity in the transform pool
    auto* pool = scene->GetComponentPool<Transform>();
    if (pool)
    {
        for (size_t i = 0; i < pool->Size(); ++i)
        {
            Transform* t = pool->Get(pool->GetEntity(i));
            if (t == this)
            {
                thisEntity = pool->GetEntity(i);
                break;
            }
        }
    }

    if (!thisEntity.IsValid())
        return;

    // Remove from old parent's children list
    if (parent.IsValid())
    {
        Transform* oldParentTransform = scene->GetComponent<Transform>(parent);
        if (oldParentTransform)
        {
            oldParentTransform->RemoveChild(thisEntity);
        }
    }

    // Set new parent
    parent = newParent;

    // Add to new parent's children list
    if (newParent.IsValid())
    {
        Transform* newParentTransform = scene->GetComponent<Transform>(newParent);
        if (newParentTransform)
        {
            newParentTransform->AddChild(thisEntity);
        }
    }

    // Mark dirty to recalculate world matrix
    MarkDirty();
}

void Transform::AddChild(Entity child)
{
    // Check if already a child
    for (const auto& c : children)
    {
        if (c == child)
            return;
    }
    children.push_back(child);
}

void Transform::RemoveChild(Entity child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
    }
}

void Transform::MarkDirty()
{
    dirty = true;
    // Note: Children will be marked dirty when scene updates
    // This avoids needing scene pointer in MarkDirty
}

void Transform::MarkChildrenDirty(Scene* scene)
{
    if (!scene)
        return;

    for (Entity child : children)
    {
        Transform* childTransform = scene->GetComponent<Transform>(child);
        if (childTransform)
        {
            childTransform->dirty = true;
            childTransform->MarkChildrenDirty(scene);
        }
    }
}
