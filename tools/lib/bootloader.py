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

    INST_SET_BASE_ADDR = 0x0
    INST_GET_BASE_ADDR = 0x1
    INST_PUSH_BASE_ADDR = 0x2
    INST_POP_BASE_ADDR = 0x3

    INST_SET_BLOCK_EXP = 0x4
    INST_GET_BLOCK_EXP = 0x5

    INST_BRANCH_TO_ADDR = 0x6

    INST_ERASE_PAGES = 0x7
    INST_ERASE_RANGE = 0x8

    INST_REBOOT = 0x9

    INST_QUERY_PROTO_VER = 0xfc
    INST_QUERY_DEVICE_INFO = 0xfd
    INST_QUERY_CHIP_INFO = 0xfe

    INST_INVALID = 0xff
