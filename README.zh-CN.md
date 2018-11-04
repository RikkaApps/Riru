# Riru

Riru 是一个简单但有用的东西。仅需要替换一个系统文件，就可以让 Riru 模块们进入应用进程或系统服务进程并执行他们的代码。

Riru 这个名字是来自 https://www.pixiv.net/member_illust.php?mode=medium&illust_id=56169989

## 需求

* Root 过的 Android 6.0+ 设备 
* Magisk (用于替换系统文件，暂时只提供 Magisk zip，当然你也可以直接去换文件)

## 她怎么工作呢？

简而言之，替换一个会被 zygote 进程加载的共享库。

首先要找到那个共享库，而且那个共享库要越简单越好，所以就盯上了只有 10 个导出函数的 libmemtrack。
然后就可以自己提供一个叫 libmemtrack 并且也提供了原来的函数们的库，这样就可以进去 zygote 进程也不会发生爆炸。

接着如何知道自己已经在应用进程或者系统服务进程里面。
JNI 函数 (`com.android.internal.os.Zygote#nativeForkAndSpecialize` & `com.android.internal.os.Zygote#nativeForkSystemServer`) 会在应用进程或者系统服务进程被 fork 出来的时候被调用。
所以只要把这两个函数换成自己的。这部分很简单，只要 hook `jniRegisterNativeMethods` 因为所有 libandroid_runtime 里面的 JNI 方法都是通过这个注册，然后就可以再调用 `RegisterNatives` 来替换它们。

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
4. 直接在命令行 (用 `gradlew`) 或者 Android Studio 执行 `:riru-your-module:zip`，zip 会被存到 `release`

## core 提供的 API

从 v8 开始，core 提供一些 API，详见 [riru.h](https://github.com/RikkaApps/Riru/blob/master/riru-module-template/jni/main/riru.h)。

## 自己的模块需要注意的地方

* 为了保证自己 hook 不被其他模块掩盖，需要使用 core 的 API
* 不要在自己的 `attribute constructor` 函数 (或 `LOCAL_LDFLAGS -init`) 掩盖 core 中 hook 的 `jniRegisterNativeMethods`（若想要得到 JNI 方法地址请使用 `riru_get_native_method_func`）
* 不要掩盖 core 中 hook 的 `android.os.SystemProperties#native_set`，
  否则在 Android P 以上可能导致数据抹除（[详细信息](https://github.com/RikkaApps/Riru/blob/v7/riru-core/jni/main/jni_native_method.cpp#L162-L176)）（如果一定要 hook 则要记得清掉异常）
* 不要在 `nativeForkAndSpecialize(Pre/Post)` `nativeForkSystemServer(Pre/Post)` 中在还处于 zygote 进程时输出 log（`__android_log_print`），
  否则可能导致 zygote 不工作（未确定的魔法，[详细信息](https://github.com/RikkaApps/Riru/blob/77adfd6a4a6a81bfd20569c910bc4854f2f84f5e/riru-core/jni/main/jni_native_method.cpp#L55-L66)）

## 保证 hook 不被掩盖的方法

```
#include "riru.h"

your_hook_func(func, new_func, &old_func);

if (riru_get_version() >= 8) { // 先判断 riru 版本
    void *f = riru_get_func("func"); // 如果 f 不为 null 说明其他模块已经设定了
    if (f) old_func = f; // 把 old_func 设为获得的 f（即上个模块 new_func）以保证上个模块的 hook 不被掩盖
    riru_set_func("func", new_func); // 设定 new_func，让下一个模块正确得到 old_func
}
```

## 安装

目前只支持 Magisk

1. Install core zip in Magisk
2. Install module zip in Magisk

## 模块

[Riru-LocationReportEnabler](https://github.com/RikkaApps/Riru-LocationReportEnabler) (also a good example)