cmake_minimum_required(VERSION 3.10.2)

project("riru")

add_library(riru STATIC riru.c)
include_directories(include)
target_link_libraries(riru)