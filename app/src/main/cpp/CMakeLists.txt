cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--hash-style=both")

add_library(helper SHARED
            helper.cpp pmparser.c)

target_link_libraries(helper
                      log)