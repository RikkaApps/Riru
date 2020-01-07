#!/system/bin/sh
MODDIR=${0%/*}

# Copy libmemtrack.so
cp -f "/system/lib/libmemtrack.so" "$MODDIR/system/lib/libmemtrack_real.so"
[[ -f "/system/lib64/libmemtrack.so" ]] && cp -f "/system/lib64/libmemtrack.so" "$MODDIR/system/lib64/libmemtrack_real.so"

# Reset context in case
chcon -R u:object_r:system_file:s0 $MODDIR

ZYGOTE_RESTART=/data/misc/riru/bin/zygote_restart
[[ ! -f "/data/misc/riru/config/disable_auto_restart" ]] && $ZYGOTE_RESTART