# Riru

Riru only does one thing, inject into zygote in order to allow modules run their codes in apps or the system server.

> The name, Riru, comes from a character. (https://www.pixiv.net/member_illust.php?mode=medium&illust_id=74128856)

## Requirements

Android 7.0+ devices rooted with [Magisk](https://github.com/topjohnwu/Magisk)

## How Riru works?

* How to inject into zygote process?

  Before v22.0, we use the method of replacing a system library (libmemtrack) that will be loaded by zygote. However, it seems to cause some weird problems. Maybe because libmemtrack is used by something else.

  Then we found a super easy way, add our so file into `/system/etc/public.libraries.txt`. All so files in `public.libraries.txt` will be automatically "dlopen-ed" by the system. This way is from [here](https://blog.canyie.top/2020/02/03/a-new-xposed-style-framework/).

* How to know if we are in an app process or a system server process?

  Some JNI functions (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) is to fork the app process or the system server process.
  So we need to replace these functions to ours. This part is simple, hook `jniRegisterNativeMethods` since all Java native methods in `libandroid_runtime.so` is registered through this function.
  Then we can call the original `jniRegisterNativeMethods` again to replace them.
  
## How hide Hide works?

From v22.0, Riru provide a hide mechanism (idea from [Haruue Icymoon](https://github.com/haruue)), make the memory of Riru and module to anonymous memory to hide from "`/proc/maps` string scanning".

## Build

Run gradle task `:module:assembleRelease` task from Android Studio or the terminal, zip will be saved to `out`.

## Install

Install zip in Magisk Manager.

## Create your own module

[Template](https://github.com/RikkaApps/Riru-ModuleTemplate)