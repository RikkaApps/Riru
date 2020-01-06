RIRU_PATH="/data/misc/riru"
RIRU_API=4

check_architecture() {
  if [[ "$ARCH" != "arm" && "$ARCH" != "arm64" && "$ARCH" != "x86" && "$ARCH" != "x64" ]]; then
    abort "! Unsupported platform: $ARCH"
  else
    ui_print "- Device platform: $ARCH"
  fi
}

check_architecture

mkdir -p "$RIRU_PATH/modules"

if [[ "$ARCH" == "x86" || "$ARCH" == "x64" ]]; then
ui_print "- Copying x86/64 libraries"
rm -rf "${MODPATH}/system/lib"
rm -rf "${MODPATH}/system/lib64"
mv "$MODPATH/system_x86/lib" "$MODPATH/system/lib"
mv "$MODPATH/system_x86/lib64" "$MODPATH/system/lib64"
fi

if [[ "$IS64BIT" == "false" ]]; then
ui_print "- Removing 64-bit libraries"
rm -rf "$MODPATH/system/lib64"
fi

ui_print "- Extracting zygote_restart executable"
mkdir -p "$RIRU_PATH/bin"
mv "${MODPATH}/zygote_restart_$ARCH" "$RIRU_PATH/bin/zygote_restart"
set_perm "$RIRU_PATH/bin/zygote_restart" 0 0 0700 u:object_r:system_file:s0

ui_print "- Writing api_version file"
echo -n "$RIRU_API" > "$RIRU_PATH/api_version"

