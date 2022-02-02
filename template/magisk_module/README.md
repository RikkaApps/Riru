# Deprecated

All Riru users and Riru modules should migrate to Zygisk.

# Riru

Riru only does one thing, inject into zygote in order to allow modules run their codes in apps or the system server.

All other Riru modules requires Riru.

See [https://github.com/RikkaApps/Riru](https://github.com/RikkaApps/Riru) for more details.

### Pre-24 modules are no longer supported

> All live modules have upgraded to v24+ for a long time.

The goal is to eliminate bad designs due to historical reasons.

If you are module developer, it only takes you less than 1 min to switch to v24+ if your module is already v22.

For users, if you find you cannot install a module (the installer will say Riru is not installed) or a module stop working, it means that such modules have stop updating for at least 4 months.

### Note

If you are use other modules that changes `ro.dalvik.vm.native.bridge`, Riru will not work. (Riru will automatically set it back)

A typical example is, some "optimize" modules changes this property. Since changing this property is meaningless for "optimization", their quality is very questionable. In fact, changing properties for optimization is a joke.

### Install from recovery is NOT supported

Many TWRP has broken implementations, which will finally cause Riru and Riru modules "installed" but not working.

### Incorrect SELinux rules problem

<https://github.com/RikkaApps/Riru/wiki/Explanation-about-incorrect-SELinux-rules-from-third-party-ROMs-cause-Riru-not-working>

## Changelog

### v26.1.5 (2022-2-2)
- THIS IS THE LAST RELEASE. MODULES AND USERS SHOULD MIGRATE TO ZYGISK.
- Fix description hint

### v26.1.4 (2021-12-15)
- Skip launching Rirud when Zygisk is enabled
- Fix `sonext` update during hiding

### v26.1.3 (2021-10-08)
- Call `android_create_namespace` on Android 8.0+

### v26.1.2 (2021-09-04)
- Speedup Rirud launch
- Warning about incorrect SELinux policy
- No warning about Rirud socket closed
- Use `getprogname` to detect zygote

### v26.1.1 (2021-08-18)
- Fix reset native bridge

### v26.1.0 (2021-08-16)
- Preload modules parallelly to speedup startup
- Fix status showing after a soft reboot
- Fix riru is not loading after a soft reboot
- Prevent modules from using internal interfaces
- Stricter authorization of Rirud
- More accurate loaded status
- Fix status showing on Huawei and Samsung devices
- Unshare Rirud

### v26.0.5 (2021-08-01)
- Fix killing parent process

### v26.0.4 (2021-07-30)
- Fix flock not working on some devices

  This is a bug from Magisk's busybox and it will be fixed [here](https://github.com/topjohnwu/ndk-busybox/commit/d75558194ae9c9dfaa21a4e514c91ec6127016f9). As a workaround, we set `SHELL` in the script manually.

### v26.0.3 (2021-07-27)

- Reset SELinux context for module files when necessary

  This will not always work since on ROMs with incorrect SELinux rules, the system will reset module file to the incorrect one at the same time

### v26.0.2 (2021-07-27)

- Report if the SELinux context of the module files are incorrect
- Use Resources for i18n

### v26.0.1 (2021-07-18)

- Remove support for pre-v24 modules (it has been more than 4 months and all live modules have upgraded to v24+)
- Display status on module description in Magisk (app is removed because of this)
- Combine `rirud` with `rirud_java`
- Refactor codes

### v25.4.4 (2021-05-07)

- Fix in rare cases "soft boot" causes Riru not working
- Fix keep `allow_install_app` flag (#225)

### v25.4.3 (2021-05-05)

- Exit `service.sh` script
- Use uid 0 to install app

### v25.4.2 (2021-04-15)

- "Fix" system server injection does not work on Huawei devices by setting `ro.maple.enable` to `0`

### v25.4.1 (2021-04-10)

- Report incorrect SELinux rule [1]
- Bundle app with the module (Create file `/data/adb/modules/riru-core/allow_install_app` to allow the module to install the app)

[1] <https://github.com/RikkaApps/Riru/wiki/Explanation-about-incorrect-SELinux-rules-from-third-party-ROMs-cause-Riru-not-working>

### v25.3.4 (2021-03-24)

- Unload API 25+ modules in the app process if the module does not provide related functions
- Don't use temporary buffers when parsing PID maps in pmparser ([#202](https://github.com/RikkaApps/Riru/pull/202))
- Use self-compiled libcxx (https://github.com/topjohnwu/libcxx)

### v25.3.3 (2021-03-22)

- Fix crash on Android 8.0 again

### Important changes from the last stable version (v23.9)

- Unify the Riru API version and Riru version, Riru 25 stands for API version 25
- For modules that have adapted Riru API 24+, lib files are loaded from the Magisk path directly, they don't need to be mounted to `/system` anymore
- Support unload self and modules, leaving no trace for unrelated processes (requires module changes)
- Support remove self and modules from `dl_iterate_phdr`
- `/data/adb/riru/modules` is no longer used, you can remove it when all modules are updated to Riru API 24+

### v25.3.2 (2021-03-22)

- New way to get realpath on old systems
- Fix next offset on Android 9

### v25.3.1 (2021-03-20)

- Fix crash on Android 8

### v25.3.0 (2021-03-20)

- Support remove self and modules from `dl_iterate_phdr` now works for all Android versions

### v25.2.0 (2021-03-17)

- Always clear name from `dl_iterate_phdr`
- Fix reset native bridge is broken since v24.0.0
- Continue to reduce the file size (down to less than 200K now)

### v25.0.0 (2021-03-16)

- Support unload self and modules, leaving no trace for unrelated processes (requires module changes)
- Support remove self and modules from `dl_iterate_phdr` (requires Android 8.0+)
- Use a new way to bypass `dlopen` path limitation

### v24.1.2 (2021-03-13)

- Don't attempt to run hide for `webview_zygote` on pre-29

### v24.1.1 (2021-03-13)

- Hide is enabled by default and cannot be disabled
- Hide works on pre-29 without extra SELinux rule

Since v24 starts to load so files directly from the Magisk path (`/sbin` or `/dev`), it's highly possible to trigger anti-cheat from games, so hide is a must.

### v24.1.0 (2021-03-12)

- Hide names from `dl_iterate_phdr`

### v24.0.1 (2021-03-11)

- Fix pre-v24 modules installation

### v24.0.0 (2021-03-11)

- Unify the Riru API version and Riru version, now the API version is 24
- For modules that have adapted Riru API 24, lib files are loaded from the Magisk path directly, they don't need to be mounted to `/system` anymore
- `/data/adb/riru/modules` is no longer used, you can remove it when all modules are update to Riru API 24
- Use git commit count as version code
- Remove fallback SELinux rules, if rirud is not started, it's highly possible that the booting processes of Magisk is totally broken on your device

### v23.9 (59) (2021-03-06)

- Fix crash when JVM reuses reference index on devices with `libnativehelper_lazy` (`libnativehelper_lazy` may come in Android 12 DP2 or later) (by LSPosed devs)

### v23.8 (58) (2021-03-05)

- Fix a problem that only exists on 32-bit devices

### v23.7 (57) (2021-03-01)

- Prepare for changes brought by `libnativehelper_lazy` (these changes may come in Android 12 DP2 or later)
- Fix symbols are incorrectly exported

### v23.6 (56) (2021-02-21)

- Continue reduce the file size
- Works on devices that have dropped 32-bit support (Android 12 emulator or devices in the future)

### v23.5 (55) (2021-02-11)

- Reduce the file size

### v23.4 (54) (2021-01-23)

- Ensure auto restart works

### v23.3 (53) (2021-01-13)

- Ensure auto restart works

### v23.2 (52) (2021-01-02)

- Add `/data/adb/riru/util_functions.sh` for module installer to use
- Ensure auto restart works

### v23.1 (51) (2020-12-18)

- Restart zygote even for the first time (for "broken environment", such as modules are loaded after than zygote is started)
- Hide should work for pre-Android-10
- Prevent crash caused by hiding failure

### v23.0 (49) (2020-12-02)

- Add read file & read dir function for "rirud". Modules can use this to read files that zygote itself has no permission to access.

### v22.4 (46) (2020-11-26)

Magisk's `sepolicy.rule` does not work on some devices and no one report to Magisk 😒. Versions from 22.1 to 22.4 attempt to workaround it.

- Add a socket runs under `u:r:zygote:s0 context` to handle all file operations from zygote (Riru)
- For Magisk < v21.1, reboot twice is no longer required

### v22.0 (41) (2020-10-09)

Riru v22 has a new hide mechanism which makes detection "not that easy".

Because of this, all modules must change. If your module hasn't updated, ask the module developer to make changes. **For 99% modules, this is super easy.**

**Before Magisk v21.1, you have to manually reboot the device twice.**

### v21.3 (36) (2020-07-01)

- Support custom ROMs with isTopApp changes backported (#106)

### v21.2 (35) (2020-05-08)

- Works on Android R DP4

### v21.1 (34) (2020-04-28)

- Generate a random name for libmemtrack_real to temporarily make SafetyNet happy (this can't work for long)

### v21.0 (33) (2020-04-24)

- Works on Android R DP3

### v20.1 (32) (2020-04-21)

- Works on Android R DP2

### v19.8 (30)

- Fix install script for x86 ([#91](https://github.com/RikkaApps/Riru/pull/91))
- Allow uid 1000 to access `/data/misc/riru`

### v19.7 (29)

- Support Samsung Q with "usap" enabled (this should happens only on custom ROMs?)

### v19.6 (28)

- Support Samsung Q
- Copy libmemtrack.so in `post-fs-data.sh`
- Upgrade to the latest module format

### v19.5 (27)

- Verify important files on install (2019/10/29)
- Fix [#58](https://github.com/RikkaApps/Riru/issues/58)

### v19.4 (26)

- Fix bug

### v19.3 (25)

- Fix not work on Android Q Beta 5 (if "process pool" enabled)
- Remove jniRegisterNativeMethods hook when entering the app process

### v19 (21)

- Always reset module files SELinux in case

### v19 (20)

- Support Android Q Beta 3 (all modules need to be upgraded)
