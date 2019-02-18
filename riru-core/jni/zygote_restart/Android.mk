LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE     := zygote_restart
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)
LOCAL_LDLIBS += -ldl -llog
LOCAL_LDFLAGS := -Wl,--hash-style=both

LOCAL_SRC_FILES:= main.cpp pmparser.c

include $(BUILD_EXECUTABLE)