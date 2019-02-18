LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Change "libriru_template" to your module name, must start with "libriru_"
LOCAL_MODULE     := libriru_template
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)
LOCAL_LDLIBS += -ldl -llog
LOCAL_LDFLAGS := -Wl,--hash-style=both

LOCAL_SRC_FILES:= main.cpp

include $(BUILD_SHARED_LIBRARY)