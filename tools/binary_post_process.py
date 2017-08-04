#!/usr/bin/env python3

"""
@file binary_post_process.py
@brief Add hash and length information to binary file, allow integrity of executable to be verified on device
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date March, 2017
"""

import hashlib
import argparse

zero = 0

class Application:
    header_addr = (0x00, 0x20)
    version_string_addr = (0x20, 0x40)
    code_addr = 0x40

class Bootloader:
    header_addr = (0x1c, 0x3c)
    version_string_addr = (0x3c, 0x5c)
    code_addr = 0x5c

class Partition:
    def __init__(self, offset):
        self.header_addr = (offset + 0x00, offset + 0x20)
        self.version_string_addr = (offset + 0x20, offset + 0x40)
        self.code_addr = offset + 0x40

def dscrc8_byte(crc, value):
    crc = crc ^ value
    for i in range(8):
        if crc & 0x01:
            crc = (crc >> 1) ^ 0x8c
        else:
            crc >>= 1
    return crc


def binary_post_process(file, offset, partitionindex):
    # open file
    fp = open(file, "rb")
    binary = fp.read()
    fp.close()

    partition = Partition(offset)

    # extract footprint and version information from binary
    header = binary[partition.header_addr[0]:partition.header_addr[1]]
    version_string = binary[partition.version_string_addr[0]:partition.version_string_addr[1]]
    code = binary[partition.code_addr:]

    # if the header is not empty (possibly the linker script went wrong)
    if header != zero.to_bytes(0x20, byteorder="little"):
        print("Wrong binary file is being post processed!")
        exit(-1)

    # calculate the total length of the code
    total_length = len(binary)
    total_length_bytes = total_length.to_bytes(4, byteorder="little")

    header_length = 64 # bytes
    header_length_bytes = header_length.to_bytes(1, byteorder="little")

    # calculate hash of version_string
    crc = 0
    for byte in version_string:
        crc = dscrc8_byte(crc, byte)

    crc_bytes = crc.to_bytes(1, byteorder="little")

    # hash code
    m = hashlib.md5()
    m.update(code)
    hash_bytes = m.digest()

    # write to binary file
    fp = open(file, "r+b")

    # write total length (0x0-0x4)
    fp.seek(partition.header_addr[0] + 0x0)
    fp.write(total_length_bytes)

    # write header length (0x4-0x8)
    fp.seek(partition.header_addr[0] + 0x4)
    fp.write(header_length_bytes)

    # write crc for version string
    fp.seek(partition.header_addr[0] + 0x5)
    fp.write(crc_bytes)

    # write firmware partition index (0=A, 1=B)
    fp.seek(partition.header_addr[0] + 0x6)
    fp.write(partitionindex.to_bytes(1, byteorder="little"))
    
    # 0x7 reserved

    # write hash
    fp.seek(partition.header_addr[0] + 0x10)
    fp.write(hash_bytes)

    fp.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("-f", "--file", nargs=1, type=str,
        help="file to be processed")

    parser.add_argument("-o", "--offset", nargs=1, default=0x00, type=int,
        help="the relative offset of program header address")

    parser.add_argument("-p", "--partition", nargs=1, type=int,
        help="partition to be written: [0]: Application Region A. [1]: Application Region B. [2]: Bootloader Region. [3]: Reserved Region")

    args = parser.parse_args()

    binary_post_process(args.file[0], args.offset[0], args.partition[0])
