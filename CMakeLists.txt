cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(ORAM_VERSION_MAJOR 1)
set(ORAM_VERSION_MINOR 2)
set(ORAM_VERSION_PATCH 0)
set(ORAM_VERSION_STRING ${ORAM_VERSION_MAJOR}.${ORAM_VERSION_MINOR}.${ORAM_VERSION_PATCH})

project(Oblivious-RAM LANGUAGES CXX VERSION ${ORAM_VERSION_STRING})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-unused-function -Wno-unused-variable")

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(core)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(protos)
add_subdirectory(base)
add_subdirectory(parse)
add_subdirectory(tests)