#!/system/bin/sh
MODDIR=${0%/*}
RIRU_PATH="/data/misc/riru"

move_new_file() {
  if [ -f "$1.new" ]; then
    rm "$1"
    mv "$1.new" "$1"
  fi
}

# rename .new files
move_new_file "$RIRU_PATH/api_version"
move_new_file "$RIRU_PATH/version_name"
move_new_file "$RIRU_PATH/version_code"

# Copy libmemtrack.so
cp -f "/system/lib/libmemtrack.so" "$MODDIR/system/lib/libmemtrack_real.so"
[[ -f "/system/lib64/libmemtrack.so" ]] && cp -f "/system/lib64/libmemtrack.so" "$MODDIR/system/lib64/libmemtrack_real.so"

# Reset context in case
chcon -R u:object_r:system_file:s0 "$MODDIR"

# Restart zygote if needed
ZYGOTE_RESTART=$RIRU_PATH/bin/zygote_restart
[[ ! -f "$RIRU_PATH/config/disable_auto_restart" ]] && $ZYGOTE_RESTART