@echo off
set MODULE_NAME=%1
if "%MODULE_NAME%" == "" (
    echo "Usage: cmd /c build.bat <module name> [<version name>]"
    exit 1
)

if not exist "%MODULE_NAME%" (
    echo "%MODULE_NAME% not exists"
    exit 1
)

set VERSION=%2
if "%VERSION%" == "" (
    set VERSION=v1
)

set LIBS_OUTPUT=%MODULE_NAME%\build\ndkBuild\libs
set NDK_OUT=%MODULE_NAME%\build\ndkBuild\obj

:: build
set NDK_BUILD=ndk-build.cmd
call %NDK_BUILD% -C %MODULE_NAME% NDK_LIBS_OUT=build/ndkBuild/libs NDK_OUT=build/ndkBuild/obj

:: create tmp dir
set TMP_DIR=build\zip
set TMP_DIR_MAGISK=%TMP_DIR%\magisk

rd /s /q %TMP_DIR%
mkdir %TMP_DIR_MAGISK%

xcopy template\magisk\. %TMP_DIR_MAGISK% /s

:: copy files
mkdir %TMP_DIR_MAGISK%\system\lib64
mkdir %TMP_DIR_MAGISK%\system\lib
mkdir %TMP_DIR_MAGISK%\system_x86\lib64
mkdir %TMP_DIR_MAGISK%\system_x86\lib
xcopy %LIBS_OUTPUT%\arm64-v8a\. %TMP_DIR_MAGISK%\system\lib64 /s
xcopy %LIBS_OUTPUT%\armeabi-v7a\. %TMP_DIR_MAGISK%\system\lib /s
if exist %LIBS_OUTPUT%\x86_64 (
    xcopy %LIBS_OUTPUT%\x86_64\. %TMP_DIR_MAGISK%\system_x86\lib64 /s
)
if exist %LIBS_OUTPUT%\x86 (
    xcopy %LIBS_OUTPUT%\x86\. %TMP_DIR_MAGISK%\system_x86\lib /s
)

:: run custom script
if exist %MODULE_NAME%\build.bat (
    call %MODULE_NAME%\build.bat
)

:: zip
mkdir release
set ZIP_NAME=magisk-%MODULE_NAME%-"%VERSION%".zip
del release\%ZIP_NAME% /f
del %TMP_DIR_MAGISK%\%ZIP_NAME% /f
cd %TMP_DIR_MAGISK%
winrar a -r %ZIP_NAME% *
cd %~dp0
move %TMP_DIR_MAGISK%\%ZIP_NAME% release

:: clean tmp dir
rd /s /q %TMP_DIR%