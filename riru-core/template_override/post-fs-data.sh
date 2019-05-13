#!/system/bin/sh
# Do NOT assume where your module will be located.
# ALWAYS use $MODDIR if you need to know where this script
# and module is placed.
# This will make sure your module will still work
# if Magisk change its mount point in the future
MODDIR=${0%/*}

# This script will be executed in post-fs-data mode
#mkdir -p /sbin/riru
#mount -o bind /sbin/.core/mirror/data/adb/riru /sbin/riru

# Reset context in case
chcon -R u:object_r:system_file:s0 $MODDIR

ZYGOTE_RESTART=/data/misc/riru/bin/zygote_restart
[[ ! -f "/data/misc/riru/config/disable_auto_restart" ]] && $ZYGOTE_RESTART