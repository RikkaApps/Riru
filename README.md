# Riru

Riru only does one thing, inject into zygote in order to allow modules run their codes in apps or the system server.

> The name, Riru, comes from a character. (https://www.pixiv.net/member_illust.php?mode=medium&illust_id=74128856)

## Requirements

Android 6.0+ devices rooted with [Magisk](https://github.com/topjohnwu/Magisk)

## Guide

### Install

* From Magisk Manager

  1. Search "Riru" in Magisk Manager
  2. Install the module named "Riru"

  > The Magisk version requirement is enforced by Magisk Manager. At the time of the release of Magisk v21.1, the requirement is v20.4.

* Manually

  1. Download the zip from [GitHub release](https://github.com/RikkaApps/Riru/releases)
  2. Install in Magisk Manager (Modules - Install from storage - Select downloaded zip)

* "Riru" app (show Riru status)

  [Download](https://github.com/RikkaApps/Riru/releases/download/v23.0/riru-v23.0.r235.d313e94.apk)

**If you are use other modules that changes `ro.dalvik.vm.native.bridge`, Riru will not work.** (Riru will automatically set it back)

A typical example is, some "optimize" modules changes this property. Since changing this property is meaningless for "optimization", their quality is very questionable. In fact, changing properties for optimization is a joke.

### Config

* When the file `/data/adb/riru/disable` exists, Riru will do nothing
* When the file `/data/adb/riru/enable_hide` exists, the hide mechanism will be enabled (also requires the support of the modules)

## How Riru works?

* How to inject into zygote process?

  Before v22.0, we use the method of replacing a system library (libmemtrack) that will be loaded by zygote. However, it seems to cause some weird problems. Maybe because libmemtrack is used by something else.

  Then we found a super easy way, the "native bridge" (`ro.dalvik.vm.native.bridge`). The specific "so" file will be automatically "dlopen-ed" and "dlclose-ed" by the system. This way is from [here](https://github.com/canyie/NbInjection).

* How to know if we are in an app process or a system server process?

  Some JNI functions (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) is to fork the app process or the system server process.
  So we need to replace these functions to ours. This part is simple, hook `jniRegisterNativeMethods` since all Java native methods in `libandroid_runtime.so` is registered through this function.
  Then we can call the original `jniRegisterNativeMethods` again to replace them.
  
## How hide Hide works?

From v22.0, Riru provide a hide mechanism (idea from [Haruue Icymoon](https://github.com/haruue)), make the memory of Riru and module to anonymous memory to hide from "`/proc/maps` string scanning".

## Build

Run gradle task `:riru:assembleRelease` task from Android Studio or the terminal, zip will be saved to `out`.

## Module template

https://github.com/RikkaApps/Riru-ModuleTemplate

## Module API changes

https://github.com/RikkaApps/Riru-ModuleTemplate/blob/master/README.md#api-changes