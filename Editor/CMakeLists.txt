project(RabBitEditor VERSION 0.0.1)

file(GLOB_RECURSE EDITOR_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"

    # ImGui files
    # (Just directly include imgui into project for easy debugging)
    "external/imgui/imgui/imconfig.h"
    "external/imgui/imgui/imgui.cpp"
    "external/imgui/imgui/imgui.h"
    "external/imgui/imgui/imgui_demo.cpp"
    "external/imgui/imgui/imgui_draw.cpp"
    "external/imgui/imgui/imgui_internal.h"
    "external/imgui/imgui/imgui_tables.cpp"
    "external/imgui/imgui/imgui_widgets.cpp"
    "external/imgui/imgui/imstb_rectpack.h"
    "external/imgui/imgui/imstb_textedit.h"
    "external/imgui/imgui/imstb_truetype.h"
    # Backends
    "external/imgui/imgui/backends/imgui_impl_win32.h"
    "external/imgui/imgui/backends/imgui_impl_win32.cpp"
    "external/imgui/imgui/backends/imgui_impl_dx12.h"
    "external/imgui/imgui/backends/imgui_impl_dx12.cpp"
)

add_executable(Editor ${EDITOR_SRC_FILES})
set_target_properties(Editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Editor>")

add_compile_definitions(
    IMGUI_USER_CONFIG="ImGuiConfig.h"
)

target_include_directories(Editor PRIVATE
    "${PROJECT_SOURCE_DIR}/Core/src"

    # D3D12
    "${PROJECT_SOURCE_DIR}/Core/external/D3D12/include"

    # Own dependencies
    "src"

    # ImGui
    "external/imgui"
    "external/imgui/imgui"
    "external/imgui/imgui/backends"

    # FFmpeg
    "external/ffmpeg/include"
)

target_link_directories(Editor PRIVATE 
    RabBit
)

target_link_libraries(Editor PRIVATE 
    RabBit

    # D3D12
    d3d12.lib
    dxgi.lib
    dxguid.lib

    # FFmpeg
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/lib/avcodec.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/lib/avformat.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/lib/avutil.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/lib/swscale.lib"
)

set(FFMPEG_DLLS "")
list (APPEND FFMPEG_DLLS
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/bin/avcodec-62.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/bin/avformat-62.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/bin/avutil-60.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/bin/swscale-9.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/bin/swresample-6.dll"
)

add_custom_command(TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${FFMPEG_DLLS}
    $<TARGET_FILE_DIR:Editor>)

# Add post build command to copy RabBit libraries to build location
RabBitCopyLibs(Editor)