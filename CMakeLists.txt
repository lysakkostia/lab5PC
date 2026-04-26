cmake_minimum_required(VERSION 3.10)
project(HttpServerLab5)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(HttpServer main.cpp)
target_link_libraries(HttpServer ws2_32)
