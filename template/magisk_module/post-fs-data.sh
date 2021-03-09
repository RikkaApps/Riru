#!/system/bin/sh
MODDIR=${0%/*}
RIRU_PATH="/data/adb/riru"

# Rename .new file
move_new_file() {
  if [ -f "$1.new" ]; then
    rm "$1"
    mv "$1.new" "$1"
  fi
}
move_new_file "$RIRU_PATH/api_version"
move_new_file "$MODDIR/rirud"
move_new_file "$MODDIR/rirud.dex"

# Remove old files to avoid downgrade problems
rm /data/misc/riru/api_version
rm /data/misc/riru/version_code
rm /data/misc/riru/version_name

# Start daemon which runs socket "rirud"
exec "$MODDIR"/rirud