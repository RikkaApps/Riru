#!/system/bin/sh
MODDIR=${0%/*}

# Rename .new file
move_new_file() {
  if [ -f "$1.new" ]; then
    rm "$1"
    mv "$1.new" "$1"
  fi
}
move_new_file "$MODDIR/rirud"
move_new_file "$MODDIR/rirud.dex"

# Start daemon which runs socket "rirud"
exec "$MODDIR"/rirud