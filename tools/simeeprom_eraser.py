#!/usr/bin/env python3

import sys
import time

from lib.bootloader import Bootloader
from lib.firmwareupdater import SerialDevice
from lib.ioserial import IOSerial
from lib.sip import SerialInterfaceProtocol

POLL_PERIOD = 0.01

class Errno:
    NO_ERROR = 0
    DEVICE_DISCONNECTED = 1
    DEVICE_NO_RESPONSE = 2
    INVALID_ARGUMENT = 3

class SerialUartDevice(SerialDevice):
    def __init__(self, port):
        SerialDevice.__init__(self)

        self.uart_device = IOSerial(port)
        self.uart_device.blockingAttach()

    def read(self):
        return self.uart_device.readchar(0.001)

    def write(self, data):
        self.uart_device.write(data)

    def isConnected(self):
        return self.uart_device.isAttached()

    def disconnect(self):
        self.uart_device.detach()


class SimEEPROMEraser():
    def __init__(self, device):
        self.device = device

    def __call__(self, **kwargs):
        pass

    def __toLittleEndianHexString(self, s1):
        """Reference: http://stackoverflow.com/a/5864313"""
        return "".join(reversed([s1[i:i+2] for i in range(0, len(s1), 2)]))

    def __write(self, data):
        self.device.write(data.encode("ascii"))
        print("Transmit:", data)

    def writePacket(self, command, payload_len, payload):
        """Include packet index and transmit over serial interface"""
        msg = "<{:01X}{:02X}{}{}>".format(
            command,
            payload_len + 2, # include packet index length
            self.__toLittleEndianHexString("{:04X}".format(0)),
            payload
        )

        self.__write(msg)

    def main(self):
        # <20B00000800E00F0000001000>
        self.writePacket(
            Bootloader.COMM_INST,
            9,
            "{:02x}{}{}".format(
                Bootloader.INST_ERASE_RANGE,
                self.__toLittleEndianHexString("{:08X}".format(0x0FE000)),
                self.__toLittleEndianHexString("{:08X}".format(0x100000)),
            )
        )

        response = ''
        while response.lower() != "<105000001002A>".lower():
            ch = self.device.read().decode("ascii")
            if ch != '':
                response += ch
            else:
                time.sleep(1)

        self.device.disconnect()

        print("Receive:", response)

def main():
    addr = ""
    path = ""

    # read argument
    if (len(sys.argv) > 1):
        # create uart object
        addr = sys.argv[1]
    else:
        print("Invalid arguments")
        exit(Errno.INVALID_ARGUMENT)

    # open uart
    uart_device = SerialUartDevice(addr)

    # create packet decoder
    decoder = SerialInterfaceProtocol()

    # enter update process
    eraser = SimEEPROMEraser(uart_device)

    # register callback
    decoder.registerCallbackObject(0x1, eraser)

    eraser.main()



if __name__ == "__main__":
    main()
