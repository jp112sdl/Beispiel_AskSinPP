#!/bin/sh
ADDON=HB-UNI-Sen-PRESS
rm $ADDON-addon.tgz
find . -name ".DS_Store" -exec rm -rf {} \;
cd $ADDON-addon-src
chmod +x update_script
chmod +x addon/install*
chmod +x addon/update-check.cgi
chmod +x rc.d/*
tar -zcvf ../$ADDON-addon.tgz *
cd ..
