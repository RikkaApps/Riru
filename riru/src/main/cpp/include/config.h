#pragma once

#define CONFIG_DIR "/data/adb/riru"
#define ENABLE_HIDE_FILE CONFIG_DIR "/enable_hide"
#define SOCKET_ADDRESS "rirud"

#ifdef __LP64__
#define LIB_PATH "/system/lib64/"
#else
#define LIB_PATH "/system/lib/"
#endif
#define MODULE_PATH_FMT LIB_PATH "libriru_%s.so"

#define MODULES_DIR CONFIG_DIR "/modules"