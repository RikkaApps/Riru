#!/system/bin/sh
MODDIR=${0%/*}

# Chmod again just in case
chmod 700 "$MODDIR"/rirud

# Start daemon which runs socket "rirud"
exec "$MODDIR"/rirud