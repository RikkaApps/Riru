cmake_minimum_required(VERSION 3.4.1)

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)

if (NOT DEFINED RIRU_VERSION_NAME)
    message(FATAL_ERROR "RIRU_VERSION_NAME is not set")
endif ()

if (NOT DEFINED RIRU_VERSION_CODE)
    message(FATAL_ERROR "RIRU_VERSION_CODE is not set")
endif ()

ADD_DEFINITIONS(-DRIRU_VERSION_NAME="${RIRU_VERSION_NAME}")
ADD_DEFINITIONS(-DRIRU_VERSION_CODE=${RIRU_VERSION_CODE})

set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(C_FLAGS "-Werror=format -fdata-sections -ffunction-sections")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(C_FLAGS "${C_FLAGS} -O2 -fvisibility=hidden -fvisibility-inlines-hidden")
    set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-exclude-libs,ALL -Wl,--gc-sections")
else ()
    set(C_FLAGS "${C_FLAGS} -O0")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

add_subdirectory(xhook)

add_library(riru SHARED
        main.cpp jni_native_method.cpp misc.cpp wrap.cpp redirect_memtrack.cpp version.cpp api.cpp native_method.cpp)

target_include_directories(riru PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(riru log xhook)

add_executable(zygote_restart
        zygote_restart.cpp pmparser.c)

target_link_libraries(zygote_restart log)

set_target_properties(riru zygote_restart
        PROPERTIES LINK_FLAGS_RELEASE -s)