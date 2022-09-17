find_package(Threads)
# The parser does not depend on libabsel.

add_library(oram_parser SHARED oram_parser.cc)

target_include_directories(oram_parser PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(oram_parser PRIVATE messages spdlog oram_base yaml-cpp)
set_target_properties(oram_parser PROPERTIES VERSION ${ORAM_VERSION_STRING})