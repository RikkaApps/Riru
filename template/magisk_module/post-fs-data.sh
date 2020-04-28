#!/system/bin/sh
MODDIR=${0%/*}
RIRU_PATH="/data/misc/riru"

# rename .new files
move_new_file() {
  if [ -f "$1.new" ]; then
    rm "$1"
    mv "$1.new" "$1"
  fi
}

move_new_file "$RIRU_PATH/api_version"
move_new_file "$RIRU_PATH/version_name"
move_new_file "$RIRU_PATH/version_code"

# remove old libmemtrack_real
rm "$MODDIR/system/lib64/libmemtrack_real.so"
rm "$MODDIR/system/lib/libmemtrack_real.so"

# get random name
RANDOM_NAME_FILE="/data/adb/riru/random_name"
RANDOM_NAME=""
if [ -f "$RANDOM_NAME_FILE" ]; then
  RANDOM_NAME=$(cat "$RANDOM_NAME_FILE")

  # Copy libmemtrack.so
  cp -f "/system/lib/libmemtrack.so" "$MODDIR/system/lib/lib$RANDOM_NAME.so"
  [ -f "/system/lib64/libmemtrack.so" ] && cp -f "/system/lib64/libmemtrack.so" "$MODDIR/system/lib64/lib$RANDOM_NAME.so"
fi

# Reset context
chcon -R u:object_r:magisk_file:s0 "/data/adb/riru"

# Restart zygote if needed
ZYGOTE_RESTART=$RIRU_PATH/bin/zygote_restart
[ ! -f "$RIRU_PATH/config/disable_auto_restart" ] && $ZYGOTE_RESTART
