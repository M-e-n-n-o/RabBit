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
    _CRT_SECURE_NO_WARNINGS

    RB_VERSION="${CMAKE_PROJECT_VERSION}"

    $<$<CONFIG:Debug>:RB_CONFIG_DEBUG>
    $<$<CONFIG:Release>:RB_CONFIG_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:RB_CONFIG_RELEASE>
    $<$<CONFIG:MinSizeRel>:RB_CONFIG_DIST>
)

# Give RabBit core access to itself
target_compile_definitions(RabBit PRIVATE RB_CORE_ACCESS)

# Set precompiled header
target_precompile_headers(RabBit PRIVATE "src/RabBit/RabBitPch.h")

target_include_directories(RabBit PUBLIC
    "src"
    "src/RabBit"
    "external"
)

# Link windows specific libraries
# if(CMAKE_SYSTEM_NAME MATCHES "Windows")
#     find_library(D3D12 d3d12)
#     find_library(DXGI dxgi)
#     find_library(DXGUID dxguid)

#     # Link external libraries
#     target_link_libraries(RabBit PUBLIC
#         ${D3D12}
#         ${DXGI}
#         ${DXGUID}
#     )
# endif()

if(RABBIT_BUILD_TESTS)
    add_subdirectory(tests)
endif()