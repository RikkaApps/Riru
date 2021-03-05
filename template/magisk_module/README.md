# Riru

Riru only does one thing, inject into zygote in order to allow modules run their codes in apps or the system server.

All other Riru modules requires Riru.

See [https://github.com/RikkaApps/Riru](https://github.com/RikkaApps/Riru) for more details.

### Note

If you are use other modules that changes `ro.dalvik.vm.native.bridge`, Riru will not work. (Riru will automatically set it back)

A typical example is, some "optimize" modules changes this property. Since changing this property is meaningless for "optimization", their quality is very questionable. In fact, changing properties for optimization is a joke.

### Config

* When the file `/data/adb/riru/disable` exists, Riru will do nothing
* When the file `/data/adb/riru/enable_hide` exists, the hide mechanism will be enabled

### Install from recovery is NOT supported

Riru requires files in `/data/adb/riru` folder, this folder is created during installation.

Many TWRP has broken implementations, causing the folder cannot be created, this will finally cause Riru and Riru modules not working.

## Changelog

### v23.9 (59) (2021-03-06)

- Fix crash when JVM reuses reference index on devices with `libnativehelper_lazy` (these changes may come in Android 12 DP2 or later) (by LSPosed devs)

### v23.8 (58) (2021-03-05)

- Fix a problem that only exists on 32-bit devices

### v23.7 (57) (2021-03-01)

- Prepare for changes brought by libnativehelper_lazy (these changes may come in Android 12 DP2 or later)
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
