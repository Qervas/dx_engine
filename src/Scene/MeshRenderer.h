#pragma once

#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include <memory>

// MeshRenderer component - holds mesh and material for rendering
struct MeshRenderer
{
    Mesh* mesh = nullptr;           // Non-owning pointer to mesh
    Material* material = nullptr;   // Non-owning pointer to material
    bool visible = true;
    bool castShadows = true;
    bool receiveShadows = true;

    MeshRenderer() = default;

    MeshRenderer(Mesh* m, Material* mat)
        : mesh(m), material(mat) {}

    bool IsValid() const
    {
        return mesh != nullptr && material != nullptr && mesh->IsValid();
    }
};
