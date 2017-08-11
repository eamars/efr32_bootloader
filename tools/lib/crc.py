CRC32_POLYNOMIAL = 0xEDB88320
CRC32_START = 0xFFFFFFFF
CRC32_END = 0xDEBB20E3


def dscrc8_byte(crc, value):
    crc = crc ^ value
    for i in range(8):
        if crc & 0x01:
            crc = (crc >> 1) ^ 0x8c
        else:
            crc >>= 1
    return crc


def crc32(newByte, prevResult):
    previous = (prevResult >> 8) & 0x00FFFFFF
    oper = (prevResult ^ newByte) & 0xFF

    for jj in range(0, 8):
        if oper & 0x01 != 0:
            oper = (oper >> 1) ^ CRC32_POLYNOMIAL
        else:
            oper = oper >> 1

    return previous ^ oper


