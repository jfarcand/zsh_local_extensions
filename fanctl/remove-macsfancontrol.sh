#!/bin/sh
# One-time cleanup: remove Macs Fan Control and its root helper.
set -u
launchctl bootout system/com.crystalidea.macsfancontrol.smcwrite 2>/dev/null
rm -f /Library/LaunchDaemons/com.crystalidea.macsfancontrol.smcwrite.plist \
      /Library/PrivilegedHelperTools/com.crystalidea.macsfancontrol.smcwrite
rm -rf "/Applications/Macs Fan Control.app"
echo "Macs Fan Control removed"
