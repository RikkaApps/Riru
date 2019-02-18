function copy_files {
  cp $MODULE_NAME/template_override/config.sh $TMP_DIR_MAGISK
  cp $MODULE_NAME/template_override/module.prop $TMP_DIR_MAGISK
  
  mkdir -p $TMP_DIR_MAGISK/common
  cp $MODULE_NAME/template_override/post-fs-data.sh $TMP_DIR_MAGISK/common/post-fs-data.sh
  
  mv $TMP_DIR_MAGISK/system/lib/zygote_restart $TMP_DIR_MAGISK/zygote_restart_arm
  mv $TMP_DIR_MAGISK/system/lib64/zygote_restart $TMP_DIR_MAGISK/zygote_restart_arm64
  mv $TMP_DIR_MAGISK/system_x86/lib/zygote_restart $TMP_DIR_MAGISK/zygote_restart_x86
  mv $TMP_DIR_MAGISK/system_x86/lib64/zygote_restart $TMP_DIR_MAGISK/zygote_restart_x64
}