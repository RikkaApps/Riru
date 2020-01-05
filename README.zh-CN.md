# Riru

Riru 是一个简单但有用的东西。仅需要替换一个系统文件，就可以让 Riru 模块们进入应用进程或系统服务进程并执行他们的代码。

Riru 这个名字来自 https://www.pixiv.net/member_illust.php?mode=medium&illust_id=74128856

## 需求

* Root 过的 Android 6.0+ 设备 
* Magisk (用于替换系统文件，暂时只提供 Magisk zip，当然你也可以直接去换文件)

## 她怎么工作呢？

简而言之，替换一个会被 zygote 进程加载的共享库。

首先要找到那个共享库，而且那个共享库要越简单越好，所以就盯上了只有 10 个导出函数的 libmemtrack。
然后就可以自己提供一个叫 libmemtrack 并且也提供了原来的函数们的库，这样就可以进去 zygote 进程也不会发生爆炸。（然而现在看来选 libmemtrack 也不是很好）

接着如何知道自己已经在应用进程或者系统服务进程里面。
JNI 函数 (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) 会在应用进程或者系统服务进程被 fork 出来的时候被调用。
所以只要把这两个函数换成自己的。这部分很简单，只要 hook `jniRegisterNativeMethods` 因为所有 libandroid_runtime 里面的 JNI 方法都是通过这个注册，然后就可以再调用 `RegisterNatives` 来替换它们。

## 为什么要做出 Riru 呢？

因为 `libmemtrack.so` 只有一个，如果有人想用替换 libmemtrack 的套路来做点什么别人就做不了。所以就制造了 Riru 来占下 libmemtrack 但是提供了模块这样的东西。

## 构建需求

Android NDK (把有 `ndk-build` 的目录加到 `PATH`)

## 构建 core

* Magisk 模块

  在命令行 (用 `gradlew`) 或者 Android Studio 执行 `:riru-core:assembleMagiskRelease`，zip 会被存到 `release`

## 构建你自己的模块

1. 复制 `riru-module-template` 并重命名
2. 在 `riru-your-module/jni/main/Android.mk` 中修改模块名字
3. 在 `build.gradle` 修改模块信息
4. 发明你自己的魔法（
5. 直接在命令行 (用 `gradlew`) 或者 Android Studio 执行 `:riru-your-module:assembleMagiskRelease`，zip 会被存到 `release`

## 自己的模块需要注意的地方

* 不要掩盖 core 中 hook 的 `android.os.SystemProperties#native_set`，
  否则在 Android P 以上可能导致数据抹除（[详细信息](https://github.com/RikkaApps/Riru/blob/v7/riru-core/jni/main/jni_native_method.cpp#L162-L176)）（如果一定要 hook 则要记得清掉异常）
* 不要在 `nativeForkAndSpecialize(Pre/Post)` `nativeForkSystemServer(Pre/Post)` 中在还处于 zygote 进程时输出 log（`__android_log_print`），
  否则可能导致 zygote 不工作（未确定的魔法，[详细信息](https://github.com/RikkaApps/Riru/blob/77adfd6a4a6a81bfd20569c910bc4854f2f84f5e/riru-core/jni/main/jni_native_method.cpp#L55-L66)）
* 编译时需要为编译和链接指令加上 `-ffixed-x18`，否则在 Android Q 上会产生问题（参考 template）

## Riru API

* 目前，一个模块版本只可支持一个 API 版本
* 看 template 来知道具体做法

### v4 (core v19+)

* 在 `riru_module.prop` 加入 `api=4` 来声明 API 版本
* 在 `config.sh` 检查并拒绝在 Riru v19 以下安装
* 增加 Android Q beta 3 使用的 `specializeAppProcessPre` `specializeAppProcessPost`（请参看 template）

### v3 (core v18+)

* 在 `riru_module.prop` 加入 `api=3` 来声明 API 版本
* 在 `config.sh` 检查并拒绝在 Riru v18 以下安装
* `nativeForkAndSpecializePre` 参数变化（相对 v2，末尾增加 `jstring *packageName, jobjectArray *packagesForUID, jstring *sandboxId`）

### v2 (core v16-v17.1)

* 导出 `int getApiVersion() { return 2; }` 来声明版本号
* `nativeForkAndSpecializePre` 参数变化（相对 v1，所有参数都是指针）
* 加入 `shouldSkipUid`

## 安装

目前只支持 Magisk

1. Install core zip in Magisk
2. Install module zip in Magisk

## 模块

[Riru-LocationReportEnabler](https://github.com/RikkaApps/Riru-LocationReportEnabler) (also a good example)

[Riru-EdXposed](https://github.com/ElderDrivers/EdXposed) (port Xposed to Android P & Q)
