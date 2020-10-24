#!/system/bin/sh
MODDIR=${0%/*}
RIRU_PATH="/data/adb/riru"

# Rename .new file
if [ -f "$RIRU_PATH/api_version.new" ]; then
  rm "$RIRU_PATH/api_version"
  mv "$RIRU_PATH/api_version.new" "$RIRU_PATH/api_version"
fi

# Backup ro.dalvik.vm.native.bridge
echo -n "$(getprop ro.dalvik.vm.native.bridge)" > $RIRU_PATH/native_bridge