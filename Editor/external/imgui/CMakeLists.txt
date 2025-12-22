cmake_minimum_required(VERSION 3.16.3)

project(ImGuiSolution)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ImGui STATIC
    "imgui/imconfig.h"
    "imgui/imgui.cpp"
    "imgui/imgui.h"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_internal.h"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imstb_rectpack.h"
    "imgui/imstb_textedit.h"
    "imgui/imstb_truetype.h"

    # Backends
    "imgui/backends/imgui_impl_win32.h"
    "imgui/backends/imgui_impl_win32.cpp"
    "imgui/backends/imgui_impl_dx12.h"
    "imgui/backends/imgui_impl_dx12.cpp"
)

target_include_directories(ImGui PUBLIC
    "imgui"
    "imgui/backends"
)

set_target_properties(ImGui PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${CMAKE_GENERATOR_PLATFORM}/$<CONFIG>"
)