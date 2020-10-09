#!/system/bin/sh
MODDIR=${0%/*}
RIRU_PATH="/data/adb/riru"

# rename .new files
move_new_file() {
  if [ -f "$1.new" ]; then
    rm "$1"
    mv "$1.new" "$1"
  fi
}

move_new_file "$RIRU_PATH/api_version"

# Reset context
chcon -R u:object_r:magisk_file:s0 "/data/adb/riru"

# Restart zygote if needed
#ZYGOTE_RESTART=$RIRU_PATH/bin/zygote_restart
#[ ! -f "$RIRU_PATH/config/disable_auto_restart" ] && $ZYGOTE_RESTART

# generate public.libraries.txt
# idea from https://blog.canyie.top/2020/02/03/a-new-xposed-style-framework/
LIBRARIES_FILE='/system/etc/public.libraries.txt'
mkdir -p "$MODDIR/system/etc"
cp -f $LIBRARIES_FILE "$MODDIR/$LIBRARIES_FILE"
grep -qxF 'libriru.so' "$MODDIR/$LIBRARIES_FILE" || echo 'libriru.so' >> "$MODDIR/$LIBRARIES_FILE"