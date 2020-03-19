#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

BOOTLOADER_OFFSET = 0x1000
PARTITIONS_OFFSET = 0x8000
APP_OFFSET = 0x10000

FLASH_INIT_SIZE = APP_OFFSET


PRE_PATH = "./build/"
PRE_PATH_BOOT = "./build/bootloader/"
PRE_PATH_OUT = "./release_bin/"
BOOTLOADER_NAME = "bootloader.bin"
PARTITIONS_NAME = "partitions.bin"
APP_NAME        = "app.bin"

def add_write(fd_all, fileName, offset):
    fd = open(fileName, "rb")
    fd_all.seek(offset, 0)
    all_data = fd.read()
    fd.close()
    fd_all.write(all_data)

def gen_file(fileName, fileSize):
    try:
        os.unlink(fileName)
    except:
        pass

    fd_all = open(fileName, "wb")
    
    for i in range(0,fileSize):
        fd_all.write("%c"%(0xFF))
        i+= 1

    add_write(fd_all, PRE_PATH_BOOT+BOOTLOADER_NAME, BOOTLOADER_OFFSET)
    add_write(fd_all, PRE_PATH+PARTITIONS_NAME, PARTITIONS_OFFSET)
    add_write(fd_all, PRE_PATH+APP_NAME,        APP_OFFSET)
    
    fd_all.close()
 
def main():
    isExists=os.path.exists(PRE_PATH_OUT)
    if not isExists:
        os.makedirs(PRE_PATH_OUT)
    file_name = PRE_PATH_OUT+'esp32_'+APP_NAME
    file_size = FLASH_INIT_SIZE
    print('genration: %s'%(file_name))
    gen_file(file_name, file_size)

if __name__ == '__main__':
    main()