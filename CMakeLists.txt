cmake_minimum_required(VERSION 3.16.3)

project(RabBitSolution)
set(CMAKE_CXX_STANDARD 17)

# Link global libraries
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    link_libraries(
        Kernel32.lib
    )
endif()

# Set RabBit configuration definitions
add_compile_definitions(
    $<$<CONFIG:Debug>:RB_CONFIG_DEBUG>
    $<$<CONFIG:Release>:RB_CONFIG_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:RB_CONFIG_RELEASE>
    $<$<CONFIG:MinSizeRel>:RB_CONFIG_FINAL>
)

# Set RabBit options
set(RABBIT_BUILD_TESTS ON)

# Add main projects
add_subdirectory(Core)
add_subdirectory(Sample)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)