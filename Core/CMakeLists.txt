cmake_minimum_required(VERSION 3.16.3)

project(RabBitEngine VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

#options
option(RABBIT_BUILD_TESTS "Build tests" ON)

file(GLOB_RECURSE RABBIT_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"

    # D3D12
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

    # WinPixEventRuntime
    "external/WinPixEventRuntime/Include/WinPixEventRuntime"
)

# Link windows specific libraries
if(CMAKE_SYSTEM_NAME MATCHES "Windows")

    set(EXTERNAL_PATH "${PROJECT_SOURCE_DIR}/external")

    # Library paths
    set(ENV{PIX_PATH} "${EXTERNAL_PATH}/WinPixEventRuntime/bin/x64")

    target_link_libraries(RabBit
        # D3D12
        d3d12.lib
        dxgi.lib
        dxguid.lib

        # D3D11
        d3d11.lib
        dcomp.lib

        # WinPixEventRuntime
        "$ENV{PIX_PATH}/WinPixEventRuntime.lib"
    )

    # Copy libraries next to the final executable (should be called app side)
    function(copyExternalLibraries)
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different

            "$ENV{PIX_PATH}/WinPixEventRuntime.dll"

            $<TARGET_FILE_DIR:${APP_NAME}>)
    endfunction()
endif()

if(RABBIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/GoogleTest)
    add_subdirectory(tests)
endif()