# Riru

Riru 是一个简单但有用的东西。仅需要替换一个系统文件，就可以让 Riru 模块们进入应用进程或系统服务进程并执行他们的代码。

## 需求

* Root 过的 Android 6.0+ 设备 
* Magisk (用于替换系统文件，暂时只提供 Magisk zip，当然你也可以直接去换文件)

## 她怎么工作呢？

简而言之，替换一个会被 zygote 进程加载的共享库。

首先，我们需要找到那个共享库。那个共享库要越简单越好，所以我们找到了只有 10 个导出函数的 libmemtrack。然后我们就可以提供一个叫 libmemtrack 也提供原本全部函数的库，这样原本的功能不会受到影响，我们也可以进去 zygote 进程。

接着就是下一个问题，如何或者我们现在已经在一个应用进程或者系统服务进程。JNI 函数 (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) 会在应用进程或者系统服务进程被 fork 出来的时候被调用。所以只要把这两个函数换成自己的。这部分很暴力（，是凄惨搜内存。最后可以找到这些方法对应的 `JNINativeMethod`，就可以再调用 `RegisterNatives` 来替换他们。

## 为什么要做出 Riru 呢？

因为 `libmemtrack.so` 只有一个，如果有人想用替换 libmemtrack 的套路来做点什么别人就做不了。所以就制造了 Riru 来占下 libmemtrack 但是提供了模块这样的东西。

## 构建需求

1. Android NDK (把有 `ndk-build` 的目录加到 `PATH`)
2. `zip` 用来创建 zip 文件 (也要加到 `PATH`)
3. (仅 Windows) 用于执行 `build.sh` 的环境

## 构建 core

直接在命令行 (用 `gradlew`) 或者 Android Studio 执行 `:riru-core:zip`，zip 会被存到 `release`

## 构建你自己的模块

1. 复制 `riru-module-template` 并重命名
2. 在 `riru-your-module/build.sh` `riru-your-module/template_override/module.prop` `riru-your-module/template_override/riru_module.prop` `riru-your-module/jni/main/Android.mk` 中修改模块名字
3. 发明你自己的魔法（
4. 直接在命令行 (用 `gradlew`) 或者 Android Studio 执行 `:riru-core:zip`，zip 会被存到 `release`


## 安装

目前只支持 Magisk

1. Install core zip in Magisk
2. Install module zip in Magisk

## 模块

[Riru-LocationReportEnabler](https://github.com/RikkaApps/Riru-LocationReportEnabler) (also a good example)