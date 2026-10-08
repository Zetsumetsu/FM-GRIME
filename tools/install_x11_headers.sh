#!/bin/bash
# Reinstall X11 dev headers needed to build JUCE on Linux.
# /usr/include does NOT survive VM replacement, so run this after a fresh VM
# (symptom: build fails with "fatal error: X11/extensions/Xrandr.h").
set -e
DL=~/workspace/fm-grime/vst-plugin/tools/x11-headers
mkdir -p "$DL"
cd "$DL"
for p in libXrandr-1.5.2 libXinerama-1.1.4 libXcursor-1.2.0; do
  [ -f "$p.tar.gz" ] || wget -q "https://www.x.org/releases/individual/lib/$p.tar.gz"
  [ -d "$p" ] || tar --no-same-owner -xzf "$p.tar.gz"
done
sudo cp libXrandr-1.5.2/include/X11/extensions/Xrandr.h /usr/include/X11/extensions/
sudo cp libXinerama-1.1.4/include/X11/extensions/Xinerama.h /usr/include/X11/extensions/
sudo mkdir -p /usr/include/X11/Xcursor
sudo cp libXcursor-1.2.0/include/X11/Xcursor/Xcursor.h /usr/include/X11/Xcursor/
echo "X11 headers installed"
