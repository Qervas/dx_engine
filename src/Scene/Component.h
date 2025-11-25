#pragma once

#include "Entity.h"
#include <vector>
#include <cassert>
#include <typeindex>
#include <memory>

// Interface for type-erased component pool storage
class IComponentPool
{
public:
    virtual ~IComponentPool() = default;
    virtual void Remove(Entity entity) = 0;
    virtual bool Has(Entity entity) const = 0;
    virtual void Clear() = 0;
};

// Sparse Set based component storage
// Provides O(1) add, remove, lookup and cache-friendly iteration
template<typename T>
class ComponentPool : public IComponentPool
{
public:
    ComponentPool()
    {
        // Reserve some initial space
        m_sparse.resize(64, INVALID_ENTITY_ID);
    }

    // Add component to entity, returns pointer to component
    T* Add(Entity entity)
    {
        assert(entity.IsValid());

        // Grow sparse array if needed
        if (entity.id >= m_sparse.size())
        {
            m_sparse.resize(entity.id + 64, INVALID_ENTITY_ID);
        }

        // Check if already has component
        if (m_sparse[entity.id] != INVALID_ENTITY_ID)
        {
            // Return existing component
            return &m_dense[m_sparse[entity.id]];
        }

        // Add new component
        uint32_t denseIndex = static_cast<uint32_t>(m_dense.size());
        m_sparse[entity.id] = denseIndex;
        m_dense.emplace_back();
        m_entities.push_back(entity);

        return &m_dense.back();
    }

    // Add component with initial value
    T* Add(Entity entity, const T& component)
    {
        T* comp = Add(entity);
        *comp = component;
        return comp;
    }

    // Get component for entity (nullptr if not found)
    T* Get(Entity entity)
    {
        if (!Has(entity))
            return nullptr;
        return &m_dense[m_sparse[entity.id]];
    }

    const T* Get(Entity entity) const
    {
        if (!Has(entity))
            return nullptr;
        return &m_dense[m_sparse[entity.id]];
    }

    // Remove component from entity
    void Remove(Entity entity) override
    {
        if (!Has(entity))
            return;

        uint32_t denseIndex = m_sparse[entity.id];
        uint32_t lastIndex = static_cast<uint32_t>(m_dense.size() - 1);

        if (denseIndex != lastIndex)
        {
            // Swap with last element
            m_dense[denseIndex] = std::move(m_dense[lastIndex]);
            m_entities[denseIndex] = m_entities[lastIndex];

            // Update sparse array for swapped entity
            m_sparse[m_entities[denseIndex].id] = denseIndex;
        }

        // Remove last element
        m_dense.pop_back();
        m_entities.pop_back();
        m_sparse[entity.id] = INVALID_ENTITY_ID;
    }

    // Check if entity has this component
    bool Has(Entity entity) const override
    {
        if (!entity.IsValid() || entity.id >= m_sparse.size())
            return false;
        return m_sparse[entity.id] != INVALID_ENTITY_ID;
    }

    // Clear all components
    void Clear() override
    {
        m_dense.clear();
        m_entities.clear();
        std::fill(m_sparse.begin(), m_sparse.end(), INVALID_ENTITY_ID);
    }

    // Iteration support - iterate over all components
    typename std::vector<T>::iterator begin() { return m_dense.begin(); }
    typename std::vector<T>::iterator end() { return m_dense.end(); }
    typename std::vector<T>::const_iterator begin() const { return m_dense.begin(); }
    typename std::vector<T>::const_iterator end() const { return m_dense.end(); }

    // Get entity at dense index (for iterating with entity access)
    Entity GetEntity(size_t denseIndex) const
    {
        return m_entities[denseIndex];
    }

    // Get all entities with this component
    const std::vector<Entity>& GetEntities() const { return m_entities; }

    // Get component count
    size_t Size() const { return m_dense.size(); }

private:
    std::vector<T> m_dense;              // Actual component data (contiguous)
    std::vector<uint32_t> m_sparse;      // Entity ID -> dense index
    std::vector<Entity> m_entities;      // dense index -> Entity
};
