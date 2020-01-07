# Riru

Riru 是一个简单但有用的东西。仅需要替换一个系统文件，就可以让 Riru 模块们进入应用进程或系统服务进程并执行他们的代码。

Riru 这个名字来自 https://www.pixiv.net/member_illust.php?mode=medium&illust_id=74128856

## 需求

* Root 过的 Android 6.0+ 设备 
* Magisk (用于替换系统文件，暂时只提供 Magisk zip)

## 她怎么工作呢？

简而言之，替换一个会被 zygote 进程加载的共享库。

首先要找到那个共享库，而且那个共享库要越简单越好，所以就盯上了只有 10 个导出函数的 libmemtrack。
然后就可以自己提供一个叫 libmemtrack 并且也提供了原来的函数们的库，这样就可以进去 zygote 进程也不会发生爆炸。（然而现在看来选 libmemtrack 也不是很好）

接着如何知道自己已经在应用进程或者系统服务进程里面。
JNI 函数 (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) 会在应用进程或者系统服务进程被 fork 出来的时候被调用。
所以只要把这两个函数换成自己的。这部分很简单，只要 hook `jniRegisterNativeMethods` 因为所有 libandroid_runtime 里面的 JNI 方法都是通过这个注册，然后就可以再调用 `RegisterNatives` 来替换它们。

## 为什么要做出 Riru 呢？

因为 `libmemtrack.so` 只有一个，如果有人想用替换 libmemtrack 的套路来做点什么别人就做不了。所以就制造了 Riru 来占下 libmemtrack 但是提供了模块这样的东西。

## 构建

在 Android Studio 或命令行执行 gradle task `:riru-core:assembleMagiskRelease`，zip 会被存到 `release`。

## 安装

在 Magisk Manager 中安装 zip。

## 创造你自己的模块

[查看模板](https://github.com/RikkaApps/Riru-ModuleTemplate)