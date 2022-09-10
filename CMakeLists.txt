cmake_minimum_required(VERSION 3.21)
project(Nereids)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

aux_source_directory(src SRC_LIST)

aux_source_directory(src/lock SRC_LIST)

aux_source_directory(src/log SRC_LIST)

aux_source_directory(src/timer SRC_LIST)

add_executable(Nereids ${SRC_LIST})
