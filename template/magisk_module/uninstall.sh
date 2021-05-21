#!/sbin/sh
if [ -d /data/adb/riru/modules ]; then
  # If user has old modules installed, only remove files created by Riru itself
  rm /data/adb/riru/api_version
  rm /data/adb/riru/dev_random
  rm /data/adb/riru/util_functions.sh
else
  rm -rf /data/adb/riru
fi
