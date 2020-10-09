cmake_minimum_required(VERSION 3.4.1)

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)

if (NOT DEFINED RIRU_VERSION_NAME)
    message(FATAL_ERROR "RIRU_VERSION_NAME is not set")
endif ()

if (NOT DEFINED RIRU_VERSION_CODE)
    message(FATAL_ERROR "RIRU_VERSION_CODE is not set")
endif ()

if (NOT DEFINED RIRU_API_VERSION)
    message(FATAL_ERROR "RIRU_API_VERSION is not set")
endif ()

add_definitions(-DRIRU_VERSION_NAME="${RIRU_VERSION_NAME}")
add_definitions(-DRIRU_VERSION_CODE=${RIRU_VERSION_CODE})
add_definitions(-DRIRU_API_VERSION=${RIRU_API_VERSION})

set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(C_FLAGS "-Werror=format -fdata-sections -ffunction-sections")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(C_FLAGS "${C_FLAGS} -O2 -fvisibility=hidden -fvisibility-inlines-hidden")
    set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-exclude-libs,ALL -Wl,--gc-sections")
else ()
    add_definitions(-DDEBUG)
    set(C_FLAGS "${C_FLAGS} -O0")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

find_package(xhook REQUIRED CONFIG)

add_library(riru SHARED main.cpp jni_native_method.cpp misc.cpp wrap.cpp api.cpp native_method.cpp hide_utils.cpp pmparser.c status.cpp module.cpp)

target_include_directories(riru PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(riru log xhook::xhook)

add_library(riruhide SHARED hide.cpp wrap.cpp pmparser.c)
target_include_directories(riruhide PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(riruhide log)

#add_executable(zygote_restart
#        zygote_restart.cpp pmparser.c)
#
#target_link_libraries(zygote_restart log)
#
set_target_properties(riru riruhide
        PROPERTIES LINK_FLAGS_RELEASE -s)