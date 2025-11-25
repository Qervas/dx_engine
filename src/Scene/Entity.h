#pragma once

#include <cstdint>
#include <functional>

// Invalid entity constant
constexpr uint32_t INVALID_ENTITY_ID = 0xFFFFFFFF;

// Entity is a lightweight handle - just an ID with generation for validity checking
struct Entity
{
    uint32_t id = INVALID_ENTITY_ID;
    uint32_t generation = 0;  // Incremented when entity is destroyed, detects stale handles

    Entity() = default;
    Entity(uint32_t id, uint32_t gen) : id(id), generation(gen) {}

    bool IsValid() const { return id != INVALID_ENTITY_ID; }

    bool operator==(const Entity& other) const
    {
        return id == other.id && generation == other.generation;
    }

    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

    // For use in unordered_map/set
    struct Hash
    {
        size_t operator()(const Entity& e) const
        {
            return std::hash<uint64_t>()((static_cast<uint64_t>(e.generation) << 32) | e.id);
        }
    };
};

// Null entity constant
const Entity NULL_ENTITY = Entity();
