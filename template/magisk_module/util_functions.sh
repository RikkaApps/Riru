RIRU_MIN_COMPATIBLE_API=9
RIRU_API="%%%RIRU_API%%%"
RIRU_VERSION_CODE="%%%RIRU_VERSION_CODE%%%"
RIRU_VERSION_NAME="%%%RIRU_VERSION_NAME%%%"

abort_for_requires_new_version() {
  ui_print "*********************************************************"
  ui_print "! This module requires Riru $1 or above"
  ui_print "! Please install (upgrade) Riru from Magisk Manager"
  ui_print "! Or you can download zip from https://github.com/RikkaApps/Riru/releases"
  abort "*********************************************************"
}

abort_for_old_module() {
  ui_print "*********************************************************"
  ui_print "! This module haven't support Riru v22.0+"
  ui_print "! Please ask the developer of this module to make changes"
  abort "*********************************************************"
}

check_riru_version() {
  ui_print "- Riru: $RIRU_VERSION_NAME ($RIRU_VERSION_CODE, API $RIRU_API)"
  [ "$RIRU_MODULE_API_VERSION" -lt "$RIRU_MIN_COMPATIBLE_API" ] && abort_for_old_module
  [ "$RIRU_MODULE_MIN_API_VERSION" -gt "$RIRU_API" ] && abort_for_requires_new_version "$RIRU_MODULE_MIN_RIRU_VERSION_NAME"
}

if $BOOTMOE; then
  ui_print "- Installing from Magisk app"
else
  ui_print "*********************************************************"
  ui_print "! Install from recovery is NOT supported"
  ui_print "! Some recovery has broken implementations, install with such recovery will finally cause Riru or Riru modules not working"
  ui_print "! Please install from Magisk app"
  abort "*********************************************************"
fi