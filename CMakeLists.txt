cmake_minimum_required(VERSION 3.21)
project(Nereids)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

aux_source_directory(src SRC_LIST)

aux_source_directory(src/lock SRC_LIST)

aux_source_directory(src/log SRC_LIST)

aux_source_directory(src/timer SRC_LIST)

aux_source_directory(src/mysql SRC_LIST)

aux_source_directory(src/threadpool SRC_LIST)

aux_source_directory(src/http SRC_LIST)

add_executable(Nereids ${SRC_LIST})

target_link_libraries(Nereids mysqlclient)

target_link_libraries(Nereids pthread)

target_link_libraries(Nereids jsoncpp)