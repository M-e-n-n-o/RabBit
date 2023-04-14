file(GLOB_RECURSE TEST_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

add_executable(Test ${TEST_SRC_FILES})

# Link RabBit
target_include_directories(Test PUBLIC 
    "../src" 
    "../src/RabBit"
)
target_link_directories(Test PUBLIC RabBit)
target_link_libraries(Test RabBit)

enable_testing()
add_test(NAME Test COMMAND Test)

# Add post build command to copy RabBit library to build location
add_custom_command(TARGET Test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:RabBit>/RabBit.dll"
        $<TARGET_FILE_DIR:Test>)