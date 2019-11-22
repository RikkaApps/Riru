#include <cstdio>
#include <cstring>
#include <jni.h>
#include <xhook/xhook.h>
#include <cstdlib>

#include "logging.h"

namespace NativeMethod {

#define NEW_FUNC_DEF(ret, func, ...) \
    static ret (*old_##func)(__VA_ARGS__); \
    static ret new_##func(__VA_ARGS__)

    static int offset = -1;

    int getOffset() {
        return offset;
    }

    static void unhook();

    static int findOffset(uintptr_t value, uintptr_t start, size_t size, size_t step = 1) {
        for (int i = 0; i <= size - step; i += step) {
            uintptr_t current = *(uintptr_t *) (start + i);
            if (value == current) {
                return i;
            }
        }
        return -1;
    }

    static size_t getPossibleSize(uintptr_t *addr, int num) {
        size_t min = 0xffffffff;
        for (int i = 0; i < num - 1; ++i) {
            for (int j = i + 1; j < num; ++j) {
                size_t size = addr[i] > addr[j] ? addr[i] - addr[j] : addr[j] - addr[i];
                if (size < min && size != 0) {
                    min = size;
                }
            }
        }
        return min;
    }

    NEW_FUNC_DEF(int, jniRegisterNativeMethods, JNIEnv *env, const char *className,
                 const JNINativeMethod *methods, int numMethods) {
        int res = old_jniRegisterNativeMethods(env, className, methods, numMethods);
        if (numMethods > 1) {
            LOGV("find offset: className=%s, numMethods=%d", className, numMethods);

            uintptr_t addrs[numMethods];
            jclass cls = env->FindClass(className);
            for (int i = 0; i < numMethods; ++i) {
                jmethodID method = env->GetMethodID(cls, methods[i].name, methods[i].signature);
                if (!method) {
                    env->ExceptionClear();
                    method = env->GetStaticMethodID(cls, methods[i].name, methods[i].signature);
                }
                addrs[i] = (uintptr_t) method;
            }

            size_t size = getPossibleSize(addrs, numMethods);
            LOGV("possible size is %zu", size);

            int o0 = findOffset((uintptr_t) methods[0].fnPtr, addrs[0], size);
            if (o0 != -1) {
                for (int i = 1; i < numMethods; ++i) {
                    int oi = findOffset((uintptr_t) methods[i].fnPtr, addrs[i], size);
                    if (o0 != oi) {
                        LOGV("failed to find offset");
                        o0 = -1;
                        break;
                    }
                }
            }
            if (o0 != -1) {
                offset = o0;
                LOGV("offset is %d", offset);
            }

            unhook();
        }
        return res;
    }

    static void hook() {
        if (xhook_register(".*\\libandroid_runtime.so$", "jniRegisterNativeMethods",
                           (void *) new_jniRegisterNativeMethods,
                           (void **) &old_jniRegisterNativeMethods) != 0) {
            LOGE("failed to register hook jniRegisterNativeMethods.");
            return;
        }

        if (xhook_refresh(0) == 0) {
            xhook_clear();
            LOGI("hook installed");
        } else {
            LOGE("failed to refresh hook");
        }
    }

    static void unhook() {
        xhook_register(".*\\libandroid_runtime.so$", "jniRegisterNativeMethods",
                       (void *) old_jniRegisterNativeMethods,
                       nullptr);
        if (xhook_refresh(0) == 0) {
            xhook_clear();
            LOGV("hook removed");
        }
    }

    void init() {
        hook();
    }

    void *getMethodAddress(JNIEnv *env, jclass cls, const char *methodName, const char *methodSignature) {
        if (getOffset() == -1) {
            return nullptr;
        }
        uintptr_t method = (uintptr_t) env->GetMethodID(cls, methodName, methodSignature);
        return (void *) *((uintptr_t *) (method + getOffset()));
    }

    void *getStaticMethodAddress(JNIEnv *env, jclass cls, const char *methodName, const char *methodSignature) {
        if (getOffset() == -1) {
            return nullptr;
        }
        uintptr_t method = (uintptr_t) env->GetStaticMethodID(cls, methodName, methodSignature);
        return (void *) *((uintptr_t *) (method + getOffset()));
    }
}