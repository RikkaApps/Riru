cmake_minimum_required(VERSION 3.4.1)

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)

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

add_library(riru-core SHARED
        main.cpp jni_native_method.cpp misc.cpp wrap.cpp redirect_memtrack.cpp version.cpp api.cpp native_method.cpp)

target_include_directories(riru-core PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(riru-core log xhook)

add_executable(zygote_restart
        zygote_restart.cpp pmparser.c)

target_link_libraries(zygote_restart log)

set_target_properties(riru-core zygote_restart
        PROPERTIES LINK_FLAGS_RELEASE -s)