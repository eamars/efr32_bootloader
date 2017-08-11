#!/usr/bin/env python3

"""
@file binary_post_process.py
@brief Add hash and length information to binary file, allow integrity of executable to be verified on device
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date Aug, 2017
"""

import struct
import datetime
import argparse
from lib.crc import crc32, CRC32_START

AAT_BEGIN_ADDR = 0x100
BASIC_HEADER_SIZE = 24

class ApplicationHeader:
    template = "<IIIIHHIIIIIII"

    def __init__(self):
        # basic application header
        self.stack_top = 0
        self.reset_handler = 0
        self.nmi_handler = 0
        self.hardfault_handler = 0

        # extended application header
        self.type = 0
        self.version = 0
        self.vector_table = 0

        self.basic_app_header_table_crc = 0
        self.ext_header_version = 0
        self.app_total_size = 0
        self.header_size = 0

        self.app_version = 0
        self.timestamp = 0

    def pack(self):
        return struct.pack(self.template,
                           self.stack_top,
                           self.reset_handler,
                           self.nmi_handler,
                           self.hardfault_handler,
                           self.type,
                           self.version,
                           self.vector_table,
                           self.basic_app_header_table_crc,
                           self.ext_header_version,
                           self.app_total_size,
                           self.header_size,
                           self.app_version,
                           self.timestamp)

    def unpack(self, binary):
        fmt_size = struct.calcsize(self.template)
        buffer = struct.unpack(self.template, binary[:fmt_size])

        self.stack_top = buffer[0]
        self.reset_handler = buffer[1]
        self.nmi_handler = buffer[2]
        self.hardfault_handler = buffer[3]

        self.type = buffer[4]
        self.version = buffer[5]
        self.vector_table = buffer[6]

        self.basic_app_header_table_crc = buffer[7]
        self.ext_header_version = buffer[8]

        self.app_total_size = buffer[9]
        self.header_size = buffer[10]

        self.app_version = buffer[11]
        self.timestamp = buffer[12]

    def __len__(self):
        return struct.calcsize(self.template)



def main(path):
    with open(path, "r+b") as fp:
        firmware = fp.read()

    # create header instance
    aat = ApplicationHeader()

    # extract header
    header = firmware[AAT_BEGIN_ADDR: AAT_BEGIN_ADDR + len(aat)]

    # decode header
    aat.unpack(header)

    # fill the basic_app_header_table_crc
    crcVal = CRC32_START
    for index in range(AAT_BEGIN_ADDR, AAT_BEGIN_ADDR + BASIC_HEADER_SIZE):
        crcVal = crc32(firmware[index], crcVal)
    aat.basic_app_header_table_crc = crcVal

    # set app total size
    aat.app_total_size = len(firmware)

    # set timestamp for post process
    aat.timestamp = int(datetime.datetime.utcnow().timestamp())

    # write back
    fp.seek(AAT_BEGIN_ADDR)
    fp.write(aat.pack())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("-f", "--file", nargs=1, type=str,
                        help="path to the file to be processed")


    args = parser.parse_args()

    main(args.file[0])
