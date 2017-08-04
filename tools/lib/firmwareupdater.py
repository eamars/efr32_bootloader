from lib.watchdog import Watchdog, WatchdogCallbackObject
from lib.threadsafedict import ThreadSafeDict
from threading import Semaphore, Thread
from lib.bootloader import Bootloader
from progressbar import ProgressBar # pip install progressbar

import time
import os

class FirmwareUpdaterConfiguration:
    transmit_window_size = 4
    watchdog_period = 0.2
    block_size_exponential = 7

    max_timeout = 50 # ticks
    max_retries = 100


class SerialDevice:
    def __init__(self):
        pass

    def write(self, data):
        raise Exception("Not implemented")

    def read(self):
        raise Exception("Not implemented")

    def isConnected(self):
        raise Exception("Not implemented")

    def disconnect(self):
        raise Exception("Not implemented")


class PollDevice(Thread):
    def __init__(self, device, decoder):
        super(PollDevice, self).__init__()
        self.device = device
        self.decoder = decoder
        self.setDaemon(True)

        self.start()

    def run(self):
        while self.device.isConnected():
            ch = self.device.read()
            if ch != '':
                self.decoder.poll(ch)


class FirmwareUpdater(WatchdogCallbackObject):
    # constants
    DATA_KEY = 0
    TIMEOUT_KEY = 1
    RETRIES_KEY = 2

    # progress bar
    progressBar = ProgressBar().start()

    def __init__(self, args):
        WatchdogCallbackObject.__init__(self)

        self.firmware = args["firmware"]
        self.sip = args["sip"]
        self.uart_device = args["uart_device"]
        self.config = args["config"]

        # packet index is used for acknowledgement. The resolution for packet index should at least larger
        # than transmit window
        self.__packetIndex__ = 0

        # the transmit window is maintained by a list of unacknowledged packets
        self.unAckedPacketList = ThreadSafeDict()

        # semaphore is used to maintain a fixed size of transmit window
        self.window = Semaphore(self.config.transmit_window_size)

    def start(self):
        # create a watchdog timer object
        self.watchdog = Watchdog(self.config.watchdog_period, self)
        self.watchdog.start()

        # queueing packet, block when window is not moved
        for partition, program in self.firmware:
            # select partition and block size exponential
            self.write_sip(
                Bootloader.COMM_INST,
                3,
                "{:02x}{:02x}{:02x}".format(
                    Bootloader.INST_SET_REGION,
                    partition,
                    self.config.block_size_exponential
                )
            )

            codeSize = len(program)
            for codeIndex, codeLine in enumerate(program):
                # send program data
                payload = "{}{}".format(
                    self.to_litten_endian_hexstring("{:04x}".format(codeIndex)),
                    "".join(map(str.format, ["{:02x}"]*len(codeLine), map(ord, codeLine)))
                )
                self.write_sip(Bootloader.COMM_DATA, len(payload) // 2, payload)

                # print progress bar
                self.progressBar.update(codeIndex * 100 // codeSize)

            # flush memory
            self.write_sip(
                Bootloader.COMM_INST,
                2,
                "{:02x}{:02x}".format(
                    Bootloader.INST_FLUSH,
                    partition
                )
            )

        # wait for device to disconnect
        while self.uart_device.isConnected():
            time.sleep(0.5)

        print("Upgrade complete")

        self.uart_device.disconnect()
        os._exit(0)

    def to_litten_endian_hexstring(self, s1):
        '''Reference: http://stackoverflow.com/a/5864313'''
        return "".join(reversed([s1[i:i+2] for i in range(0, len(s1), 2)]))

    def watchdog_handler(self, arg):
        # schedule next event
        self.watchdog.reset()

        issueRebootCommand = False

        # critical section
        with self.unAckedPacketList as ackList:
            # if no scheduled event in list, then reboot device
            if len(ackList) == 0:
                # reboot to partition
                issueRebootCommand = True

            # otherwise transmit anything left
            else:
                for key in ackList:
                    # print("ACKNO:", key, "Timeout:", ackList[key][self.TIMEOUT_KEY], "Retries:", ackList[key][self.RETRIES_KEY])
                    ackList[key][self.TIMEOUT_KEY] -= 1

                    # if the timeout for the packet has expired
                    if ackList[key][self.TIMEOUT_KEY] == 0:
                        # if number of retries exceeded maximum threshold, then quit
                        ackList[key][self.RETRIES_KEY] -= 1
                        if ackList[key][self.RETRIES_KEY] == 0:
                            print("Fatal: exceed maximum retries:", key, ackList[key][self.DATA_KEY])
                            os._exit(0)

                        # otherwise, reset the timeout and retry
                        ackList[key][self.TIMEOUT_KEY] = self.config.max_timeout
                        self.__write__(ackList[key][self.DATA_KEY])

        # outside the critical section
        if issueRebootCommand:
            self.write_sip(Bootloader.COMM_INST, 2, "{:02x}{:02x}".format(Bootloader.INST_BRANCH_TO_REGION, self.firmware.reboot_to_region))

    def write_sip(self, command, payload_len, payload):
        msg = "<{:01x}{:02x}{}{}>".format(
            command,
            payload_len + 2, # include packet index length
            self.to_litten_endian_hexstring("{:04x}".format(self.__packetIndex__)),
            payload
        )

        self.write_new_packet(msg)

    def write_new_packet(self, data):
        # attempt to transmit new packet if there is place available for the new packet
        self.window.acquire()

        with self.unAckedPacketList as ackList:
            ackList[self.__packetIndex__] = {
                self.DATA_KEY: data,
                self.TIMEOUT_KEY: self.config.max_timeout,
                self.RETRIES_KEY: self.config.max_retries
            }

        self.__write__(data)

        # increase packet index count (used for ARQ protocol)
        self.__packetIndex__ += 1

    def __write__(self, data):
        self.uart_device.write(data)

    def __call__(self, **kwargs):
        '''
        Callback function
        The function is called when a full SIP message is received
        '''
        # read acknowledgement number
        ackno = (self.sip.get(1) << 8) + self.sip.get(0)

        # read window size
        rx_window = (self.sip.get(3) << 8) + self.sip.get(2)

        # critical section
        with self.unAckedPacketList as ackList:
            if ackno in ackList:
                del ackList[ackno]

                # allow next packet to be transmitted
                self.window.release()


if __name__ == "__main__":
    pass
