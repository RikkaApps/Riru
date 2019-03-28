:copy_files
:: /data/misc/riru/modules/template exists -> libriru_template.so will be loaded
:: Change "template" to your module name
:: You can also use this folder as your config folder
set NAME="template"
mkdir %TMP_DIR_MAGISK%\data\misc\riru\modules\%NAME%
xcopy %MODULE_NAME%\template_override\riru_module.prop %TMP_DIR_MAGISK%\data\misc\riru\modules\%NAME%\
move %TMP_DIR_MAGISK%\data\misc\riru\modules\%NAME%\riru_module.prop %TMP_DIR_MAGISK%\data\misc\riru\modules\%NAME%\module.prop

xcopy %MODULE_NAME%\template_override\config.sh %TMP_DIR_MAGISK%
xcopy %MODULE_NAME%\template_override\module.prop %TMP_DIR_MAGISK%