#!/usr/bin/env python3

'''
@file bootloader.py
@brief provide brief definitions for bootloader implementation
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date May, 2017
'''

class Bootloader:

    COMM_DATA = 0x0
    COMM_ACK = 0x1
    COMM_INST = 0x2

    INST_SET_REGION = 0x1              #/* deprecated */
    INST_BRANCH_TO_REGION = 0x1        #/* deprecated */
    INST_COPY_REGION = 0x2             #/* deprecated */
    INST_FLUSH = 0x3                   #/* deprecated */

    # newly added attributes
    INST_SET_BASE_ADDR = 0x4
    INST_BRANCH_TO_ADDR = 0x5
    INST_PUSH_BASE_ADDR = 0x6
    INST_POP_BASE_ADDR = 0x7
    INST_ERASE_PAGES = 0x8
    INST_ERASE_RANGE = 0x9
    INST_QUERY_PROTO_VER = 0xa
    INST_GET_BASE_ADDR = 0xb

    INST_REBOOT = 0xd
    INST_QUERY_DEVICE_INFO = 0xe
    INST_QUERY_CHIP_INFO = 0xf
