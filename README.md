# Riru

Riru is a very simple but useful thing. Only requires to replace one system file, it will provide the ability to Riru modules to run their code in apps' or system server's process.

The name Riru comes from a character. (https://www.pixiv.net/member_illust.php?mode=medium&illust_id=74128856)

[中文说明](https://github.com/RikkaApps/Riru/blob/master/README.zh-CN.md)

## Requirements

* Rooted Android 6.0+ devices 
* Magisk (use to replace system files, temporarily only provide Magisk zip)

## How it works?

In short, replace a shared library which will be loaded by the zygote process.

First, we need to find that library. The library needs to be as simple as possible, so we found libmemtrack, with only 10 exported functions.
Then we can provide a library named libmemtrack with all its functions, so the functionality will not be affected and we will able to in the zygote process. (However, it seems that choose libmemtrack is not so appropriate now)

Now the next question, how to know if we are in an app process or a system server process.
We found some JNI functions (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) will be called when a app or system server is forked.
So we just need to replace these functions to ours. This part is simple, just hook `jniRegisterNativeMethods` since all Java native method in libandroid_runtime is registered with this function.
Then we can call `RegisterNatives` again to replace them.

## Why Riru is made?

There is only one `libmemtrack.so`, if someone wants to do something by replacing it, others can't. So I made Riru occupy libmemtrack but provide the ability to make modules.

## Build Requirements

Android NDK (add the directory with `ndk-build` to `PATH`)

## Build core

* Magisk Module

  Run `:riru-core:assembleMagiskRelease` task in the command line (use `gradlew`) or Android Studio, zip will be saved to `release`

## Build your own module

1. Copy `riru-module-template` and rename to your name
2. Change module name in `riru-your-module/jni/main/Android.mk`
3. Change module information in `build.gradle`
4. Write your code
5. Run `:riru-your-module:assembleMagiskRelease` task in command line (use `gradlew`) or Android Studio, zip will be saved to `release`

## Where your own module needs attention

* DO NOT overwrite `android.os.SystemProperties#native_set` in core, or your data may be wiped
  ([Detail info](https://github.com/RikkaApps/Riru/blob/v7/riru-core/jni/main/jni_native_method.cpp#L162-L176))
  (If you really need to hook this, remember to clear exception)
* DO NO print log (`__android_log_print`) in `nativeForkAndSpecialize(Pre/Post)` `nativeForkSystemServer(Pre/Post)` when in zygote process, or it may cause zygote not work
  (magic not confirmed, [Detail info](https://github.com/RikkaApps/Riru/blob/77adfd6a4a6a81bfd20569c910bc4854f2f84f5e/riru-core/jni/main/jni_native_method.cpp#L55-L66))
* Add `-ffixed-x18` to both compiler and linker parameter, or it will cause problems on Android Q (see template)

## Riru API

* Currently, one module version can only support one API version
* See template for details

### v4 (core v19+)

* Add `api=4` to `riru_module.prop` to declare API version
* Check and deny installation if Riru version is below v19 in `config.sh`
* Add `specializeAppProcessPre` `specializeAppProcessPost` used by Android Q beta 3 (see template)

### v3 (core v18+)

* Add `api=3` to `riru_module.prop` to declare API version
* Check and deny installation if Riru version is below v18 in `config.sh` 
* Parameter of `nativeForkAndSpecializePre` changes (compare to v2, added `jstring *packageName, jobjectArray *packagesForUID, jstring *sandboxId` in the end)

### v2 (core v16-v17.1)

* Export `int getApiVersion() { return 2; }` to declare API version
* Parameter of `nativeForkAndSpecializePre` changes (compare to v1, all parameter is pointer)

## Install

Current only support Magisk.

1. Install core zip in Magisk
2. Install module zip in Magisk

## Modules

[Riru-LocationReportEnabler](https://github.com/RikkaApps/Riru-LocationReportEnabler) (also a good example)
