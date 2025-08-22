# Define app name
set(APP_NAME Sample)

file(GLOB_RECURSE SAMPLE_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

add_executable(${APP_NAME} ${SAMPLE_SRC_FILES})

# Link RabBit
target_include_directories(${APP_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/Core/src"
    "../Core/external"
)
target_link_directories(${APP_NAME} PUBLIC RabBit)
target_link_libraries(${APP_NAME} RabBit)

# Add post build command to copy RabBit libraries to build location
RabBitCopyLibs()