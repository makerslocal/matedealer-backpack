#!/usr/bin/env bash

NAME="$1"

#terrible build script
~/Downloads/arduino-1.*/arduino *.ino
echo    "===================="
read -p "hit return to upload " && 
curl -v#F "image=@matedealer-backpack.ino.generic.bin" http://$NAME/update
echo

#echo    "==================="
#read -p "hit return to reset" && timeout 2 curl -v http://$NAME/debug/reset
