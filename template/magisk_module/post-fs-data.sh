#!/system/bin/sh
MODDIR=${0%/*}
RIRU_PATH="/data/adb/riru"

# Use magisk_file like other Magisk files
mkdir $RIRU_PATH
chcon -R u:object_r:magisk_file:s0 $RIRU_PATH

# Rename .new file
move_new_file() {
  if [ -f "$1.new" ]; then
    rm "$1"
    mv "$1.new" "$1"
  fi
}
move_new_file "$RIRU_PATH/api_version"
move_new_file "$RIRU_PATH/bin/rirud"
move_new_file "$RIRU_PATH/bin/rirud.dex"

# Remove old files to avoid downgrade problems
rm /data/misc/riru/api_version
rm /data/misc/riru/version_code
rm /data/misc/riru/version_name

# Backup ro.dalvik.vm.native.bridge
echo -n "$(getprop ro.dalvik.vm.native.bridge)" > $RIRU_PATH/native_bridge

# Start daemon which runs socket "rirud"
exec $RIRU_PATH/bin/rirud