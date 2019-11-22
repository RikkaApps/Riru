LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE     := libmemtrack
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	jni/external/include
LOCAL_STATIC_LIBRARIES := xhook
LOCAL_LDLIBS += -ldl -llog
LOCAL_LDFLAGS := -Wl,--hash-style=both

LOCAL_SRC_FILES:= main.cpp jni_native_method.cpp misc.cpp wrap.cpp redirect_memtrack.cpp version.cpp api.cpp native_method.cpp

include $(BUILD_SHARED_LIBRARY)