RIRU_PATH="/data/misc/riru"

check_riru_version() {
  [[ ! -f "$RIRU_PATH/api_version" ]] && abort "! Please Install Riru - Core v19 or above"
  VERSION=$(cat "$RIRU_PATH/api_version")
  ui_print "- Riru API version is $VERSION"
  [[ "$VERSION" -ge 4 ]] || abort "! Please Install Riru - Core v19 or above"
}

check_architecture() {
  if [[ "$ARCH" != "arm" && "$ARCH" != "arm64" && "$ARCH" != "x86" && "$ARCH" != "x64" ]]; then
    abort "! Unsupported platform: $ARCH"
  else
    ui_print "- Device platform: $ARCH"
  fi
}

check_architecture
check_riru_version

if [[ "$ARCH" == "x86" || "$ARCH" == "x64" ]]; then
ui_print "- Copying x86/64 libraries"
rm -rf "${MODPATH}/system/lib"
rm -rf "${MODPATH}/system/lib64"
mv "$MODPATH/system_x86/lib" "$MODPATH/system/lib"
mv "$MODPATH/system_x86/lib64" "$MODPATH/system/lib64"
fi

if [[ "$IS64BIT" = false ]]; then
ui_print "- Removing 64-bit libraries"
rm -rf "$MODPATH/system/lib64"
fi

ui_print "- Copying extra files"

TARGET="${RIRU_PATH}/modules/%%%ID%%%"

[[ -d "${TARGET}" ]] || mkdir -p "${TARGET}" || abort "! Can't mkdir -p ${TARGET}"

cp "${MODPATH}/module.prop" "${TARGET}/module.prop" || abort "! Can't create ${TARGET}/module.prop"

ui_print "- Files copied"
