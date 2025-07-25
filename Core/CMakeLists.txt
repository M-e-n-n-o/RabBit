cmake_minimum_required(VERSION 3.16.3)

project(RabBitEngine VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

# Platform detection
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_compile_definitions(
        RB_PLATFORM_WINDOWS

        # This does not directly choose the graphics API, but is used to compile the API specific code
        RB_GRAPHICS_API_D3D12
        RB_GRAPHICS_API_OPENGL
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(
        RB_PLATFORM_LINUX_ES

        RB_GRAPHICS_API_OPENGL_ES
    )

    # Adds Microsoft extentions to the compiler (necessary for supporting anonymous structs in unions)
    # Does not work with the default Raspberry Pi GCC compiler, so use clang on this platform!
    add_compile_options(-fms-extensions)
else()
    message(FATAL_ERROR "Unsupported platform!")
    add_compile_definitions((RB_PLATFORM_UNKNOWN))
endif()

#options
option(RABBIT_BUILD_TESTS "Build tests" ON)

file(GLOB_RECURSE RABBIT_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

# Disable PCH for c files (libraries)
# UFBX
set_source_files_properties("external/ufbx/ufbx.c" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
list(APPEND RABBIT_SRC_FILES "external/ufbx/ufbx.c")

add_library(RabBit STATIC ${RABBIT_SRC_FILES})

# Add standard preprocessor definitions
add_compile_definitions(
    RB_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
    RB_VERSION_MINOR="${PROJECT_VERSION_MINOR}"
    RB_VERSION_PATCH="${PROJECT_VERSION_PATCH}"
)

# Give RabBit core access to itself
target_compile_definitions(RabBit PRIVATE RB_CORE_ACCESS)

target_include_directories(RabBit PUBLIC
    "src"
    "src/rabbit"
    "external"
)

# Set precompiled header
target_precompile_headers(RabBit PRIVATE "src/rabbit/RabBitCommon.h")

set(EXTERNAL_PATH "${PROJECT_SOURCE_DIR}/external")

target_include_directories(RabBit PUBLIC
    # STB
    "external/stb"
    
    # UFBX
    "external/ufbx"
)

# Windows specific includes and libraries
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_include_directories(RabBit PUBLIC
        # D3D12
        "external/D3D12/include"
        
        # WinPixEventRuntime
        "external/WinPixEventRuntime/Include/WinPixEventRuntime"
        
        # DXC
        "external/DXC/inc"
    )

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
    
    # Add D3D12 shader compiler
    add_subdirectory(shaderCompiler/d3d12)
endif()


# Copy libraries next to the final executable (should be called app side)
function(RabBitCopyLibs)
    # Create a lit of files to copy to the executable
    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        list (APPEND DLLS_TO_COPY
            "$ENV{PIX_PATH}/WinPixEventRuntime.dll"
            "$ENV{DXC_PATH}/bin/x64/dxcompiler.dll"
        )

        # Copy the D3D12 DLL's to a specific folder
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$ENV{D3D_PATH}/bin/x64/D3D12Core.dll"
            "$<TARGET_FILE_DIR:${APP_NAME}>/D3D12/D3D12Core.dll")
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$ENV{D3D_PATH}/bin/x64/d3d12SDKLayers.dll"
            "$<TARGET_FILE_DIR:${APP_NAME}>/D3D12/d3d12SDKLayers.dll")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list (APPEND DLLS_TO_COPY
        )
    endif()

    foreach (file_i ${DLLS_TO_COPY})
        if("${file_i}" STREQUAL "")
            continue()
        endif()

        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${file_i}"
            $<TARGET_FILE_DIR:${APP_NAME}>)
    endforeach(file_i)
endfunction()


# Add tests
if(RABBIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/GoogleTest)
    add_subdirectory(tests)
endif()