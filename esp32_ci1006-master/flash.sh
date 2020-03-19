#!/bin/bash

PYTHON_CMD="$IDF_PATH/components/esptool_py/esptool/esptool.py --port /dev/ttyUSB0 --baud 921600 write_flash"

BOOTLOADER="0x1000 build/bootloader/bootloader.bin"
PARTITION="0x8000 build/partitions.bin"
APP="0x10000 build/app.bin"
APP2="0x1f0000 build/app.bin"

if [ "$1" = "app" ]; then
    BOOTLOADER=""
    PARTITION=""
elif [ "$1" = "help" ]; then
    echo "eg:"
    echo "./flash.sh           #for all"
    echo "./flash.sh app       #for app"
exit 0
fi

echo $PYTHON_CMD $BOOTLOADER $PARTITION $APP $APP2

$PYTHON_CMD $BOOTLOADER $PARTITION $APP $APP2
