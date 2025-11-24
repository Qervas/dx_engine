#pragma once

#include <cstdint>
#include <memory>
#include <wrl/client.h>

// Common types
using uint = uint32_t;

// Microsoft WRL ComPtr for COM objects
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// Type-safe handles
struct TextureHandle { uint32_t index = 0; bool IsValid() const { return index != 0; } };
struct BufferHandle { uint32_t index = 0; bool IsValid() const { return index != 0; } };
struct MaterialHandle { uint32_t index = 0; bool IsValid() const { return index != 0; } };
struct MeshHandle { uint32_t index = 0; bool IsValid() const { return index != 0; } };

// Constants
constexpr uint32_t FRAME_COUNT = 2;  // Double buffering
