##########################################################################################
#
# Magisk Module Template Config Script
# by topjohnwu
#
##########################################################################################
##########################################################################################
#
# Instructions:
#
# 1. Place your files into system folder (delete the placeholder file)
# 2. Fill in your module's info into module.prop
# 3. Configure the settings in this file (config.sh)
# 4. If you need boot scripts, add them into common/post-fs-data.sh or common/service.sh
# 5. Add your additional or modified system properties into common/system.prop
#
##########################################################################################

##########################################################################################
# Configs
##########################################################################################

# Set to true if you need to enable Magic Mount
# Most mods would like it to be enabled
AUTOMOUNT=true

# Set to true if you need to load system.prop
PROPFILE=false

# Set to true if you need post-fs-data script
POSTFSDATA=true

# Set to true if you need late_start service script
LATESTARTSERVICE=false

##########################################################################################
# Installation Message
##########################################################################################

# Set what you want to show when installing your mod

print_modname() {
  ui_print "***********"
  ui_print "Riru - Core"
  ui_print "***********"
}

##########################################################################################
# Replace list
##########################################################################################

# List all directories you want to directly replace in the system
# Check the documentations for more info about how Magic Mount works, and why you need this

# This is an example
REPLACE="
/system/app/Youtube
/system/priv-app/SystemUI
/system/priv-app/Settings
/system/framework
"

# Construct your own list here, it will override the example above
# !DO NOT! remove this if you don't need to replace anything, leave it empty as it is now
REPLACE="
"

##########################################################################################
# Permissions
##########################################################################################

set_permissions() {
  # Only some special files require specific permissions
  # The default permissions should be good enough for most cases

  # Here are some examples for the set_perm functions:

  # set_perm_recursive  <dirname>                <owner> <group> <dirpermission> <filepermission> <contexts> (default: u:object_r:system_file:s0)
  # set_perm_recursive  $MODPATH/system/lib       0       0       0755            0644

  # set_perm  <filename>                         <owner> <group> <permission> <contexts> (default: u:object_r:system_file:s0)
  # set_perm  $MODPATH/system/bin/app_process32   0       2000    0755         u:object_r:zygote_exec:s0
  # set_perm  $MODPATH/system/bin/dex2oat         0       2000    0755         u:object_r:dex2oat_exec:s0
  # set_perm  $MODPATH/system/lib/libart.so       0       0       0644

  # The following is default permissions, DO NOT remove
  set_perm_recursive  $MODPATH  0  0  0755  0644
}

##########################################################################################
# Custom Functions
##########################################################################################

# This file (config.sh) will be sourced by the main flash script after util_functions.sh
# If you need custom logic, please add them here as functions, and call these functions in
# update-binary. Refrain from adding code directly into update-binary, as it will make it
# difficult for you to migrate your modules to newer template versions.
# Make update-binary as clean as possible, try to only do function calls in it.
check_architecture() {
  if [[ "$ARCH" != "arm" && "$ARCH" != "arm64" && "$ARCH" != "x86" && "$ARCH" != "x64" ]]; then
    ui_print "- Unsupported platform: $ARCH"
    exit 1
  else
    ui_print "- Device platform: $ARCH"
  fi
}

copy_file_from() {
  FROM_PATH=$1
  FULL_PATH="/system/$FROM_PATH/libmemtrack_real.so"
  if [[ -f "$FULL_PATH" ]]; then
    ui_print "- Found $FULL_PATH"
  else
    FULL_PATH="/system/$FROM_PATH/libmemtrack.so"
    if [[ -f "$FULL_PATH" ]]; then
      ui_print "- Found $FULL_PATH"
	else
      ui_print "- $FULL_PATH not found"
      exit 1
    fi
  fi
  
  cp "$FULL_PATH" "$MODPATH/system/$FROM_PATH/libmemtrack_real.so" || exit 1
  return 0
}

copy_files() {
  mkdir -p "/data/misc/riru/modules"
  
  if [[ "$ARCH" == "x86" || "$ARCH" == "x64" ]]; then
    ui_print "- Removing arm/arm64 libraries"
    rm -rf "$MODPATH/system/lib"
    rm -rf "$MODPATH/system/lib64"
	ui_print "- Extracting x86/64 libraries"
	unzip -o "$ZIP" 'system_x86/*' -d $MODPATH >&2
    mv "$MODPATH/system_x86/lib" "$MODPATH/system/lib"
    mv "$MODPATH/system_x86/lib64" "$MODPATH/system/lib64"
  fi
  
  if [[ "$IS64BIT" = true ]]; then
    copy_file_from lib64
  else
    ui_print "- Removing 64-bit libraries"
	rm -rf "$MODPATH/system/lib64"
  fi
  copy_file_from lib
  
  ui_print "- Extracting zygote_restart executable"
  mkdir -p "/data/misc/riru/bin"
  unzip -j "$ZIP" "zygote_restart_$ARCH" -d "/data/misc/riru/bin" >&2
  mv "/data/misc/riru/bin/zygote_restart_$ARCH" "/data/misc/riru/bin/zygote_restart"
  set_perm "/data/misc/riru/bin/zygote_restart" 0 0 0700 u:object_r:system_file:s0

  ui_print "- Writing api_version file"
  echo -n "3" > "/data/misc/riru/api_version"
}