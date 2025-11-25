#pragma once

#include "Entity.h"
#include "Component.h"
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <string>
#include <memory>

// Forward declarations
struct Transform;

// EntityInfo - metadata for each entity
struct EntityInfo
{
    std::string name;
    bool active = true;
    uint32_t generation = 0;
};

class Scene
{
public:
    Scene();
    ~Scene();

    // Entity management
    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity entity);
    bool IsValid(Entity entity) const;

    // Entity info
    void SetEntityName(Entity entity, const std::string& name);
    const std::string& GetEntityName(Entity entity) const;
    void SetEntityActive(Entity entity, bool active);
    bool IsEntityActive(Entity entity) const;

    // Component management
    template<typename T>
    T* AddComponent(Entity entity);

    template<typename T>
    T* AddComponent(Entity entity, const T& component);

    template<typename T>
    T* GetComponent(Entity entity);

    template<typename T>
    const T* GetComponent(Entity entity) const;

    template<typename T>
    void RemoveComponent(Entity entity);

    template<typename T>
    bool HasComponent(Entity entity) const;

    // Get all entities with a specific component
    template<typename T>
    const std::vector<Entity>& GetEntitiesWithComponent() const;

    // Get component pool for iteration
    template<typename T>
    ComponentPool<T>* GetComponentPool();

    // Scene updates
    void Update(float deltaTime);
    void UpdateTransforms();

    // Entity count
    size_t GetEntityCount() const { return m_entityCount; }

private:
    // Get or create component pool for type T
    template<typename T>
    ComponentPool<T>* GetOrCreatePool();

    // Entity storage
    std::vector<EntityInfo> m_entityInfos;     // Entity ID -> EntityInfo
    std::vector<uint32_t> m_freeIds;           // Recycled entity IDs
    uint32_t m_nextId = 0;
    size_t m_entityCount = 0;

    // Component pools (type-erased)
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_componentPools;

    // Static empty entity list for when pool doesn't exist
    static const std::vector<Entity> s_emptyEntityList;
};

// Template implementations

template<typename T>
ComponentPool<T>* Scene::GetOrCreatePool()
{
    std::type_index typeId = std::type_index(typeid(T));

    auto it = m_componentPools.find(typeId);
    if (it == m_componentPools.end())
    {
        auto pool = std::make_unique<ComponentPool<T>>();
        auto* rawPtr = pool.get();
        m_componentPools[typeId] = std::move(pool);
        return rawPtr;
    }

    return static_cast<ComponentPool<T>*>(it->second.get());
}

template<typename T>
T* Scene::AddComponent(Entity entity)
{
    if (!IsValid(entity))
        return nullptr;

    auto* pool = GetOrCreatePool<T>();
    return pool->Add(entity);
}

template<typename T>
T* Scene::AddComponent(Entity entity, const T& component)
{
    if (!IsValid(entity))
        return nullptr;

    auto* pool = GetOrCreatePool<T>();
    return pool->Add(entity, component);
}

template<typename T>
T* Scene::GetComponent(Entity entity)
{
    if (!IsValid(entity))
        return nullptr;

    std::type_index typeId = std::type_index(typeid(T));
    auto it = m_componentPools.find(typeId);
    if (it == m_componentPools.end())
        return nullptr;

    return static_cast<ComponentPool<T>*>(it->second.get())->Get(entity);
}

template<typename T>
const T* Scene::GetComponent(Entity entity) const
{
    if (!IsValid(entity))
        return nullptr;

    std::type_index typeId = std::type_index(typeid(T));
    auto it = m_componentPools.find(typeId);
    if (it == m_componentPools.end())
        return nullptr;

    return static_cast<const ComponentPool<T>*>(it->second.get())->Get(entity);
}

template<typename T>
void Scene::RemoveComponent(Entity entity)
{
    if (!IsValid(entity))
        return;

    std::type_index typeId = std::type_index(typeid(T));
    auto it = m_componentPools.find(typeId);
    if (it != m_componentPools.end())
    {
        it->second->Remove(entity);
    }
}

template<typename T>
bool Scene::HasComponent(Entity entity) const
{
    if (!IsValid(entity))
        return false;

    std::type_index typeId = std::type_index(typeid(T));
    auto it = m_componentPools.find(typeId);
    if (it == m_componentPools.end())
        return false;

    return it->second->Has(entity);
}

template<typename T>
const std::vector<Entity>& Scene::GetEntitiesWithComponent() const
{
    std::type_index typeId = std::type_index(typeid(T));
    auto it = m_componentPools.find(typeId);
    if (it == m_componentPools.end())
        return s_emptyEntityList;

    return static_cast<const ComponentPool<T>*>(it->second.get())->GetEntities();
}

template<typename T>
ComponentPool<T>* Scene::GetComponentPool()
{
    std::type_index typeId = std::type_index(typeid(T));
    auto it = m_componentPools.find(typeId);
    if (it == m_componentPools.end())
        return nullptr;

    return static_cast<ComponentPool<T>*>(it->second.get());
}
