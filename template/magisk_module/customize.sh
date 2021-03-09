SKIPUNZIP=1

RIRU_PATH="/data/adb/riru"
RIRU_API="%%%RIRU_API%%%"
RIRU_VERSION_CODE="%%%RIRU_VERSION_CODE%%%"
RIRU_VERSION_NAME="%%%RIRU_VERSION_NAME%%%"

if $BOOTMOE; then
  ui_print "- Installing from Magisk app"
else
  ui_print "*********************************************************"
  ui_print "! Install from recovery is NOT supported"
  ui_print "! Some recovery has broken implementations, install with such recovery will finally cause Riru or Riru modules not working"
  ui_print "! Please install from Magisk app"
  abort "*********************************************************"
fi

ui_print "- Installing Riru $RIRU_VERSION_NAME ($RIRU_VERSION_CODE, API v$RIRU_API)"

# check Magisk
ui_print "- Magisk version: $MAGISK_VER ($MAGISK_VER_CODE)"

# check android
if [ "$API" -lt 23 ]; then
  ui_print "! Unsupported sdk: $API"
  abort "! Minimal supported sdk is 23 (Android 6.0)"
else
  ui_print "- Device sdk: $API"
fi

# check architecture
if [ "$ARCH" != "arm" ] && [ "$ARCH" != "arm64" ] && [ "$ARCH" != "x86" ] && [ "$ARCH" != "x64" ]; then
  abort "! Unsupported platform: $ARCH"
else
  ui_print "- Device platform: $ARCH"
fi

unzip -o "$ZIPFILE" 'verify.sh' -d "$TMPDIR" >&2
if [ ! -f "$TMPDIR/verify.sh" ]; then
  ui_print "*********************************************************"
  ui_print "! Unable to extract verify.sh!"
  ui_print "! This zip may be corrupted, please try downloading again"
  abort    "*********************************************************"
fi
. $TMPDIR/verify.sh

ui_print "- Extracting Magisk files"

extract "$ZIPFILE" 'module.prop' "$MODPATH"
extract "$ZIPFILE" 'post-fs-data.sh' "$MODPATH"
extract "$ZIPFILE" 'service.sh' "$MODPATH"
extract "$ZIPFILE" 'uninstall.sh' "$MODPATH"
extract "$ZIPFILE" 'sepolicy.rule' "$MODPATH"
extract "$ZIPFILE" 'system.prop' "$MODPATH"
extract "$ZIPFILE" 'util_functions.sh' "$RIRU_PATH" true

mkdir "$MODPATH/lib"
mkdir "$MODPATH/lib64"
mkdir "$MODPATH/system"
mkdir "$MODPATH/system/lib"
[ "$IS64BIT" = true ] && mkdir "$MODPATH/system/lib64"

if [ "$ARCH" = "x86" ] || [ "$ARCH" = "x64" ]; then
  ui_print "- Extracting x86 libraries"
  extract "$ZIPFILE" 'lib/x86/libriru.so' "$MODPATH/lib" true
  extract "$ZIPFILE" 'lib/x86/libriruhide.so' "$MODPATH/lib" true
  extract "$ZIPFILE" 'lib/x86/libriruloader.so' "$MODPATH/system/lib" true

  if [ "$IS64BIT" = true ]; then
    ui_print "- Extracting x64 libraries"
    extract "$ZIPFILE" 'lib/x86_64/libriru.so' "$MODPATH/lib64" true
    extract "$ZIPFILE" 'lib/x86_64/libriruhide.so' "$MODPATH/lib64" true
    extract "$ZIPFILE" 'lib/x86_64/libriruloader.so' "$MODPATH/system/lib64" true
    extract "$ZIPFILE" 'lib/x86_64/librirud.so' "$MODPATH" true
  else
    extract "$ZIPFILE" 'lib/x86/librirud.so' "$MODPATH" true
  fi
else
  ui_print "- Extracting arm libraries"
  extract "$ZIPFILE" 'lib/armeabi-v7a/libriru.so' "$MODPATH/lib" true
  extract "$ZIPFILE" 'lib/armeabi-v7a/libriruhide.so' "$MODPATH/lib" true
  extract "$ZIPFILE" 'lib/armeabi-v7a/libriruloader.so' "$MODPATH/system/lib" true

  if [ "$IS64BIT" = true ]; then
    ui_print "- Extracting arm64 libraries"
    extract "$ZIPFILE" 'lib/arm64-v8a/libriru.so' "$MODPATH/lib64" true
    extract "$ZIPFILE" 'lib/arm64-v8a/libriruhide.so' "$MODPATH/lib64" true
    extract "$ZIPFILE" 'lib/arm64-v8a/libriruloader.so' "$MODPATH/system/lib64" true
    extract "$ZIPFILE" 'lib/arm64-v8a/librirud.so' "$MODPATH" true
  else
    extract "$ZIPFILE" 'lib/armeabi-v7a/librirud.so' "$MODPATH" true
  fi
fi

ui_print "- Setting permissions"
set_perm_recursive "$MODPATH" 0 0 0755 0644

ui_print "- Moving rirud"
rm "$MODPATH/rirud.new"
mv "$MODPATH/librirud.so" "$MODPATH/rirud.new"
set_perm "$MODPATH/rirud.new" 0 0 0700

ui_print "- Extracting rirud.dex"
extract "$ZIPFILE" "classes.dex" "$MODPATH"
rm "$MODPATH/rirud.dex.new"
mv "$MODPATH/classes.dex" "$MODPATH/rirud.dex.new"
set_perm "$MODPATH/rirud.dex.new" 0 0 0600

# write api version to a persist file, only for the check process of the module installation
ui_print "- Writing Riru files"
echo -n "$RIRU_API" > "$RIRU_PATH/api_version.new"
set_perm "$RIRU_PATH/api_version.new" 0 0 0600

# Copy old config
if [ "$MAGISK_VER_CODE" -ge 21000 ]; then
  ENABLE_HIDE=$(magisk --path)/.magisk/modules/riru-core/enable_hide
else
  ENABLE_HIDE=/sbin/.magisk/modules/riru-core/enable_hide
fi

if [ -f $ENABLE_HIDE ] || [ -f /data/adb/riru/enable_hide ]; then
  ui_print "- Hide is enabled"
  touch "$MODPATH"/enable_hide
  set_perm "$MODPATH/enable_hide" 0 0 0700
fi

ui_print "- Removing old files"
rm -rf /data/adb/riru/bin
rm /data/misc/riru/api_version
rm /data/misc/riru/version_code
rm /data/misc/riru/version_name