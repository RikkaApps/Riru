#ifndef NATIVE_METHOD_H
#define NATIVE_METHOD_H

#include <jni.h>

namespace NativeMethod {

    void init();

    int getOffset();

    void *getMethodAddress(JNIEnv *env, jclass cls, const char *methodName, const char *methodSignature);

    void *getStaticMethodAddress(JNIEnv *env, jclass cls, const char *methodName, const char *methodSignature);
}

#endif // NATIVE_METHOD_H
