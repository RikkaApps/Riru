cmake_minimum_required(VERSION 3.10.2)

project("riru-api")

add_definitions(-DRIRU_MAX_API_VERSION=${RIRU_MAX_API_VERSION})

add_library(riru-api STATIC riru.c)
include_directories(include)
target_link_libraries(riru-api)