cmake_minimum_required(VERSION 3.21)
project(Nereids)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

aux_source_directory(src SRC_LIST)

add_executable(Nereids ${SRC_LIST} src/timer/lst_timer.cpp src/timer/lst_timer.h src/http/http_conn.cpp src/http/http_conn.h src/log/log.cpp src/log/log.h src/lock/block_queue.h src/lock/locker.h)
