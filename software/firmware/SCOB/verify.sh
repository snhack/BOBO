#!/bin/bash
# Command line wrapper for Arduino
P1="/Applications/Arduino.app/Contents/MacOS/Arduino"
P2="/Applications/Arduino 2.app/Contents/MacOS/Arduino"
CMD="--verify --board arduino:avr:pro:cpu=16MHzatmega328 --pref build.path=~/tmp  $PWD/SCOB.ino"
if [ -e "$P1" ]; then
    echo "Found: $P2"
    "$P1" $CMD
fi
if [ -e "$P2" ]; then
    echo "Found: $P2"
    "$P2" $CMD
fi
