# Riru

Riru is a very simple but useful thing. Only requires to replace one system file, it will provide the ability to Riru modules to run their code in apps' or system server's process.

The name Riru is from https://www.pixiv.net/member_illust.php?mode=medium&illust_id=56169989

[中文说明](https://github.com/RikkaApps/Riru/blob/master/README.zh-CN.md)

## Requirements

* Rooted Android 6.0+ devices 
* Magisk (use to replace system files, temporarily only provide Magisk zip)

## How it works?

In short, replace a shared library which will be loaded by the zygote process.

First, we need to find that library. The library needs to be as simple as possible, so we found libmemtrack, with only 10 exported functions.
Then we can provide a library named libmemtrack with all its functions, so the functionality will not be affected and we will able to in the zygote process.

Now the next question, how to know if we are in an app process or a system server process.
We found some JNI functions (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) will be called when a app or system server is forked.
So we just need to replace these functions to ours. This part is simple, just hook `jniRegisterNativeMethods` since all Java native method in libandroid_runtime is registered with this function.
Then we can call `RegisterNatives` again to replace them.

## Why Riru is made?

There is only one `libmemtrack.so`, if someone wants to do something by replacing it, others can't. So I made Riru occupy libmemtrack but provide the ability to make modules.

## Build Requirements

1. Android NDK (add the directory with `ndk-build` to `PATH`)
2. `zip` to create the zip file (add to `PATH`)
3. (Windows only) Environment to run `build.sh` script

## Build core

Run `:riru-core:zip` task in the command line (use `gradlew`) or Android Studio, zip will be saved to `release`

## Build your own module

1. Copy `riru-module-template` and rename to your name
2. Change module name in `riru-your-module/build.sh` `riru-your-module/template_override/module.prop` `riru-your-module/template_override/riru_module.prop` `riru-your-module/jni/main/Android.mk`
3. Write your code
4. Run `:riru-your-module:zip` task in command line (use `gradlew`) or Android Studio, zip will be saved to `release`

## APIs provided by core

From v8, core starts to providing some APIs, see [riru.h](https://github.com/RikkaApps/Riru/blob/master/riru-module-template/jni/main/riru.h).

## Where your own module needs attention

* To ensure your hook is not being overwritten by other modules, use API from core
* DO NOT overwrite `jniRegisterNativeMethods` hook in core in your `attribute constructor` func (or `LOCAL_LDFLAGS -init`)
  (To get JNI method address, use `riru_get_native_method_func`)
* DO NOT overwrite `android.os.SystemProperties#native_set` in core, or your data may be wiped
  ([Detail info](https://github.com/RikkaApps/Riru/blob/v7/riru-core/jni/main/jni_native_method.cpp#L162-L176))
  (If you really need to hook this, remember to clear exception)
* DO NO print log (`__android_log_print`) in `nativeForkAndSpecialize(Pre/Post)` `nativeForkSystemServer(Pre/Post)` when in zygote process, or it may cause zygote not work
  (magic not confirmed, [Detail info](https://github.com/RikkaApps/Riru/blob/77adfd6a4a6a81bfd20569c910bc4854f2f84f5e/riru-core/jni/main/jni_native_method.cpp#L55-L66))

## Method to ensure your hook not being overwritten

```
#include "riru.h"

your_hook_func(func, new_func, &old_func);

if (riru_get_version() >= 8) { // determine riru version first
    void *f = riru_get_func("func"); // if f is not null, other module has set it
    if (f) old_func = f; // set your old_func as f (new_func in last module) to ensure last module's hook not being overwritten
    riru_set_func("func", new_func); // set new_func to let next module get correct old_func
}
```

## Install

Current only support Magisk.

1. Install core zip in Magisk
2. Install module zip in Magisk

## Modules

[Riru-LocationReportEnabler](https://github.com/RikkaApps/Riru-LocationReportEnabler) (also a good example)