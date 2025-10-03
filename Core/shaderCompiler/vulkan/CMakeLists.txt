file(GLOB_RECURSE SC_SRC_FILES CONFIGURE_DEPENDS
    "../src/*.h"
    "../src/*.cpp"
)

add_executable(ShaderCompilerVK ${SC_SRC_FILES})

target_include_directories(ShaderCompilerVK PUBLIC 
    "../../external/DXC/inc"
)

set(DXC_PATH ${EXTERNAL_PATH}/DXC)

target_link_libraries(ShaderCompilerVK PRIVATE
    dxgi.lib
    "${DXC_PATH}/lib/x64/dxcompiler.lib"
)

get_target_property(RB_SRC RabBit SOURCE_DIR)

target_compile_definitions(ShaderCompilerVK PRIVATE
    RB_SHADER_COMPILER_VK
    RB_SHADER_SOURCE="${RB_SRC}/src/RabBit/graphics/shaders"
    RB_GRAPHICS_FOLDER="${RB_SRC}/src/RabBit/graphics"
    RB_API_GRAPHICS_FOLDER="${RB_SRC}/src/platform/graphics/vulkan"
    RB_OUTPUT_FOLDER="$<TARGET_FILE_DIR:Sample>"
)

add_custom_command(TARGET ShaderCompilerVK POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DXC_PATH}/bin/x64/dxcompiler.dll
        $<TARGET_FILE_DIR:ShaderCompilerVK>)

add_custom_command(TARGET ShaderCompilerVK POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DXC_PATH}/bin/x64/dxil.dll
        $<TARGET_FILE_DIR:ShaderCompilerVK>)