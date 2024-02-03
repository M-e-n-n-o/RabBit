file(GLOB_RECURSE TEST_SRC_FILES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.cpp"
)

include(GoogleTest)
add_executable(EngineTest ${TEST_SRC_FILES})

# Link RabBit & GoogleTest
target_include_directories(EngineTest PUBLIC 
    "../src" 
    "../src/RabBit"
)
target_link_directories(EngineTest PUBLIC 
    RabBit
)
target_link_libraries(EngineTest PRIVATE
    RabBit
    gtest
)

gtest_discover_tests(EngineTest
    DISCOVERY_TIMEOUT
        240
)

# Add post build command to copy RabBit library to build location
add_custom_command(TARGET EngineTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:RabBit>/RabBit.lib"
        $<TARGET_FILE_DIR:EngineTest>)