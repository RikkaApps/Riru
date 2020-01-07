# Riru

Riru is a very simple but useful thing. Only requires to replace one system file, it will provide the ability to Riru modules to run their code in apps' or system server's process.

The name Riru comes from a character. (https://www.pixiv.net/member_illust.php?mode=medium&illust_id=74128856)

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

## Build

Run gradle task `:module:assembleRelease` task from Android Studio or command line, zip will be saved to `out`.

## Install

Install zip in Magisk Manager.

## Create your own module

[View template](https://github.com/RikkaApps/Riru-ModuleTemplate)