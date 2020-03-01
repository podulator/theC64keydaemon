#!/bin/sh

echo "making filesystem rw" >> /tmp/keymgr.install
/bin/mount -o remount, rw /

echo "copying files" >> /tmp/keymgr.install
/bin/cp -p /mnt/keymgr /usr/bin/keymgr
/bin/cp -p /mnt/S99keymgr /etc/init.d/S99keymgr

echo "setting filesystem back to ro" >> /tmp/keymgr.install
/bin/mount -o remount, ro /

echo "starting the service..." >> /tmp/keymgr.install
/etc/init.d/S99keymgr start

echo "finished" >> /tmp/keymgr.install
