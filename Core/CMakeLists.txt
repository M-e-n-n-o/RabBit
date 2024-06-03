cmake_minimum_required(VERSION 3.16.3)

project(RabBitEngine VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

#options
option(RABBIT_BUILD_TESTS "Build tests" ON)

file(GLOB_RECURSE RABBIT_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
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

    # D3D12
    "external/D3D12/include"

    # WinPixEventRuntime
    "external/WinPixEventRuntime/Include/WinPixEventRuntime"

    # DXC
    "external/DXC/inc"
)

set(EXTERNAL_PATH "${PROJECT_SOURCE_DIR}/external")

# Library paths
set(ENV{D3D_PATH} "${EXTERNAL_PATH}/D3D12")
set(ENV{PIX_PATH} "${EXTERNAL_PATH}/WinPixEventRuntime/bin/x64")
set(ENV{DXC_PATH} "${EXTERNAL_PATH}/DXC")

target_link_libraries(RabBit
    # D3D12
    d3d12.lib
    dxgi.lib
    dxguid.lib

    # D3D11 (D3D11On12)
    d3d11.lib
    dcomp.lib

    # WinPixEventRuntime
    "$ENV{PIX_PATH}/WinPixEventRuntime.lib"

    # DXC 
    "$ENV{DXC_PATH}/lib/x64/dxcompiler.lib"
)

# Copy libraries next to the final executable (should be called app side)
function(copyExternalLibraries)

    # Create a lit of files to copy to the executable
    list (APPEND DLLS_TO_COPY
        "$ENV{PIX_PATH}/WinPixEventRuntime.dll"
        "$ENV{DXC_PATH}/bin/x64/dxcompiler.dll"
    )

    foreach (file_i ${DLLS_TO_COPY})
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${file_i}"
            $<TARGET_FILE_DIR:${APP_NAME}>)
    endforeach(file_i)

    # Copy the D3D12 DLL's to a specific folder
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$ENV{D3D_PATH}/bin/x64/D3D12Core.dll"
        "$<TARGET_FILE_DIR:${APP_NAME}>/D3D12/D3D12Core.dll")
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$ENV{D3D_PATH}/bin/x64/d3d12SDKLayers.dll"
        "$<TARGET_FILE_DIR:${APP_NAME}>/D3D12/d3d12SDKLayers.dll")
endfunction()

# Add shader compiler
add_subdirectory(shaderCompiler/d3d12)

# Add tests
if(RABBIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/GoogleTest)
    add_subdirectory(tests)
endif()