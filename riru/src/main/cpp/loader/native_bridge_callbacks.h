//
// Created by loves on 7/25/2021.
//

#ifndef RIRU_NATIVE_BRIDGE_CALLBACKS_H
#define RIRU_NATIVE_BRIDGE_CALLBACKS_H

#include <android/api-level.h>

template<unsigned>
struct NativeBridgeCallbacks;

template<>
struct NativeBridgeCallbacks<__ANDROID_API_M__> {
    [[maybe_unused]] uint32_t version;
    [[maybe_unused]] void *initialize;
    [[maybe_unused]] void *loadLibrary;
    [[maybe_unused]] void *getTrampoline;
    [[maybe_unused]] void *isSupported;
    [[maybe_unused]] void *getAppEnv;
    [[maybe_unused]] void *isCompatibleWith;
    [[maybe_unused]] void *getSignalHandler;
};

template<>
struct NativeBridgeCallbacks<__ANDROID_API_N__> : NativeBridgeCallbacks<__ANDROID_API_M__> {
};

template<>
struct NativeBridgeCallbacks<__ANDROID_API_N_MR1__> : NativeBridgeCallbacks<__ANDROID_API_N__> {
};

template<>
struct NativeBridgeCallbacks<__ANDROID_API_O__> : NativeBridgeCallbacks<__ANDROID_API_N_MR1__> {
    [[maybe_unused]] void *unloadLibrary;
    [[maybe_unused]] void *getError;
    [[maybe_unused]] void *isPathSupported;
    [[maybe_unused]] void *initAnonymousNamespace;
    [[maybe_unused]] void *createNamespace;
    [[maybe_unused]] void *linkNamespaces;
    [[maybe_unused]] void *loadLibraryExt;
    [[maybe_unused]] void *getVendorNamespace;
};

template<>
struct NativeBridgeCallbacks<__ANDROID_API_O_MR1__> : NativeBridgeCallbacks<__ANDROID_API_O__> {
};

template<>
struct NativeBridgeCallbacks<__ANDROID_API_P__> : NativeBridgeCallbacks<__ANDROID_API_O_MR1__> {
};

template<>
struct NativeBridgeCallbacks<__ANDROID_API_Q__> : NativeBridgeCallbacks<__ANDROID_API_P__> {
    [[maybe_unused]] void *getExportedNamespace;
};

template<>
struct NativeBridgeCallbacks<__ANDROID_API_R__> : NativeBridgeCallbacks<__ANDROID_API_Q__> {
    [[maybe_unused]] void *preZygoteFork;
};

#endif //RIRU_NATIVE_BRIDGE_CALLBACKS_H
