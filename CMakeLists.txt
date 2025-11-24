cmake_minimum_required(VERSION 3.20)
project(dx_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_executable(dx_engine
    src/main.cpp
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
