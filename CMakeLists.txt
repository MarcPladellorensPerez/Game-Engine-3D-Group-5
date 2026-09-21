cmake_minimum_required(VERSION 3.10)

project(HelloWorld CXX)

find_package(SDL3 CONFIG REQUIRED)

add_executable(HelloWorld Main.cpp)

target_link_libraries(HelloWorld PRIVATE SDL3::SDL3)
