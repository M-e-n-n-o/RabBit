file(GLOB_RECURSE SC_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

add_executable(ShaderCompilerD3D12 ${SC_SRC_FILES})

target_include_directories(ShaderCompilerD3D12 PUBLIC 
    "../external/DXC/inc"
)

set(DXC_PATH ${EXTERNAL_PATH}/DXC)

target_link_libraries(ShaderCompilerD3D12 PRIVATE
    dxgi.lib
    "${DXC_PATH}/lib/x64/dxcompiler.lib"
)

get_target_property(RB_SRC RabBit SOURCE_DIR)

target_compile_definitions(ShaderCompilerD3D12 PRIVATE
    RB_SHADER_COMPILER
    RB_SHADER_SOURCE="${RB_SRC}/src/RabBit/graphics/d3d12/shaders"
)

add_custom_command(TARGET ShaderCompilerD3D12 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DXC_PATH}/bin/x64/dxcompiler.dll
        $<TARGET_FILE_DIR:ShaderCompilerD3D12>)

add_custom_command(TARGET ShaderCompilerD3D12 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DXC_PATH}/bin/x64/dxil.dll
        $<TARGET_FILE_DIR:ShaderCompilerD3D12>)