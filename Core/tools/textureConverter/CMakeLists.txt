project(TextureConverter VERSION 0.0.1)

file(GLOB_RECURSE TC_SRC_FILES CONFIGURE_DEPENDS
    "include/*.h"
    "src/*.h"
    "src/*.cpp"
)

add_executable(TextureConverter ${TC_SRC_FILES})


target_include_directories(TextureConverter PRIVATE 
    "include"
    "../utils"
    "../../external/stb"
    "../../external/libsquish/lib/include"
)

target_link_libraries(TextureConverter PRIVATE
    "libsquish::Squish"
)