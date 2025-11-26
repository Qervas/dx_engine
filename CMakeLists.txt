cmake_minimum_required(VERSION 3.20)
project(dx_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_executable(dx_engine
    src/main.cpp

    # Core
    src/Core/Types.h
    src/Core/Timer.h

    # RHI (Render Hardware Interface)
    src/RHI/Device.h
    src/RHI/Device.cpp
    src/RHI/CommandQueue.h
    src/RHI/CommandQueue.cpp
    src/RHI/CommandList.h
    src/RHI/CommandList.cpp
    src/RHI/SwapChain.h
    src/RHI/SwapChain.cpp
    src/RHI/Resource.h
    src/RHI/Resource.cpp
    src/RHI/Texture.h
    src/RHI/Texture.cpp
    src/RHI/Buffer.h
    src/RHI/Buffer.cpp
    src/RHI/DescriptorHeap.h
    src/RHI/DescriptorHeap.cpp
    src/RHI/UploadBuffer.h
    src/RHI/UploadBuffer.cpp
    src/RHI/Shader.h
    src/RHI/Shader.cpp
    src/RHI/RootSignature.h
    src/RHI/RootSignature.cpp
    src/RHI/PipelineState.h
    src/RHI/PipelineState.cpp

    # Renderer
    src/Renderer/Camera.h
    src/Renderer/Camera.cpp
    src/Renderer/Mesh.h
    src/Renderer/Mesh.cpp
    src/Renderer/TextureLoader.h
    src/Renderer/TextureLoader.cpp
    src/Renderer/ProceduralTexture.h
    src/Renderer/ProceduralTexture.cpp
    src/Renderer/Material.h
    src/Renderer/Material.cpp
    src/Renderer/Light.h
    src/Renderer/SceneRenderer.h
    src/Renderer/SceneRenderer.cpp
    src/Renderer/ShadowMap.h
    src/Renderer/ShadowMap.cpp
    src/Renderer/DebugRenderer.h
    src/Renderer/DebugRenderer.cpp

    # Scene
    src/Scene/Entity.h
    src/Scene/MeshRenderer.h
    src/Scene/Component.h
    src/Scene/Scene.h
    src/Scene/Scene.cpp
    src/Scene/Transform.h
    src/Scene/Transform.cpp

    # Render Graph
    src/RenderGraph/RenderPass.h
    src/RenderGraph/RenderPass.cpp
    src/RenderGraph/RenderGraph.h
    src/RenderGraph/RenderGraph.cpp
    src/RenderGraph/ShadowPass.h
    src/RenderGraph/ShadowPass.cpp
    src/RenderGraph/MainPass.h
    src/RenderGraph/MainPass.cpp
    src/RenderGraph/DebugPass.h
    src/RenderGraph/DebugPass.cpp

    # Platform
    src/Platform/Window.h
    src/Platform/Window.cpp
    src/Platform/Input.h
    src/Platform/Input.cpp
)

# Find packages
find_package(Stb REQUIRED)

# Define UNICODE for wide character support
target_compile_definitions(dx_engine PRIVATE UNICODE _UNICODE)

# Include directories
target_include_directories(dx_engine PRIVATE ${Stb_INCLUDE_DIR})

# Link DirectX 12 libraries
target_link_libraries(dx_engine
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

# Set subsystem to Windows
if(MSVC)
    set_target_properties(dx_engine PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# Copy shaders to build directory
add_custom_command(TARGET dx_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:dx_engine>/shaders
    COMMENT "Copying shaders to build directory"
)
