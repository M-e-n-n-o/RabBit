enable_testing()

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(googletest)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

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
target_link_libraries(Test 
    RabBit
    GTest::GTest
)

add_test(NAME Test COMMAND Test)

# Add post build command to copy RabBit library to build location
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_custom_command(TARGET Test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:RabBit>/RabBit.lib"
            $<TARGET_FILE_DIR:Test>)
endif()