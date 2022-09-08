file(GLOB_RECURSE SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cc)

set(CMAKE_CXX_STANDARD 17)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})
add_library(base SHARED ${SRC_FILES})
target_link_libraries(base PRIVATE sodium lz4 fpe)
set_target_properties(base PROPERTIES VERSION ${ORAM_VERSION_STRING})
