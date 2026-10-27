#!/bin/sh
# Build fanctl and run it as a root LaunchDaemon. Usage: sudo ./install.sh [uninstall]
set -eu
cd "$(dirname "$0")"
PLIST=/Library/LaunchDaemons/local.fanctl.plist
[ "$(id -u)" = 0 ] || { echo "run with sudo" >&2; exit 1; }
launchctl bootout system/local.fanctl 2>/dev/null || true
if [ "${1:-}" = uninstall ]; then
  rm -f "$PLIST" /usr/local/bin/fanctl
  echo "fanctl removed; fans are back under SMC control"
  exit 0
fi
clang -O2 -Wall -framework IOKit fanctl.c -o fanctl
mkdir -p /usr/local/bin
install -m 755 -o root -g wheel fanctl /usr/local/bin/fanctl
install -m 644 -o root -g wheel local.fanctl.plist "$PLIST"
launchctl bootstrap system "$PLIST"
sleep 4
/usr/local/bin/fanctl status
tail -3 /var/log/fanctl.log
