import struct
import argparse

ZIGBEE_HEADER_MANDATORY_FIELD_SIZE = 56
ZIGBEE_TAG_SIZE = 6
ZIGBEE_HEADER_FILE_IDENTIFIER = 0x0BEEF11E
ZIGBEE_HEADER_VERSION = 0x0100
ZIGBEE_HEADER_ZSTACK_VERSION = 0x0002

class ZigbeeTagDataStructure:
    template = "<HI"

    def __init__(self):
        self.id = 0
        self.length = 0

    def pack(self):
        return struct.pack(self.template,
                           self.id,
                           self.length)



class ZigbeeOTAHeaderStructure:
    # template = "<IHHHHHIH32sIB8sHH"
    template = "<IHHHHHIH32sI"

    def __init__(self):
        self.fileIdentifier = 0
        self.headerVersion = 0
        self.headerLength = 0
        self.fieldControl = 0
        self.manufactureId = 0
        self.imageTypeId = 0
        self.firmwareVersion = 0
        self.zigbeeStackVersion = 0
        self.headerString = bytes("", 'ascii')
        self.imageSize = 0
        # self.securityCredentials = 0
        # self.upgradeFileDestination = bytes("", 'ascii')
        # self.minimumHardwareVersion = 0
        # self.maximumHardwareVersion = 0

    def pack(self):
        return struct.pack(self.template,
                           self.fileIdentifier,
                           self.headerVersion,
                           self.headerLength,
                           self.fieldControl,
                           self.manufactureId,
                           self.imageTypeId,
                           self.firmwareVersion,
                           self.zigbeeStackVersion,
                           self.headerString,
                           self.imageSize)
                           # self.securityCredentials,
                           # self.upgradeFileDestination,
                           # self.minimumHardwareVersion,
                           # self.maximumHardwareVersion)

class ZigbeeOTASingleFile:
    # Zigbee OTA file can be represented as
    # [OTA Header][Tag1][----Image #1 Data----][Tag2][----Image #2 Data----]

    def __init__(self, firmware, manufactureId, firmwareVersion, headerString):
        self.header = ZigbeeOTAHeaderStructure()
        self.tag = ZigbeeTagDataStructure()
        self.firmware = firmware

        self.header.manufactureId = manufactureId
        self.header.firmwareVersion = firmwareVersion
        self.header.headerString = headerString

    def pack(self):
        firmware_size = len(self.firmware)

        self.header.fileIdentifier = ZIGBEE_HEADER_FILE_IDENTIFIER
        self.header.headerVersion = ZIGBEE_HEADER_VERSION
        self.header.headerLength = ZIGBEE_HEADER_MANDATORY_FIELD_SIZE
        self.header.fieldControl = 0x0
        self.header.imageTypeId = 0x0
        self.header.zigbeeStackVersion = ZIGBEE_HEADER_ZSTACK_VERSION
        self.header.imageSize = firmware_size + ZIGBEE_TAG_SIZE + ZIGBEE_HEADER_MANDATORY_FIELD_SIZE

        self.tag.id = 0x0
        self.tag.length = firmware_size

        return self.header.pack() + self.tag.pack() + self.firmware

def main(args_input, args_output, args_manufactureId, args_version, args_header_string):

    with open(args_input, "rb") as inFp:
        firmware = inFp.read()
        otaFile = ZigbeeOTASingleFile(firmware, args_manufactureId, args_version, args_header_string.encode('ascii')).pack()

        with open(args_output, "wb") as outFp:
            outFp.write(otaFile)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("-i", "--input", nargs=1, type=str,
                        help="Path to the input firmware")

    parser.add_argument("-o", "--output", nargs=1, type=str,
                        help="Path to the output OTA image")

    parser.add_argument("-m", "--manufactureId", nargs=1, type=int,
                        help="Set manufacture Id")

    parser.add_argument("-v", "--version", nargs=1, type=int,
                        help="Set version number")

    parser.add_argument("-s", "--header_string", nargs=1, type=str,
                        help="Set header string")

    args = parser.parse_args()

    main(args.input[0], args.output[0], args.manufactureId[0], args.version[0], args.header_string[0])
