#!/bin/bash
IPHONE_HOST="localhost"
SSHPORT=2222

mkdir -p cydia/usr/lib

cp isslfix.dylib cydia/usr/lib
cp postinst extrainst_ prerm cydia/DEBIAN

ssh -p $SSHPORT root@$IPHONE_HOST 'rm -rf /var/root/cydia'
scp -P $SSHPORT -r cydia root@$IPHONE_HOST:/var/root
ssh -p $SSHPORT root@$IPHONE_HOST 'chmod -R 644 cydia ; chmod -R 755 cydia/DEBIAN; dpkg-deb -b cydia'
scp -P $SSHPORT root@$IPHONE_HOST:/var/root/cydia.deb isslfix.deb

