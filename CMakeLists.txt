cmake_minimum_required(VERSION 3.16.3)

project(RabBitSolution)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Used for VS extensions
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set RabBit configuration definitions
add_compile_definitions(
    $<$<CONFIG:Debug>:RB_CONFIG_DEBUG>
    $<$<CONFIG:Release>:RB_CONFIG_OPTIMIZED>
    $<$<CONFIG:RelWithDebInfo>:RB_CONFIG_OPTIMIZED>
    $<$<CONFIG:MinSizeRel>:RB_CONFIG_DIST>
)

# Add main projects
add_subdirectory(Core)
add_subdirectory(Editor)
add_subdirectory(Sample)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)