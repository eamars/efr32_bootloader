#!/usr/bin/env python2
#
'''
@file btle_updater.py
@brief Firmware updater via bluetooth low energy controller
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date June, 2017

Simple firmware update for Hatch Device by Bluetooth
'''
import sys

import lib.btuart as btuart
from lib.firmwarepackage import FirmwarePackage
from lib.firmwareupdater import SerialDevice
from lib.sip import SerialInterfaceProtocol
from lib.bootloader import Bootloader
from lib.simpleupdater import SimpleUpdater, Errno



class BluetoothLowEnergyUartDevice(SerialDevice):
    def __init__(self, addr, addrType, name):
        SerialDevice.__init__(self)

        self.uart_device = btuart.UART(addr=addr, addrType=addrType, name=name)
        self.uart_device.printNotify = False
        self.uart_device.startNotify()

    def read(self):
        return self.uart_device.read()

    def write(self, data):
        self.uart_device.write(data)

    def isConnected(self):
        return self.uart_device.connected

    def disconnect(self):
        self.uart_device.disconnect()



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
    btle_device = BluetoothLowEnergyUartDevice(addr, "random", "Hatch")

    # create packet decoder
    decoder = SerialInterfaceProtocol()

    # enter update process
    updater = SimpleUpdater(firmware, btle_device, decoder)

    # register callback
    decoder.registerCallbackObject(0x1, updater)

    updater.main()



if __name__ == "__main__":
    main()
