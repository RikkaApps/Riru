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

# generate a random name
RANDOM_NAME_FILE="/data/adb/riru/random_name"
RANDOM_NAME=""
if [ -f "$RANDOM_NAME_FILE" ]; then
  RANDOM_NAME=$(cat "$RANDOM_NAME_FILE")
else
  while true; do
    RANDOM_NAME=$(mktemp -u XXXXXXXX)
    [ -f "/system/lib/lib$RANDOM_NAME.so" ] || break
  done
  mkdir "/data/adb/riru"
  printf "%s" "$RANDOM_NAME" > "$RANDOM_NAME_FILE"
fi

# use magisk contextr
chcon -R u:object_r:magisk_file:s0 "/data/adb/riru"

# remove old libmemtrack_real
rm "$MODDIR/system/lib64/libmemtrack_real.so"
rm "$MODDIR/system/lib/libmemtrack_real.so"

# Copy libmemtrack.so
cp -f "/system/lib/libmemtrack.so" "$MODDIR/system/lib/lib$RANDOM_NAME.so"
[ -f "/system/lib64/libmemtrack.so" ] && cp -f "/system/lib64/libmemtrack.so" "$MODDIR/system/lib64/lib$RANDOM_NAME.so"

# Reset context in case
chcon -R u:object_r:system_file:s0 "$MODDIR"

# Restart zygote if needed
ZYGOTE_RESTART=$RIRU_PATH/bin/zygote_restart
[ ! -f "$RIRU_PATH/config/disable_auto_restart" ] && $ZYGOTE_RESTART
