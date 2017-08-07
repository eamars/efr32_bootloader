#!/usr/bin/env python3
#
'''
@file serial_updater.py
@brief Firmware updater via serial port
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date May, 2017

Since btuart is only available under Linux, for those developer running macOS, please use FirmwareUpdater instead
'''

import sys

from lib.ioserial import IOSerial
from lib.firmwarepackage import FirmwarePackage
from lib.firmwareupdater import SerialDevice
from lib.sip import SerialInterfaceProtocol
from lib.simpleupdater import SimpleUpdater, Errno


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



def main():
    addr = ""
    path = ""

    # read argument
    if (len(sys.argv) > 2):
        # create uart object
        addr = sys.argv[1]
        path = sys.argv[2]
    else:
        print("Invalid arguments")
        exit(Errno.INVALID_ARGUMENT)

    # open firmware
    firmware = FirmwarePackage(path)

    # open uart
    btle_device = SerialUartDevice(addr)

    # create packet decoder
    decoder = SerialInterfaceProtocol()

    # enter update process
    updater = SimpleUpdater(firmware, btle_device, decoder)

    # register callback
    decoder.registerCallbackObject(0x1, updater)

    updater.main()



if __name__ == "__main__":
    main()
