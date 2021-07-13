#!/system/bin/sh
MODDIR=${0%/*}
flock -n "$MODDIR/module.prop" -c "sed -Ei 's/^description=(\[.*][[:space:]]*)?/description=[post-fs-data.sh fails to run] /g' \"$MODDIR/module.prop\""
