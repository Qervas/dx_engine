cmake_minimum_required(VERSION 3.20)
project(dx_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_executable(dx_engine
    src/main.cpp

    # Core
    src/Core/Types.h

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

    # Platform
    src/Platform/Window.h
    src/Platform/Window.cpp
)

# Define UNICODE for wide character support
target_compile_definitions(dx_engine PRIVATE UNICODE _UNICODE)

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
