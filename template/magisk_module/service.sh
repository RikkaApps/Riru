#!/system/bin/sh
MODDIR=${0%/*}
flock -n "$MODDIR/module.prop" -c "sed -Ei 's/^description=(\[.*][[:space:]]*)?/description=[ post-fs-data.sh fails to run. Magisk is broken on this device. ] /g' \"$MODDIR/module.prop\""
