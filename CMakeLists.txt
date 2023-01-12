cmake_minimum_required(VERSION 3.21)
project(Narcissus)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

aux_source_directory(src SRC_LIST)

aux_source_directory(src/lock SRC_LIST)

aux_source_directory(src/log SRC_LIST)

aux_source_directory(src/timer SRC_LIST)

aux_source_directory(src/timer/set SRC_LIST) #set timer

aux_source_directory(src/timer/heap SRC_LIST) #heap timer

aux_source_directory(src/mysql SRC_LIST)

aux_source_directory(src/threadpool SRC_LIST)

aux_source_directory(src/http SRC_LIST)

add_executable(Narcissus ${SRC_LIST})

target_link_libraries(Narcissus mysqlclient)

target_link_libraries(Narcissus pthread)

target_link_libraries(Narcissus jsoncpp)