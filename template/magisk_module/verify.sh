##########################################################################################
#
# Verifier for Magisk Module Installer
#
##########################################################################################
##########################################################################################
# 
# Instructions:
# 
# 1. Copy the code below to .git/hooks/pre-commit or execute it manually every time
# 2. Replace unzip xxx with vunzip in install.sh
# 3. To see the output of verification, remove >&2 from the end of unzip
#
# #!/bin/sh
# find -type f \
#   -not -path "./.git/*" \
#   -not -path "./.git*" \
#   -not -path "./META-INF/*" \
#   -not -path "./README.md" \
#   -not -name "*.sha256sum" \
#   -exec echo "Generating {}.sha256sum" \; \
#   -exec sh -c "sha256sum -b {} | cut -d \" \" -f 1 | tr -d '\n' > {}.sha256sum" \; \
#   -exec git add "{}.sha256sum" \;
#
##########################################################################################

TMPDIR_FOR_VERIFY=$TMPDIR/.vunzip

abort_verify() {
  ui_print "*********************************************************"
  ui_print "! $1"                                                     
  ui_print "! This zip may be corrupted, please try downloading again"
  abort    "*********************************************************"
}

verify_and_move() {
  # Ignore .sha256sum file itself
  if [[ $1 = *.sha256sum ]]; then
      ui_print "- Ignore $1.sha256sum"
    return
  fi
  
  file=$TMPDIR_FOR_VERIFY/$1
  if [[ ! -f $file.sha256sum ]]; then
    unzip -o -j $ZIPFILE "$1.sha256sum" -d $2 >&2
  fi
  
  if [[ ! -f $file.sha256sum ]]; then
    abort_verify "$1.sha256sum not exists"
    return
  fi
  
  (echo "$(cat $file.sha256sum)  $file" | sha256sum -c -s -) || abort_verify "Failed to verify $1"
  
  ui_print "- Verified $1" >&1

  rm -f $file.sha256sum
  
  dir=$3
  restore_paths=$4
  
  if [[ $restore_paths = true ]]; then
    mkdir -p "$(dirname "$dir/$1")"
    mv $file $dir/$1
  else
      mkdir -p $dir
    mv $file $dir/"$(basename $1)"
  fi
}

verify_and_move_recurse() {
  for file in $1/*; do
    if [ -d $file ];then
      verify_and_move_recurse $file $2 $3 $4
    elif [ -f $file ]; then
      verify_and_move $(echo $file | cut -c $2-) $1 $3 $4
    fi
  done
}

vunzip() {
  rm -rf $TMPDIR_FOR_VERIFY
  mkdir $TMPDIR_FOR_VERIFY
  
  new_args=""
  dir_exists=0
  dir=""
  restore_paths=true
  
  for arg in "$@"
  do  
    # To get .sha256sum file from original structure, ignore -j
    if [[ $arg = "-j" ]]; then
      restore_paths=false
    else
      # Change -d path to our tmp path
      if [[ $dir_exists -eq 1 ]]; then
        dir_exists=2
        dir=$arg
        new_args=$(eval echo $new_args $TMPDIR_FOR_VERIFY)
      else
        new_args=$(eval echo $new_args $arg)
      fi
      if [[ $arg = "-d" ]]; then
        dir_exists=1
      fi
    fi
  done
  
  # Use pwd if -d not set
  if [[ $dir_exists -eq 0 ]]; then
    dir=$(pwd)
    new_args=$(eval echo $new_args -d $TMPDIR_FOR_VERIFY)
  fi
  
  # Call real unzip
  eval unzip $new_args >&2
  
  len=${#TMPDIR_FOR_VERIFY}
  let len=len+2
  
  verify_and_move_recurse $TMPDIR_FOR_VERIFY $len $dir $restore_paths
}