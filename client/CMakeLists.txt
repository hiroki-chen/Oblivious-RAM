find_package(Threads)
find_package(absl REQUIRED)

file(GLOB_RECURSE SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})

add_executable(client ${SRC_FILES})
add_library(oram_controller SHARED oram_controller.cc )
add_library(ods_controller SHARED ods_controller.cc ods_cache.cc)

target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(client PRIVATE messages spdlog base absl::base absl::flags absl::flags_parse)

target_include_directories(oram_controller PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(oram_controller PRIVATE messages spdlog base absl::base absl::flags absl::flags_parse)
target_include_directories(ods_controller PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ods_controller PRIVATE messages spdlog base absl::base absl::flags absl::flags_parse)