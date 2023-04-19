file(GLOB_RECURSE TEST_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

include(GoogleTest)
add_executable(Test ${TEST_SRC_FILES})

# Link RabBit & GoogleTest
target_include_directories(Test PUBLIC 
    "../src" 
    "../src/RabBit"
)
target_link_directories(Test PUBLIC 
    RabBit
)
target_link_libraries(Test PRIVATE
    RabBit
    gtest
)

gtest_discover_tests(Test
    DISCOVERY_TIMEOUT
        240
)

# Add post build command to copy RabBit library to build location
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_custom_command(TARGET Test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:RabBit>/RabBit.lib"
            $<TARGET_FILE_DIR:Test>)
endif()