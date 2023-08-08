cmake_minimum_required(VERSION 3.16.3)

project(RabBitEngine VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

#options
option(RABBIT_BUILD_TESTS "Build tests" ON)

file(GLOB_RECURSE RABBIT_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
    "external/D3DX12/*.h"
)

add_library(RabBit STATIC ${RABBIT_SRC_FILES})

# Add standard preprocessor definitions
add_compile_definitions(
    RB_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
    RB_VERSION_MINOR="${PROJECT_VERSION_MINOR}"
    RB_VERSION_PATCH="${PROJECT_VERSION_PATCH}"
)

# Give RabBit core access to itself
target_compile_definitions(RabBit PRIVATE RB_CORE_ACCESS)

# Set precompiled header
target_precompile_headers(RabBit PRIVATE "src/RabBit/RabBitCommon.h")

target_include_directories(RabBit PUBLIC
    "src"
    "src/RabBit"
    "external"
)

# Link windows specific libraries
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(RabBit
        d3d12.lib
        dxgi.lib
        dxguid.lib
    )
endif()

if(RABBIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/GoogleTest)
    add_subdirectory(tests)
endif()