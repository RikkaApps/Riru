#!/system/bin/sh
MODDIR=${0%/*}
if [ -z "$SHELL" ]; then
  export SHELL=sh
fi
flock -n "$MODDIR/module.prop" -c "sed -Ei 's/^description=(\[.*][[:space:]]*)?/description=[ post-fs-data.sh fails to run. Magisk is broken on this device. ] /g' \"$MODDIR/module.prop\""
