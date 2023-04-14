#define app name
set(APP_NAME Sample)

file(GLOB_RECURSE SAMPLE_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

if(PLATFORM_WINDOWS)
    add_executable(${APP_NAME} WIN32 ${SAMPLE_SRC_FILES})
endif()

# Link RabBit
target_include_directories(${APP_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/Core/src"
    "../Core/external"
)
target_link_directories(${APP_NAME} PUBLIC RabBit)
target_link_libraries(${APP_NAME} RabBit)

# Add post build command to copy RabBit library to build location
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:RabBit>/RabBit.dll"
        $<TARGET_FILE_DIR:${APP_NAME}>)