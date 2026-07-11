project(RabBitEngine VERSION 0.0.1)

# Platform detection
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_compile_definitions(RB_PLATFORM_WINDOWS)

    set(RABBIT_API_DX12 ON CACHE BOOL "Enable DX12")
    set(RABBIT_API_VULKAN ON CACHE BOOL "Enable Vulkan")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(RB_PLATFORM_LINUX_ES)

    set(RABBIT_API_VULKAN ON CACHE BOOL "Enable Vulkan")

    # Adds Microsoft extentions to the compiler (necessary for supporting anonymous structs in unions)
    # Does not work with the default Raspberry Pi GCC compiler, so use clang on this platform!
    add_compile_options(-fms-extensions)
else()
    message(FATAL_ERROR "Unsupported platform!")
    add_compile_definitions((RB_PLATFORM_UNKNOWN))
endif()

set(RABBIT_BUILD_TESTS ON CACHE BOOL "Build Unit Tests")

# Source files
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

target_include_directories(RabBit PRIVATE
    # Custom tools
    "tools/utils"
    "tools/shaderCompiler/include"
    "tools/textureConverter/include"

    # STB
    "external/stb"
    
    # UFBX
    "external/ufbx"

    # FreeType
    "external/freetype/include"
)

target_link_libraries(RabBit PRIVATE
    # FreeType
    "${EXTERNAL_PATH}/freetype/lib/x64/freetype.lib"
)

# DX12 specific includes and libraries
if(RABBIT_API_DX12)
    add_compile_definitions(RB_GRAPHICS_API_D3D12)

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
    add_subdirectory(tools/shaderCompiler/d3d12)
endif()

# Vulkan specific includes and libraries
if(RABBIT_API_VULKAN)
    add_compile_definitions(RB_GRAPHICS_API_VULKAN)

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
    add_subdirectory(tools/shaderCompiler/vulkan)
endif()

# Texture converter
add_subdirectory(external/libsquish)
add_subdirectory(tools/textureConverter)

# Copy libraries next to the final executable (should be called app side)
function(RabBitCopyLibs TARGET_NAME)
    # Create a lit of files to copy to the executable
    set(DLLS_TO_COPY "")

    if(RABBIT_API_DX12)
        list (APPEND DLLS_TO_COPY
            "$ENV{PIX_PATH}/WinPixEventRuntime.dll"
        )

        # Copy the D3D12 DLL's to a specific folder
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$ENV{D3D_PATH}/bin/x64/D3D12Core.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12/D3D12Core.dll")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$ENV{D3D_PATH}/bin/x64/d3d12SDKLayers.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12/d3d12SDKLayers.dll")
    endif()

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLLS_TO_COPY}
        $<TARGET_FILE_DIR:${TARGET_NAME}>)
endfunction()


# Add tests
if(RABBIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/GoogleTest)
    add_subdirectory(tests)
endif()