cmake_minimum_required(VERSION 3.16.3)

project(RabBitSolution)
set(CMAKE_CXX_STANDARD 17)

# Detect platform
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(PLATFORM_WINDOWS ON)
    add_compile_definitions(RB_PLATFORM_WINDOWS)
endif()

# Add main projects
add_subdirectory(Core)
add_subdirectory(Sample)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)