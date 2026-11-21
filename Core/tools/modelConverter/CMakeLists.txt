project(ModelConverter VERSION 0.0.1)

file(GLOB_RECURSE MC_SRC_FILES CONFIGURE_DEPENDS
    "include/*.h"
    "src/*.h"
    "src/*.cpp"
)

set(UFBX_DIR "../../external/ufbx")

add_executable(ModelConverter ${MC_SRC_FILES} "${UFBX_DIR}/ufbx.c")

target_include_directories(ModelConverter PRIVATE
    "include"
    "src"
    "../utils"
    "${UFBX_DIR}"
)
