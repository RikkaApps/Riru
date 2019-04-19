#!/system/bin/sh
# Please don't hardcode /magisk/modname/... ; instead, please use $MODDIR/...
# This will make your scripts compatible even if Magisk change its mount point in the future
MODDIR=${0%/*}

# This script will be executed in late_start service mode
# More info in the main Magisk thread
ZYGOTE_RESTART=/data/misc/riru/bin/zygote_restart
[[ ! -f "/data/misc/riru/config/disable_auto_restart" ]] && $ZYGOTE_RESTART