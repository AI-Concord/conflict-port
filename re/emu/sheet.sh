#!/bin/sh
# usage: sheet.sh dir [cols] -> dir/sheet.png
cd "$1" && montage $(ls *.png | grep -v sheet | grep -v final) -tile ${2:-3}x -geometry 640x350+4+4 -label '%t' -background '#222' -fill white sheet.png
