#!/system/bin/sh

# Reset ro.dalvik.vm.native.bridge or reboot if needed
export CLASSPATH=/data/adb/riru/bin/rirud.dex
su -c app_process -Djava.class.path=/data/adb/riru/bin/rirud.dex /system/bin --nice-name=rirud_java riru.Daemon