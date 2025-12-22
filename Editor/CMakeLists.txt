file(GLOB_RECURSE SAMPLE_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

add_executable(Editor ${SAMPLE_SRC_FILES})

# Link RabBits
target_include_directories(Editor PUBLIC
    "${PROJECT_SOURCE_DIR}/Core/src"
    "../Core/external"

    # Own dependencies
    "src"
    "external/imgui/imgui"
)

target_link_directories(Editor PRIVATE 
    RabBit

    # Own dependencies
    "external/imgui/imgui"
)

target_link_libraries(Editor PRIVATE 
    RabBit

    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/lib/x64/Release/ImGui.lib"
)

# Add post build command to copy RabBit libraries to build location
RabBitCopyLibs()