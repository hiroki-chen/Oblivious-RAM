find_package(Threads)
find_package(absl REQUIRED)

file(GLOB_RECURSE SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})

add_executable(path_oram_client ${SRC_FILES})

target_include_directories(path_oram_client PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(path_oram_client PRIVATE messages spdlog base controller absl::base absl::flags absl::flags_parse)