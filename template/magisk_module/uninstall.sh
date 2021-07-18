#!/sbin/sh
# /data/adb/riru/modules may be used by old modules, remove /data/adb/riru only if /data/adb/riru/modules does not exists
if [ ! -d /data/adb/riru/modules ]; then
  rm -rf /data/adb/riru
fi
