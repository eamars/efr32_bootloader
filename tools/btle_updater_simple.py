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

    def main(self):
        for partition, program in self.firmware:
            # select partition and block size exponential
            self.writePacket(
                Bootloader.COMM_INST,
                3,
                "{:02x}{:02x}{:02x}".format(
                    Bootloader.INST_SET_REGION,
                    partition,
                    BLOCK_SIZE_EXPONENTIAL
                )
            )

            codeSize = len(program)
            for codeIndex, codeLine in enumerate(program):
                # send program data
                payload = "{}{}".format(
                    self.__toLittleEndianHexString("{:04x}".format(codeIndex)),
                    "".join(map(str.format, ["{:02x}"]*len(codeLine), map(ord, codeLine)))
                )
                self.writePacket(Bootloader.COMM_DATA, len(payload) // 2, payload)

                # print progress bar
                self.__progressBar.update(codeIndex * 100 // codeSize)

            # flush memory
            self.writePacket(
                Bootloader.COMM_INST,
                2,
                "{:02x}{:02x}".format(
                    Bootloader.INST_FLUSH,
                    partition
                )
            )

        # boot to region
        self.__isLastPacket = True
        self.writePacket(Bootloader.COMM_INST, 2, "{:02x}{:02x}".format(Bootloader.INST_BRANCH_TO_REGION, self.firmware.reboot_to_region))

        # wait for device to disconnect
        counter = 10
        while self.device.isConnected():
            counter -= 1
            if counter == 0:
                break;
            time.sleep(0.5)

        print("You are not suppose to be here, device failed to reboot")


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
