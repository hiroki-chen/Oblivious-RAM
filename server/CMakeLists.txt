find_package(Threads)
find_package(absl REQUIRED)

file(GLOB_RECURSE SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})

add_executable(server ${SRC_FILES})
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(server PRIVATE messages gflags spdlog base absl::hash absl::flags absl::flags_parse)