find_package(absl REQUIRED)

add_library(oram_client SHARED oram_client.cc)

target_link_libraries(oram_client PRIVATE oram_base oram_parse oram_controller ods_controller)

set_target_properties(oram_client PROPERTIES VERSION ${ORAM_VERSION_STRING})
