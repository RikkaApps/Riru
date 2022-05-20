cmake_minimum_required(VERSION 3.4.1)

project("riru")

find_program(CCACHE ccache)

if (CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
endif ()

message("Build type: ${CMAKE_BUILD_TYPE}")


if (NOT DEFINED RIRU_VERSION_NAME)
    message(FATAL_ERROR "RIRU_VERSION_NAME is not set")
endif ()

if (NOT DEFINED RIRU_VERSION_CODE)
    message(FATAL_ERROR "RIRU_VERSION_CODE is not set")
endif ()

if (NOT DEFINED RIRU_API_VERSION)
    message(FATAL_ERROR "RIRU_API_VERSION is not set")
endif ()

if (NOT DEFINED RIRU_MIN_API_VERSION)
    message(FATAL_ERROR "RIRU_MIN_API_VERSION is not set")
endif ()

configure_file(template/config.cpp config.cpp)

#add_definitions(-DRIRU_VERSION_NAME="${RIRU_VERSION_NAME}")
#add_definitions(-DRIRU_VERSION_CODE=${RIRU_VERSION_CODE})
#add_definitions(-DRIRU_API_VERSION=${RIRU_API_VERSION})
#add_definitions(-DRIRU_MIN_API_VERSION=${RIRU_MIN_API_VERSION})

set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(C_FLAGS "-Werror=format -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics -D__FILE__=__FILE_NAME__ -Wno-builtin-macro-redefined")

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(C_FLAGS "${C_FLAGS} -Os -fvisibility=hidden -fvisibility-inlines-hidden -fno-asynchronous-unwind-tables -fno-unwind-tables")
    set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-exclude-libs,ALL -Wl,--gc-sections")
else ()
    set(C_FLAGS "${C_FLAGS} -Og")
endif ()

string(TOUPPER CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} CXX_FLAGS_WITH_BUILD_TYPE)
string(TOUPPER CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE} C_FLAGS_WITH_BUILD_TYPE)

set(${CXX_FLAGS_WITH_BUILD_TYPE} "${${CXX_FLAGS_WITH_BUILD_TYPE}} ${C_FLAGS}")
set(${C_FLAGS_WITH_BUILD_TYPE} "${${CXX_FLAGS_WITH_BUILD_TYPE}} ${C_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

find_package(xhook REQUIRED CONFIG)
find_package(cxx REQUIRED CONFIG)
find_package(proc-maps-parser REQUIRED CONFIG)

include_directories(include)
include_directories(include_riru)

add_library(utils STATIC
        util/android_prop.cpp
        util/elf_util.cpp
        util/dl.cpp
        util/rirud.cpp)
target_link_libraries(utils cxx::cxx)

add_library(riru SHARED
        entry.cpp
        jni_hooks.cpp
        hide_utils.cpp
        module.cpp
        magisk.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/config.cpp)
target_include_directories(riru PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(riru log utils xhook::xhook cxx::cxx proc-maps-parser::proc-maps-parser)

add_library(riruhide SHARED hide/hide.cpp)
target_include_directories(riruhide PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(riruhide log utils cxx::cxx proc-maps-parser::proc-maps-parser)

if ("${ANDROID_ABI}" STREQUAL "x86" OR "${ANDROID_ABI}" STREQUAL "x86_64")
    add_definitions(-DHAS_NATIVE_BRIDGE)
endif ()

add_library(riruloader SHARED loader/loader.cpp ${CMAKE_CURRENT_BINARY_DIR}/config.cpp)
target_link_libraries(riruloader log utils cxx::cxx)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET riru POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libriru.so")
    add_custom_command(TARGET riruhide POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libriruhide.so")
    add_custom_command(TARGET riruloader POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libriruloader.so")
endif ()
