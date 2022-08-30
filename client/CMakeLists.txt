find_package(Threads)
find_package(absl REQUIRED)

add_executable(path_oram_client path_oram_client.cc path_oram_main.cc)
add_executable(linear_oram_client linear_oram_client.cc linear_oram_main.cc)
add_executable(ods_client ods_client.cc ods_client_main.cc)

target_include_directories(path_oram_client PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(linear_oram_client PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(ods_client PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(path_oram_client PRIVATE messages spdlog base oram_controller absl::base absl::flags absl::flags_parse)
target_link_libraries(linear_oram_client PRIVATE messages spdlog base oram_controller absl::base absl::flags absl::flags_parse)
target_link_libraries(ods_client PRIVATE messages spdlog base oram_controller ods_controller absl::base absl::flags absl::flags_parse)