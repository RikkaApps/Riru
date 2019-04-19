#!/bin/bash
MODULE_NAME=$1
shift
if [ "$MODULE_NAME" == "" ]; then
  echo "Usage: sh build.sh <module name> [<version name>]"
  exit 1
fi

if [ ! -d "$MODULE_NAME" ]; then
  echo "$MODULE_NAME not exists"
  exit 1
fi

VERSION=$1
shift
[[ "$VERSION" == "" ]] && VERSION=v1

LIBS_OUTPUT=$MODULE_NAME/build/ndkBuild/libs
NDK_OUT=$MODULE_NAME/build/ndkBuild/obj

# build
NDK_BUILD=ndk-build
[[ "$OSTYPE" == "msys" ]] && NDK_BUILD=ndk-build.cmd
[[ "$OSTYPE" == "cygwin" ]] && NDK_BUILD=ndk-build.cmd

$NDK_BUILD -C $MODULE_NAME NDK_LIBS_OUT=build/ndkBuild/libs NDK_OUT=build/ndkBuild/obj $@

# create tmp dir
TMP_DIR=build/zip
TMP_DIR_MAGISK=$TMP_DIR/magisk

rm -rf $TMP_DIR
mkdir -p $TMP_DIR

cp -a template/magisk/. $TMP_DIR_MAGISK

# copy files
mkdir -p $TMP_DIR_MAGISK/system/lib64
mkdir -p $TMP_DIR_MAGISK/system/lib
mkdir -p $TMP_DIR_MAGISK/system_x86/lib64
mkdir -p $TMP_DIR_MAGISK/system_x86/lib
cp -a $LIBS_OUTPUT/arm64-v8a/. $TMP_DIR_MAGISK/system/lib64
cp -a $LIBS_OUTPUT/armeabi-v7a/. $TMP_DIR_MAGISK/system/lib
[[ -d $LIBS_OUTPUT/x86_64 ]] && cp -a $LIBS_OUTPUT/x86_64/. $TMP_DIR_MAGISK/system_x86/lib64
[[ -d $LIBS_OUTPUT/x86 ]] && cp -a $LIBS_OUTPUT/x86/. $TMP_DIR_MAGISK/system_x86/lib

# run custom script
if [ -f $MODULE_NAME/build.sh ]; then
  source $MODULE_NAME/build.sh
  copy_files
fi

# zip
mkdir -p release
ZIP_NAME=magisk-$MODULE_NAME-"$VERSION".zip
rm -f release/$ZIP_NAME
rm -f $TMP_DIR_MAGISK/$ZIP_NAME
(cd $TMP_DIR_MAGISK; zip -r $ZIP_NAME * > /dev/null)
mv $TMP_DIR_MAGISK/$ZIP_NAME release/$ZIP_NAME

# clean tmp dir
rm -rf $TMP_DIR
