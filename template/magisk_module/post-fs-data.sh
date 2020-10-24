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

# Set ro.dalvik.vm.native.bridge
resetprop ro.dalvik.vm.native.bridge libriruloader.so

# Set prop back & reboot if needed
export CLASSPATH=/data/adb/riru/bin/rirud.dex
(exec app_process -Djava.class.path=/data/adb/riru/bin/rirud.dex /system/bin --nice-name=rirud riru.Daemon)&