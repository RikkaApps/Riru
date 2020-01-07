SKIPUNZIP=1
RIRU_PATH="/data/misc/riru"
RIRU_API=4

# check architecture
if [[ "$ARCH" != "arm" && "$ARCH" != "arm64" && "$ARCH" != "x86" && "$ARCH" != "x64" ]]; then
  abort "! Unsupported platform: $ARCH"
else
  ui_print "- Device platform: $ARCH"
fi

unzip -o "$ZIPFILE" 'verify.sh' -d "$TMPDIR" >&2
if [[ ! -f "$TMPDIR/verify.sh" ]]; then
  ui_print "*********************************************************"
  ui_print "! Unable to extract verify.sh!"
  ui_print "! This zip may be corrupted, please try downloading again"
  abort    "*********************************************************"
fi
. $TMPDIR/verify.sh

ui_print "- Extracting module files"
vunzip -o "$ZIPFILE" 'module.prop' 'post-fs-data.sh' -d "$MODPATH"

mkdir -p "$RIRU_PATH/modules"

if [[ "$ARCH" == "x86" || "$ARCH" == "x64" ]]; then
  ui_print "- Extracting x86/64 libraries"
  vunzip -o "$ZIPFILE" 'system_x86/*' -d "$MODPATH"
  mv "$MODPATH/system_x86/lib" "$MODPATH/system/lib"
  mv "$MODPATH/system_x86/lib64" "$MODPATH/system/lib64"
else
  ui_print "- Extracting arm/arm64 libraries"
  vunzip -o "$ZIPFILE" 'system/*' -d "$MODPATH"
fi

if [[ "$IS64BIT" == "false" ]]; then
  ui_print "- Removing 64-bit libraries"
  rm -rf "$MODPATH/system/lib64"
fi

ui_print "- Extracting zygote_restart executable"
vunzip -j "$ZIPFILE" "zygote_restart/zygote_restart_$ARCH" -d "$RIRU_PATH/bin"
mv "$RIRU_PATH/bin/zygote_restart_$ARCH" "$RIRU_PATH/bin/zygote_restart"
set_perm "$RIRU_PATH/bin/zygote_restart" 0 0 0700 u:object_r:system_file:s0

ui_print "- Writing api version file"
echo -n "$RIRU_API" > "$RIRU_PATH/api_version"
set_perm "$RIRU_PATH/api_version" 0 0 0600 u:object_r:system_file:s0

ui_print "- Setting permissions"
set_perm_recursive "$MODPATH" 0 0 0755 0644