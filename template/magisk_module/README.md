# Riru

Inject zygote process by replacing libmemtrack, provide the ability for Riru modules to run their code in apps' or system server's process.

All other Riru modules requires Riru installed to work.

See [https://github.com/RikkaApps/Riru](https://github.com/RikkaApps/Riru) for more details.

## Changelog

### v20.1 (32) (2020-04-21)

- Works Android R DP2

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