#!/bin/sh
rm HB-UNI-Sen-CAP-MOIST-addon.tgz
find . -name ".DS_Store" -exec rm -rf {} \;
cd HB-UNI-Sen-CAP-MOIST-addon-src
chmod +x update_script
chmod +x addon/install*
chmod +x addon/update-check.cgi
chmod +x rc.d/*
tar -zcvf ../HB-UNI-Sen-CAP-MOIST-addon.tgz *
cd ..
