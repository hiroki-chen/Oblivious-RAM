find_package(Threads)
find_package(absl REQUIRED)
# The parser does not depend on libabsel.

add_library(oram_parse SHARED oram_parse.cc)

target_include_directories(oram_parse PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(oram_parse PRIVATE messages spdlog oram_base yaml-cpp absl::base absl::flags absl::flags_parse)
set_target_properties(oram_parse PROPERTIES VERSION ${ORAM_VERSION_STRING})