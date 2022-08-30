find_package(Threads)
find_package(absl REQUIRED)

add_library(oram_controller SHARED
  oram_controller.cc
  path_oram_controller.cc
  partition_oram_controller.cc
  linear_oram_controller.cc
  square_root_oram_controller.cc
)

add_library(ods_controller SHARED odict_controller.cc ods_cache.cc)

target_include_directories(oram_controller PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(oram_controller PRIVATE messages spdlog base absl::base absl::flags absl::flags_parse)
target_include_directories(ods_controller PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ods_controller PRIVATE messages spdlog base absl::base absl::flags absl::flags_parse)