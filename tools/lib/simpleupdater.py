import sys
from progressbar import ProgressBar # pip install progressbar
import time

BLOCK_SIZE_EXPONENTIAL = 7
POLL_PERIOD = 0.01
DEFAULT_TIMEOUT = 1
MAX_RETRY = 5

class Errno:
    NO_ERROR = 0
    DEVICE_DISCONNECTED = 1
    DEVICE_NO_RESPONSE = 2
    INVALID_ARGUMENT = 3

class SimpleUpdater():
    def __init__(self, firmware, device, decoder):
        self.firmware = firmware
        self.device = device
        self.decoder = decoder
        self.__progressBar = ProgressBar().start()

        self.__packetIndex = 0
        self.__isPacketAcked = False
        self.__isLastPacket = False
        self.__currentPacketTimeoutCounter = 0
        self.__currentPacketRetryCounter = 0


    def __call__(self, **kwargs):
        ackno = (self.decoder.get(1) << 8) + self.decoder.get(0)

        if self.__packetIndex == ackno:
            self.__isPacketAcked = True

    def __toLittleEndianHexString(self, s1):
        """Reference: http://stackoverflow.com/a/5864313"""
        return "".join(reversed([s1[i:i+2] for i in range(0, len(s1), 2)]))

    def __write(self, data):
        self.device.write(data)

    def writePacket(self, command, payload_len, payload):
        """Include packet index and transmit over serial interface"""
        msg = "<{:01x}{:02x}{}{}>".format(
            command,
            payload_len + 2, # include packet index length
            self.__toLittleEndianHexString("{:04x}".format(self.__packetIndex)),
            payload
        )

        # set a timeout for specific packet
        self.__currentPacketTimeoutCounter = DEFAULT_TIMEOUT // POLL_PERIOD
        self.__currentPacketRetryCounter = MAX_RETRY

        self.__write(msg)

        # wait for response
        while True:
            if not self.device.isConnected():
                # the device is disconnected
                if self.__isLastPacket:
                    print("Update complete")
                    self.__isLastPacket = False
                    sys.exit(Errno.NO_ERROR)
                else:
                    print("Device disconnected unexpectedly")
                    sys.exit(Errno.DEVICE_DISCONNECTED)

            # read from serial if the device is remained connected
            ch = self.device.read()

            # read nothing
            if ch == '':
                self.__currentPacketTimeoutCounter -= 1

                # if timeout, then retransmit
                if self.__currentPacketTimeoutCounter == 0:
                    self.__currentPacketRetryCounter -= 1

                    # if reach maximum retransmit, then quit
                    if self.__currentPacketRetryCounter == 0:
                        if self.__isLastPacket:
                            print("Update complete")
                            self.__isLastPacket = False
                            sys.exit(Errno.NO_ERROR)
                        else:
                            print("Device failed to response")
                            sys.exit(Errno.DEVICE_NO_RESPONSE)

                    # otherwise, reset the timeout and retry
                    self.__currentPacketTimeoutCounter = DEFAULT_TIMEOUT // POLL_PERIOD
                    self.__write(msg)

                time.sleep(POLL_PERIOD)
                continue

            # read something
            else:
                self.decoder.poll(ch)

                # check if proper response is received
                if (self.__isPacketAcked):
                    self.__packetIndex += 1
                    self.__isPacketAcked = False
                    break
