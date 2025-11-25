#include "Scene.h"
#include "Transform.h"

// Static member initialization
const std::vector<Entity> Scene::s_emptyEntityList;

Scene::Scene()
{
    // Reserve some initial space for entities
    m_entityInfos.reserve(256);
}

Scene::~Scene()
{
    // Component pools are automatically cleaned up by unique_ptr
}

Entity Scene::CreateEntity(const std::string& name)
{
    uint32_t id;
    uint32_t generation;

    if (!m_freeIds.empty())
    {
        // Reuse recycled ID
        id = m_freeIds.back();
        m_freeIds.pop_back();
        generation = m_entityInfos[id].generation;
    }
    else
    {
        // Allocate new ID
        id = m_nextId++;
        generation = 0;

        // Grow entity info array if needed
        if (id >= m_entityInfos.size())
        {
            m_entityInfos.resize(id + 64);
        }
    }

    // Setup entity info
    m_entityInfos[id].name = name;
    m_entityInfos[id].active = true;
    m_entityInfos[id].generation = generation;

    m_entityCount++;

    return Entity(id, generation);
}

void Scene::DestroyEntity(Entity entity)
{
    if (!IsValid(entity))
        return;

    // Remove all components for this entity
    for (auto& [typeId, pool] : m_componentPools)
    {
        pool->Remove(entity);
    }

    // Increment generation so old handles become invalid
    m_entityInfos[entity.id].generation++;
    m_entityInfos[entity.id].active = false;
    m_entityInfos[entity.id].name.clear();

    // Add to free list for reuse
    m_freeIds.push_back(entity.id);

    m_entityCount--;
}

bool Scene::IsValid(Entity entity) const
{
    if (!entity.IsValid())
        return false;

    if (entity.id >= m_entityInfos.size())
        return false;

    // Check generation matches (entity wasn't destroyed and recreated)
    return m_entityInfos[entity.id].generation == entity.generation;
}

void Scene::SetEntityName(Entity entity, const std::string& name)
{
    if (IsValid(entity))
    {
        m_entityInfos[entity.id].name = name;
    }
}

const std::string& Scene::GetEntityName(Entity entity) const
{
    static const std::string emptyString;
    if (!IsValid(entity))
        return emptyString;

    return m_entityInfos[entity.id].name;
}

void Scene::SetEntityActive(Entity entity, bool active)
{
    if (IsValid(entity))
    {
        m_entityInfos[entity.id].active = active;
    }
}

bool Scene::IsEntityActive(Entity entity) const
{
    if (!IsValid(entity))
        return false;

    return m_entityInfos[entity.id].active;
}

void Scene::Update(float deltaTime)
{
    // Update transforms (propagate hierarchy)
    UpdateTransforms();

    // Future: Add other system updates here (physics, animation, etc.)
}

void Scene::UpdateTransforms()
{
    // Get transform pool
    auto* transformPool = GetComponentPool<Transform>();
    if (!transformPool)
        return;

    // Update all dirty transforms
    // We iterate through all transforms and update world matrices
    // A more optimized approach would sort by hierarchy depth
    for (size_t i = 0; i < transformPool->Size(); ++i)
    {
        Entity entity = transformPool->GetEntity(i);
        Transform* transform = transformPool->Get(entity);

        if (transform && transform->dirty)
        {
            transform->UpdateWorldMatrix(this);
        }
    }
}
