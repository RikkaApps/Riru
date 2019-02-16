cmake_minimum_required(VERSION 3.4.1)

add_library(helper SHARED
            helper.cpp pmparser.c)

target_link_libraries(helper
                      log)