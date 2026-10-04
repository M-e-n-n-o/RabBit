project(RabBitEngine VERSION 0.0.1)

# Source files
file(GLOB_RECURSE RABBIT_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

add_library(RabBit STATIC ${RABBIT_SRC_FILES})

# Add standard preprocessor definitions
target_compile_definitions(RabBit PUBLIC
    $<$<CONFIG:Debug>:RB_CONFIG_DEBUG>
    $<$<CONFIG:Release>:RB_CONFIG_OPTIMIZED>
    $<$<CONFIG:RelWithDebInfo>:RB_CONFIG_OPTIMIZED>
    $<$<CONFIG:MinSizeRel>:RB_CONFIG_DIST>

    RB_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
    RB_VERSION_MINOR="${PROJECT_VERSION_MINOR}"
    RB_VERSION_PATCH="${PROJECT_VERSION_PATCH}"
)

# Give RabBit core access to itself
target_compile_definitions(RabBit PRIVATE RB_CORE_ACCESS)

target_include_directories(RabBit PUBLIC
    "src"
    "src/rabbit"
)

# Set precompiled header
target_precompile_headers(RabBit PRIVATE "src/rabbit/RabBitCommon.h")

# --------------------------------------------------------------------------
# Platform detection
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_compile_definitions(RabBit PUBLIC RB_PLATFORM_WINDOWS)

    set(RABBIT_API_DX12 ON CACHE BOOL "Enable DX12")
    set(RABBIT_API_VULKAN ON CACHE BOOL "Enable Vulkan")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(RabBit PUBLIC RB_PLATFORM_LINUX_ES)

    set(RABBIT_API_VULKAN ON CACHE BOOL "Enable Vulkan")

    # Adds Microsoft extentions to the compiler (necessary for supporting anonymous structs in unions)
    # Does not work with the default Raspberry Pi GCC compiler, so use clang on this platform!
    add_compile_options(-fms-extensions)
else()
    message(FATAL_ERROR "Unsupported platform!")
    target_compile_definitions(RabBit PUBLIC RB_PLATFORM_UNKNOWN)
endif()

set(RABBIT_BUILD_TESTS ON CACHE BOOL "Build Unit Tests")
set(RABBIT_STEAM ON CACHE BOOL "Enable Steam API")

# --------------------------------------------------------------------------
# Libraries

set(EXTERNAL_PATH "${PROJECT_SOURCE_DIR}/external")

target_include_directories(RabBit PRIVATE
    "external"

    # Custom tools
    "tools/utils"
    "tools/shaderCompiler/include"
    "tools/textureConverter/include"
    "tools/modelConverter/include"

    # STB
    "external/stb"

    # FreeType
    "external/freetype/include"
)

add_subdirectory(external/box2d)

target_link_libraries(RabBit PRIVATE
    # FreeType
    "${EXTERNAL_PATH}/freetype/lib/x64/freetype.lib"

    # Box2D
    box2d
)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(RabBit PRIVATE
        # Winsock
        ws2_32.lib
    )
endif()

if(RABBIT_STEAM)
    target_compile_definitions(RabBit PUBLIC RB_STEAM_API)

    set(ENV{STEAM_PATH} "${EXTERNAL_PATH}/steamworks/165")

    target_include_directories(RabBit PRIVATE
        "$ENV{STEAM_PATH}/public"
    )

    target_link_libraries(RabBit PRIVATE
        "$ENV{STEAM_PATH}/redistributable_bin/win64/steam_api64.lib"
    )
endif()

# DX12 specific includes and libraries
if(RABBIT_API_DX12)
    target_compile_definitions(RabBit PUBLIC RB_GRAPHICS_API_D3D12)

    target_include_directories(RabBit PRIVATE
        # D3D12
        "external/D3D12/include"
        
        # WinPixEventRuntime
        "external/WinPixEventRuntime/Include/WinPixEventRuntime"
    )

    # Library paths
    set(ENV{D3D_PATH} "${EXTERNAL_PATH}/D3D12")
    set(ENV{PIX_PATH} "${EXTERNAL_PATH}/WinPixEventRuntime/bin/x64")
    
    target_link_libraries(RabBit PRIVATE
        # D3D12
        d3d12.lib
        dxgi.lib
        dxguid.lib
    
        # D3D11 (D3D11On12)
        d3d11.lib
        dcomp.lib
    
        # WinPixEventRuntime
        "$ENV{PIX_PATH}/WinPixEventRuntime.lib"
    )
    
    # Add D3D12 shader compiler
    set(ENV{SHADER_D3D_OUT} "ShadersD3D12.bin")
    add_subdirectory(tools/shaderCompiler/d3d12)
endif()

# Vulkan specific includes and libraries
if(RABBIT_API_VULKAN)
    target_compile_definitions(RabBit PUBLIC RB_GRAPHICS_API_VULKAN)

    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_compile_definitions(VK_USE_PLATFORM_DISPLAY_KHR)
    endif()

    add_subdirectory(external/VulkanHeaders)

    find_package(Vulkan REQUIRED)

    target_link_libraries(RabBit PRIVATE
        Vulkan::Headers
        Vulkan::Vulkan
    )

    # Add VK shader compiler
    set(ENV{SHADER_VK_OUT} "ShadersVK.bin")
    add_subdirectory(tools/shaderCompiler/vulkan)
endif()

# --------------------------------------------------------------------------
# Tools

# Texture converter
add_subdirectory(external/libsquish)
add_subdirectory(tools/textureConverter)

# Model converter
add_subdirectory(tools/modelConverter)

# --------------------------------------------------------------------------
# Copy libraries next to the final executable (should be called app side)
function(RabBitCopyLibs TARGET_NAME)
    set(DEST "$<TARGET_FILE_DIR:${TARGET_NAME}>")

    if(RABBIT_STEAM)
        list(APPEND FILES "$ENV{STEAM_PATH}/redistributable_bin/win64/steam_api64.dll")
    endif()

    if(RABBIT_API_DX12)
        list(APPEND FILES "$ENV{PIX_PATH}/WinPixEventRuntime.dll")
        
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST}/D3D12"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$ENV{D3D_PATH}/bin/x64/D3D12Core.dll" "$ENV{D3D_PATH}/bin/x64/d3d12SDKLayers.dll" "${DEST}/D3D12")
    endif()

    if(RABBIT_API_DX12)
        list(APPEND FILES "$<$<TARGET_EXISTS:ShaderCompilerD3D12>:$<TARGET_FILE_DIR:ShaderCompilerD3D12>/$ENV{SHADER_D3D_OUT}>")
    endif()
    if(RABBIT_API_VULKAN)
        list(APPEND FILES "$<$<TARGET_EXISTS:ShaderCompilerVK>:$<TARGET_FILE_DIR:ShaderCompilerVK>/$ENV{SHADER_VK_OUT}>")
    endif()

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FILES} "${DEST}"
        COMMAND_EXPAND_LISTS)
endfunction()

# --------------------------------------------------------------------------
# Tests
if(RABBIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/GoogleTest)
    add_subdirectory(tests)
endif()