file(GLOB_RECURSE SC_SRC_FILES CONFIGURE_DEPENDS
    "../include/ShaderReflection.h"
    "../src/*.h"
    "../src/*.cpp"
)

add_executable(ShaderCompilerD3D12 ${SC_SRC_FILES})

target_include_directories(ShaderCompilerD3D12 PUBLIC
    "../../../external/slang/include"
)


set(SLANG_PATH ${EXTERNAL_PATH}/slang)

target_link_libraries(ShaderCompilerD3D12 PRIVATE
    "${SLANG_PATH}/lib/x86_64/slang.lib"
    "${SLANG_PATH}/lib/x86_64/slang-compiler.lib"
)

get_target_property(RB_SRC RabBit SOURCE_DIR)

target_compile_definitions(ShaderCompilerD3D12 PRIVATE
    RB_SHADER_COMPILER_D3D12
    RB_SHADER_SOURCE="${RB_SRC}/src/RabBit/graphics/shaders"
    RB_DEFINE_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/../include"
    RB_OUTPUT_FOLDER="$<TARGET_FILE_DIR:Sample>"
)

add_custom_command(TARGET ShaderCompilerD3D12 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SLANG_PATH}/bin/x86_64/slang.dll
        ${SLANG_PATH}/bin/x86_64/slang-compiler.dll
        $<TARGET_FILE_DIR:ShaderCompilerD3D12>)