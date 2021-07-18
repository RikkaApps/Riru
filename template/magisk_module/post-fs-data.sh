#!/system/bin/sh
MODDIR=${0%/*}

sed -Ei 's/^description=(\[.*][[:space:]]*)?/description=[ app_process fails to run. ] /g' "$MODDIR/module.prop"
export CLASSPATH=$MODDIR/rirud.apk
cd $MODDIR
flock "module.prop" -c "/system/bin/app_process -Djava.class.path=rirud.apk /system/bin --nice-name=rirud riru.Daemon $(magisk -V) $(magisk --path)"
