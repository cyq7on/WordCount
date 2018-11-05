cmake_minimum_required(VERSION 3.12)
project(WordCount C)

set(CMAKE_C_STANDARD 99)

add_executable(WordCount main.c)

find_package(Threads REQUIRED)
target_link_libraries(WordCount Threads::Threads)