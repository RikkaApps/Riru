:copy_files
xcopy %MODULE_NAME%\template_override\config.sh %TMP_DIR_MAGISK%
xcopy %MODULE_NAME%\template_override\module.prop %TMP_DIR_MAGISK%

mkdir %TMP_DIR_MAGISK%\common
xcopy %MODULE_NAME%\template_override\post-fs-data.sh %TMP_DIR_MAGISK%\common

move %TMP_DIR_MAGISK%\system\lib\zygote_restart %TMP_DIR_MAGISK%\zygote_restart_arm
move %TMP_DIR_MAGISK%\system\lib64\zygote_restart %TMP_DIR_MAGISK%\zygote_restart_arm64
move %TMP_DIR_MAGISK%\system_x86\lib\zygote_restart %TMP_DIR_MAGISK%\zygote_restart_x86
move %TMP_DIR_MAGISK%\system_x86\lib64\zygote_restart %TMP_DIR_MAGISK%\zygote_restart_x64